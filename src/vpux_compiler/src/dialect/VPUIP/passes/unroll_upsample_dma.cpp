//
// Copyright (C) 2022 Intel Corporation.
// SPDX-License-Identifier: Apache 2.0
//

#include "vpux/compiler/dialect/IE/utils/resources.hpp"
#include "vpux/compiler/dialect/VPUIP/passes.hpp"

#include "vpux/compiler/core/aliases_info.hpp"
#include "vpux/compiler/core/cost_model_utils.hpp"
#include "vpux/compiler/dialect/VPU/attributes.hpp"
#include "vpux/compiler/dialect/VPUIP/convert_to_dma_utils.hpp"
#include "vpux/compiler/dialect/VPUIP/dma_descriptor_generator.hpp"
#include "vpux/compiler/dialect/VPURT/attributes.hpp"
#include "vpux/compiler/dialect/VPURT/ops.hpp"
#include "vpux/compiler/dialect/VPURT/task.hpp"
#include "vpux/compiler/utils/attributes.hpp"
#include "vpux/compiler/utils/rewriter.hpp"

#include <llvm/ADT/DenseMap.h>
#include <mlir/Transforms/GreedyPatternRewriteDriver.h>

#include <numeric>

using namespace vpux;

namespace {

//
// UpsamplingDMARewriter
//

class UpsamplingDMARewriter final : public mlir::OpRewritePattern<VPUIP::UpsamplingDMAOp> {
public:
    UpsamplingDMARewriter(mlir::MLIRContext* ctx, int64_t dmaPortCount, Logger log)
            : mlir::OpRewritePattern<VPUIP::UpsamplingDMAOp>(ctx), _log(log), _dmaPortCount(dmaPortCount) {
        setDebugName("UpsamplingDMARewriter");

        _cmxNameAttr = mlir::FlatSymbolRefAttr::get(ctx, stringifyEnum(VPU::MemoryKind::CMX_NN));
    }

