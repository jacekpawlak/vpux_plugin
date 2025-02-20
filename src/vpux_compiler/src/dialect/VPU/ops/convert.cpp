//
// Copyright (C) 2022 Intel Corporation.
// SPDX-License-Identifier: Apache 2.0
//

#include "vpux/compiler/dialect/VPU/ops.hpp"
#include "vpux/compiler/dialect/VPU/utils/const_utils.hpp"
#include "vpux/compiler/dialect/VPU/utils/distributed_tensor_utils.hpp"

using namespace vpux;

mlir::LogicalResult vpux::VPU::ConvertOp::inferReturnTypes(mlir::MLIRContext* ctx,
                                                           mlir::Optional<mlir::Location> optLoc,
                                                           mlir::ValueRange operands, mlir::DictionaryAttr attrs,
                                                           mlir::RegionRange /*regions*/,
                                                           mlir::SmallVectorImpl<mlir::Type>& inferredReturnTypes) {
    const auto loc = optLoc.value_or(mlir::UnknownLoc::get(ctx));

    VPU::ConvertOpAdaptor cvt(operands, attrs);
    if (mlir::failed(cvt.verify(loc))) {
        return mlir::failure();
    }

    const auto inType = cvt.input().getType().cast<vpux::NDTypeInterface>();
    const auto dstElemType = cvt.dstElemType();

    const auto outType = inType.changeElemType(dstElemType);
    inferredReturnTypes.push_back(outType);

    return mlir::success();
}

bool vpux::VPU::ConvertOp::areCastCompatible(mlir::TypeRange inputs, mlir::TypeRange outputs) {
    if (inputs.size() != 1 || outputs.size() != 1) {
        return false;
    }

    const auto input = inputs.front().dyn_cast<vpux::NDTypeInterface>();
    const auto output = outputs.front().dyn_cast<vpux::NDTypeInterface>();

    if (!input || !output || input.getShape() != output.getShape()) {
        return false;
    }

    return true;
}

//
// serialize
//

EMU::BlobWriter::SpecificTask vpux::VPU::ConvertOp::serialize(EMU::BlobWriter& writer) {
    MVCNN::ConvertParamsBuilder builder(writer);
    builder.add_scale(checked_cast<float>(1.0));
    builder.add_bias(checked_cast<float>(0.0));
    builder.add_from_detection_output(false);
    builder.add_have_batch(false);
    builder.add_batch_id(0);
    const auto paramsOff = builder.Finish();

    return writer.createUPALayerTask(*this, {paramsOff.Union(), MVCNN::SoftwareLayerParams_ConvertParams});
}

bool vpux::VPU::ConvertOp::checkStrategyCompatibility(vpux::VPU::MultiClusterStrategy strategy) {
    bool isStrategyCompatible = false;
    constexpr int64_t MIN_DIM_SIZE_FOR_TILING = 4;
    auto inputShape = getShape(input());

    switch (strategy) {
    case VPU::MultiClusterStrategy::Clustering:
        isStrategyCompatible = true;
        break;

    case VPU::MultiClusterStrategy::SplitOverHeight:
    case VPU::MultiClusterStrategy::SplitOverHeightOverlapped:
        isStrategyCompatible = inputShape[Dims4D::Act::H] >= MIN_DIM_SIZE_FOR_TILING;
        break;
    case VPU::MultiClusterStrategy::SplitOverKernel:
        isStrategyCompatible = inputShape[Dims4D::Act::C] >= MIN_DIM_SIZE_FOR_TILING;
        break;
    default:
        break;
    }
    return isStrategyCompatible;
}

vpux::VPU::DistributedTensorAttr vpux::VPU::ConvertOp::getExplicitDistributedTensorAttr(
        vpux::ShapeRef shape, vpux::VPU::DistributionMode distributionMode, mlir::ArrayAttr numTiles,
        mlir::IntegerAttr numClusters, mlir::ArrayAttr alignment, mlir::ArrayAttr /*kernel*/,
        vpux::VPU::PaddingAttr /*pad*/, mlir::ArrayAttr /*stride*/, mlir::UnitAttr uniformDistributedSegments) {
    return VPU::getSWExplicitDistributedTensorAttr(mlir::dyn_cast<VPU::SWOpInterface>(getOperation()), shape,
                                                   distributionMode, numTiles, numClusters, alignment,
                                                   uniformDistributedSegments);
}

//
// fitIntoCMX
//

bool vpux::VPU::ConvertOp::fitIntoCMX(llvm::ArrayRef<vpux::NDTypeInterface> buffers, Byte reservedMem) {
    SmallVector<Byte> buffersSize;
    std::transform(buffers.begin(), buffers.end(), std::back_inserter(buffersSize), [](const auto buffer) {
        return buffer.getTotalAllocSize();
    });

    auto totalAvailableCMXSize = reservedMem.count() == 0 ? getTotalCMXSize(getOperation()).count()
                                                          : getTotalCMXFragmentationAwareSize(getOperation()).count();

    return vpux::VPU::calculateAlignedBuffersMemoryRequirement(getArch(getOperation()), buffersSize).count() +
                   reservedMem.count() <=
           totalAvailableCMXSize;
}

bool vpux::VPU::ConvertOp::fitIntoCMX(llvm::ArrayRef<vpux::NDTypeInterface> buffers) {
    return fitIntoCMX(buffers, Byte(0));
}

bool vpux::VPU::ConvertOp::supportCycleCostCalculation() {
    return false;
}

//
// build
//

void vpux::VPU::ConvertOp::build(::mlir::OpBuilder& builder, ::mlir::OperationState& state, ::mlir::Value input,
                                 ::mlir::TypeAttr dstElemType) {
    build(builder, state, input, dstElemType, {});
}