    mlir::LogicalResult matchAndRewrite(VPUIP::UpsamplingDMAOp UpsamplingDMAOp,
                                        mlir::PatternRewriter& rewriter) const final;

private:
    Logger _log;
    int64_t _dmaPortCount;
    mlir::FlatSymbolRefAttr _cmxNameAttr;
};

static VPUIP::DMADescriptorAttr generateUpsamplingDmaDescriptor(mlir::MLIRContext* ctx, vpux::ShapeRef inShape,
                                                                mlir::ArrayAttr factor, Byte inElemTypeSize,
                                                                int64_t expandChannel) {
    auto upsampleFactor = parseIntArrayAttr<int64_t>(factor);
    auto elemTypeSize = inElemTypeSize.count();

    const auto IC = inShape[Dims4D::Act::C];
    const auto H = inShape[Dims4D::Act::H];
    const auto W = inShape[Dims4D::Act::W];
    auto len = vpux::getIntAttr(ctx, W * IC * elemTypeSize);
    auto srcWidth = vpux::getIntAttr(ctx, IC * W * elemTypeSize);
    auto srcStride = vpux::getIntAttr(ctx, IC * W * elemTypeSize);
    auto srcPlaneStride = vpux::getIntAttr(ctx, W * IC * elemTypeSize);
    auto dstWidth = vpux::getIntAttr(ctx, IC * elemTypeSize);
    auto dstStride = vpux::getIntAttr(ctx, (IC + expandChannel) * elemTypeSize * upsampleFactor[Dims4D::Act::W.ind()]);
    auto dstPlaneStride =
            vpux::getIntAttr(ctx, W * (IC + expandChannel) * elemTypeSize * upsampleFactor[Dims4D::Act::H.ind()] *
                                          upsampleFactor[Dims4D::Act::W.ind()]);
    auto numPlanes = vpux::getIntAttr(ctx, H);
    return VPUIP::DMADescriptorAttr::get(ctx, numPlanes, len, srcWidth, srcStride, srcPlaneStride, dstWidth, dstStride,
                                         dstPlaneStride);
}

mlir::LogicalResult UpsamplingDMARewriter::matchAndRewrite(VPUIP::UpsamplingDMAOp upsamplingDMAOp,
                                                           mlir::PatternRewriter& rewriter) const {
    _log.trace("Process UpsamplingDMA op: {0}", upsamplingDMAOp);
    if (upsamplingDMAOp.dma_descriptor().has_value()) {
        return mlir::failure();
    }

    auto vpurtTask = upsamplingDMAOp->getParentOfType<VPURT::TaskOp>();
    VPUX_THROW_UNLESS(vpurtTask != nullptr, "Can't get VPURT task operation");
    auto cycleBeginAttr = vpurtTask->getAttr(cycleBegin);
    auto cycleEndAttr = vpurtTask->getAttr(cycleEnd);
    rewriter.setInsertionPointAfter(vpurtTask);

    auto inType = upsamplingDMAOp.input().getType().cast<vpux::NDTypeInterface>();
    Byte elemTypeSize = inType.getElemTypeSize();

    auto srcDeclBuff = upsamplingDMAOp.input().getDefiningOp<VPURT::DeclareBufferOp>();
    VPUX_THROW_UNLESS(srcDeclBuff != nullptr, "Can't get buffer for operand: {0}", upsamplingDMAOp.input());
    auto dstDeclBuff = upsamplingDMAOp.output_buff().getDefiningOp<VPURT::DeclareBufferOp>();
    VPUX_THROW_UNLESS(dstDeclBuff != nullptr, "Can't get buffer for operand: {0}", upsamplingDMAOp.output_buff());
    auto srcType = srcDeclBuff.getType().cast<vpux::NDTypeInterface>();
    auto dstType = dstDeclBuff.getType().cast<vpux::NDTypeInterface>();

    const auto inOrder = DimsOrder::fromValue(upsamplingDMAOp.input());
    auto subShape = Shape(inType.getShape().raw());
    int64_t totalNumPlane = inType.getShape()[Dims4D::Act::N] * inType.getShape()[Dims4D::Act::H];
    subShape[Dims4D::Act::N] = 1;
    subShape[Dims4D::Act::H] = VPUIP::DMA_MAX_NUMBER_PLANES;
    // Convert dim C to 1 when NCHW
    // That can make NCHW's generation logic same as NHWC layout
    if (inOrder == DimsOrder::NCHW) {
        totalNumPlane *= inType.getShape()[Dims4D::Act::C];
        subShape[Dims4D::Act::C] = 1;
    }

    auto numberDMAs = divUp(totalNumPlane, VPUIP::DMA_MAX_NUMBER_PLANES);
    SmallVector<Shape> subInputShapes(numberDMAs - 1, subShape);
    subShape[Dims4D::Act::H] = totalNumPlane - (VPUIP::DMA_MAX_NUMBER_PLANES * (numberDMAs - 1));
    subInputShapes.push_back(subShape);
    auto srcOffset = srcDeclBuff.getByteOffset();
    auto dstOffset = dstDeclBuff.getByteOffset();
    auto context = upsamplingDMAOp.getContext();
    auto upsamplingFactor = parseIntArrayAttr<int64_t>(upsamplingDMAOp.upsampling_factor());
    auto hasExpandAttr = upsamplingDMAOp.expand().has_value();
    SmallVector<int64_t, 4> expand;
    if (hasExpandAttr) {
        expand = extractFromI64ArrayAttr(upsamplingDMAOp.expandAttr());
    }

    auto getOutShape = [&upsamplingFactor, &hasExpandAttr, &expand](ShapeRef inShape) {
        auto outShape = Shape(inShape.raw());
        for (size_t i = 0; i < outShape.size(); i++) {
            outShape[Dim(i)] *= upsamplingFactor[i];
            if (hasExpandAttr) {
                outShape[Dim(i)] += expand[i];
            }
        }
        return outShape;
    };

    int64_t dmaPort = 0;
    for (auto& inputShape : subInputShapes) {
        auto newSrcMemRef = vpux::getMemRefType(inputShape, srcType.getElementType(), inOrder, srcType.getMemSpace());
        auto newSrcBuff = VPUIP::createNewDeclareBuffer(rewriter, srcDeclBuff, srcDeclBuff, newSrcMemRef, srcOffset);

        auto outShape = getOutShape(inputShape);
        auto newDstMemRef = vpux::getMemRefType(outShape, dstType.getElementType(), inOrder, dstType.getMemSpace());
        auto newDstBuff = VPUIP::createNewDeclareBuffer(rewriter, dstDeclBuff, dstDeclBuff, newDstMemRef, dstOffset);
        auto descriptorAttr =
                hasExpandAttr
                        ? generateUpsamplingDmaDescriptor(context, inputShape, upsamplingDMAOp.upsampling_factor(),
                                                          inType.getElemTypeSize(), expand[1])
                        : generateUpsamplingDmaDescriptor(context, inputShape, upsamplingDMAOp.upsampling_factor(),
                                                          inType.getElemTypeSize(), 0);

        auto nextOffset = srcOffset + inputShape.totalSize() * elemTypeSize.count();
        const auto newLoc =
                appendLoc(upsamplingDMAOp->getLoc(), "_unroll_upsamplingDMA[{0},{1}]", srcOffset, nextOffset);
        const auto newUpsamplingDMA = VPURT::wrapIntoTaskOp<VPUIP::UpsamplingDMAOp>(
                rewriter, vpurtTask.getWaitBarriers(), vpurtTask.getUpdateBarriers(), newLoc, newSrcBuff, newDstBuff,
                upsamplingDMAOp.upsampling_factorAttr(), descriptorAttr, upsamplingDMAOp.expandAttr(), dmaPort,
                upsamplingDMAOp.channelTypeAttr(), upsamplingDMAOp.dma_hwp_idAttr());

        auto newVpurtTask = newUpsamplingDMA->getParentOfType<VPURT::TaskOp>();
        if (cycleBeginAttr) {
            newVpurtTask->setAttr(cycleBegin, cycleBeginAttr);
        }
        if (cycleEndAttr) {
            newVpurtTask->setAttr(cycleEnd, cycleEndAttr);
        }
        dmaPort = (dmaPort + 1) % _dmaPortCount;
        srcOffset = nextOffset;
        dstOffset += outShape.totalSize() * elemTypeSize.count();
    }
    rewriter.eraseOp(vpurtTask);
    return mlir::success();
}

//
// UnrollUpsamplingDMAPass
//

class UnrollUpsamplingDMAPass final : public VPUIP::UnrollUpsamplingDMABase<UnrollUpsamplingDMAPass> {
public:
    explicit UnrollUpsamplingDMAPass(Logger log) {
        Base::initLogger(log, Base::getArgumentName());
    }

private:
    void safeRunOnFunc() final;
};

void UnrollUpsamplingDMAPass::safeRunOnFunc() {
    auto& ctx = getContext();
    auto func = getOperation();
    auto module = func->getParentOfType<mlir::ModuleOp>();
    auto dmaOp = IE::getAvailableExecutor(module, VPU::ExecutorKind::DMA_NN);
    auto dmaPortCount = dmaOp.count();

    mlir::RewritePatternSet patterns(&ctx);
    patterns.insert<UpsamplingDMARewriter>(&ctx, dmaPortCount, _log);

    if (mlir::failed(
                mlir::applyPatternsAndFoldGreedily(func, std::move(patterns), vpux::getDefaultGreedyRewriteConfig()))) {
        signalPassFailure();
    }
}

}  // namespace

//
// createUnrollUpsamplingDMAPass
//

std::unique_ptr<mlir::Pass> vpux::VPUIP::createUnrollUpsamplingDMAPass(Logger log) {
    return std::make_unique<UnrollUpsamplingDMAPass>(log);
}
