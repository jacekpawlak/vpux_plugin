//
// Copyright (C) 2022 Intel Corporation.
// SPDX-License-Identifier: Apache 2.0
//

#include "vpux/compiler/dialect/IE/ops.hpp"

#include "vpux/compiler/dialect/IE/utils/propagate_quantize_dequantize_utils.hpp"
#include "vpux/compiler/dialect/IE/utils/reshape_utils.hpp"
#include "vpux/compiler/dialect/VPU/utils/layout_utils.hpp"
#include "vpux/compiler/dialect/const/ops.hpp"
#include "vpux/compiler/utils/attributes.hpp"
#include "vpux/compiler/utils/error.hpp"
#include "vpux/compiler/utils/types.hpp"

#include "vpux/utils/core/checked_cast.hpp"
#include "vpux/utils/core/small_vector.hpp"

#include <mlir/IR/PatternMatch.h>

#include <numeric>

using namespace vpux;

namespace {

mlir::FailureOr<mlir::Type> inferElemType(IE::AffineReshapeOpAdaptor affineReshapeOp, mlir::Type inputElemType) {
    const auto perAxisQType = inputElemType.dyn_cast_or_null<mlir::quant::UniformQuantizedPerAxisType>();
    if (perAxisQType == nullptr) {
        return inputElemType;
    }

    const auto inputQAxis = perAxisQType.getQuantizedDimension();

    const auto dimMapping = parseIntArrayOfArrayAttr<int64_t>(affineReshapeOp.dim_mapping());
    const auto outputShape = parseIntArrayAttr<int64_t>(affineReshapeOp.shape_value());
    const auto inputShape = getShape(affineReshapeOp.input()).raw();

    // get output dims for input Q axis
    const auto outputDims = dimMapping[inputQAxis];
    int64_t outQAxis = -1;
    int64_t inputQAxisSize = inputShape[inputQAxis];

    if (inputQAxisSize == 1) {
        // Per tensor, but must be per channel, do not handle it here
        return mlir::failure();
    }

    for (const auto& dim : outputDims) {
        if (inputQAxisSize == outputShape[dim]) {
            // firstly check that element is unique and others == 1
            if (std::find_if(outputDims.begin(), outputDims.end(), [&](int64_t elem) {
                    return (outputShape[elem] != 1 && outputShape[elem] != inputQAxisSize);
                }) != outputDims.end()) {
                return mlir::failure();
            }
            outQAxis = dim;
            break;
        }
    }

    if (outQAxis == -1) {
        return mlir::failure();
    }

    return mlir::quant::UniformQuantizedPerAxisType::get(
            perAxisQType.getFlags(), perAxisQType.getStorageType(), perAxisQType.getExpressedType(),
            perAxisQType.getScales(), perAxisQType.getZeroPoints(), static_cast<int32_t>(outQAxis),
            perAxisQType.getStorageTypeMin(), perAxisQType.getStorageTypeMax());
}

}  // namespace

//
// inferReturnTypeComponents
//

mlir::LogicalResult vpux::IE::AffineReshapeOp::inferReturnTypeComponents(
        mlir::MLIRContext* ctx, Optional<mlir::Location> optLoc, mlir::ValueShapeRange operands,
        mlir::DictionaryAttr attrs, mlir::RegionRange,
        SmallVectorImpl<mlir::ShapedTypeComponents>& inferredReturnShapes) {
    const auto loc = optLoc.value_or(mlir::UnknownLoc::get(ctx));

    IE::AffineReshapeOpAdaptor affineReshape(operands, attrs);
    if (mlir::failed(affineReshape.verify(loc))) {
        return mlir::failure();
    }

    const auto outShape = parseIntArrayAttr<int64_t>(affineReshape.shape_value());
    const auto input = affineReshape.input();
    const auto inType = input.getType().cast<mlir::RankedTensorType>();
    const auto ndInType = inType.cast<vpux::NDTypeInterface>();
    const auto inOrder = DimsOrder::fromValue(input);

    const auto outputLayout =
            vpux::VPU::inferAffineReshapeOutputLayout(inOrder.toPermutation(), affineReshape.dim_mapping());
    if (mlir::failed(outputLayout)) {
        return mlir::failure();
    }

    const auto outDesc = vpux::getTensorAttr(ctx, outputLayout.value(), ndInType.getMemSpace());

    const auto elemTypeInferResult = inferElemType(affineReshape, ndInType.getElementType());
    if (mlir::failed(elemTypeInferResult)) {
        inferredReturnShapes.emplace_back(outShape, ndInType.getElementType(), outDesc);
    } else {
        inferredReturnShapes.emplace_back(outShape, elemTypeInferResult.value(), outDesc);
    }

    return mlir::success();
}

//
// inferElemTypeInfo
//

void vpux::IE::AffineReshapeOp::inferElemTypeInfo(vpux::IE::LayerDataInfo<mlir::Type>& info) {
    auto outputElemType = inferElemType(*this, info.getInput(0));
    if (mlir::failed(outputElemType)) {
        return;
    }

    for (size_t outputInd = 0; outputInd < info.getNumOutputs(); ++outputInd) {
        info.setOutput(outputInd, outputElemType.value());
    }
}

void vpux::IE::AffineReshapeOp::inferElemTypeInfoUp(vpux::IE::LayerDataInfo<mlir::Type>& info) {
    // E#84659: implement propagate type up for per channel, currently it leads to failures in later passes.
    propagateElementTypeUp(info);
}

//
// verify
//

mlir::LogicalResult vpux::IE::AffineReshapeOp::verify() {
    const auto inType = input().getType().cast<vpux::NDTypeInterface>();
    const auto outType = output().getType().cast<vpux::NDTypeInterface>();

    auto inNumElem = inType.getNumElements();
    auto outNumElem = outType.getNumElements();
    if (inNumElem != outNumElem) {
        return errorAt(*this,
                       "AffineReshape input and output must have the same number of elements. Got: input number '{0}'; "
                       "output number '{1}'",
                       inNumElem, outNumElem);
    }

    return mlir::success();
}

//
// fold
//

mlir::OpFoldResult vpux::IE::AffineReshapeOp::fold(ArrayRef<mlir::Attribute> operands) {
    auto inputType = input().getType().cast<vpux::NDTypeInterface>();
    auto outputType = output().getType().cast<vpux::NDTypeInterface>();
    if (inputType == outputType) {
        return input();
    }

    VPUX_THROW_UNLESS(!operands.empty(), "Wrong number of operands : {0}", operands.size());

    if (const auto attr = operands[0].dyn_cast_or_null<Const::ContentAttr>()) {
        const auto inputElemType =
                inputType.getElementType().dyn_cast_or_null<mlir::quant::UniformQuantizedPerAxisType>();
        const auto outputElemType =
                outputType.getElementType().dyn_cast_or_null<mlir::quant::UniformQuantizedPerAxisType>();
        if (inputElemType && outputElemType && isQuantizedDimensionPermutation(inputElemType, outputElemType)) {
            const auto newShape = outputType.getShape();
            return attr.changeShapeAndElemType(newShape, outputElemType);
        }
        return attr.reshape(getShape(output()));
    }

    return nullptr;
}

//
// FuseAffineReshapes
//

namespace {
class FuseAffineReshapes final : public mlir::OpRewritePattern<IE::AffineReshapeOp> {
public:
    using mlir::OpRewritePattern<IE::AffineReshapeOp>::OpRewritePattern;

public:
    mlir::LogicalResult matchAndRewrite(IE::AffineReshapeOp origOp, mlir::PatternRewriter& rewriter) const final;
};

mlir::LogicalResult FuseAffineReshapes::matchAndRewrite(IE::AffineReshapeOp origOp,
                                                        mlir::PatternRewriter& rewriter) const {
    auto prevOp = origOp.input().getDefiningOp<IE::AffineReshapeOp>();
    if (prevOp == nullptr) {
        return mlir::failure();
    }

    auto inputType = prevOp.input().getType().cast<NDTypeInterface>();
    auto outputType = origOp.output().getType().cast<NDTypeInterface>();

    const auto inputDimsOrder = inputType.getDimsOrder();
    const auto outputDimsOrder = outputType.getDimsOrder();

    const auto inputShape = inputType.cast<mlir::ShapedType>().getShape();
    const auto outputShape = outputType.cast<mlir::ShapedType>().getShape();
    const auto outputShapeAttr = getIntArrayAttr(getContext(), outputShape);

    // Fusing AffineReshape with any of the above mentioned ops might result in another AffineReshape or not,
    // depending on the resulting input and output shapes.
    // E. g. 1 x 24 x 2 x 2 -> AffineReshape -> 1 x 24 x 4 -> AffineReshape -> 1 x 24 x 4 x 1
    //       mapping: id0 = od0, id1 = od1 and id2 * id3 = od2 * od3 (not an AffineReshape)
    // If the Reshape that replaces the two ops ends up being a valid AffineReshape, then it will be converted by
    // Reshape's canonicalizer.
    // TODO: E#70418 1. support reshape(in: NHWC, out: NHWC) 2. support different in&out order of reshape
    if (inputDimsOrder == outputDimsOrder && inputDimsOrder == DimsOrder::NHWC) {
        const auto reassociationMap = vpux::IE::getReassociationMap(inputShape, outputShape);

        if (mlir::failed(reassociationMap)) {
            return mlir::failure();
        }

        rewriter.replaceOpWithNewOp<IE::AffineReshapeOp>(
                origOp, prevOp.input(), getIntArrayOfArray(getContext(), reassociationMap.value()), outputShapeAttr);
        return mlir::success();
    }
    // Reshape's output dim order is limited to NCHW, so the compiler will not fuse the ops in this case
    if (outputDimsOrder != DimsOrder::NCHW) {
        return mlir::failure();
    }

    rewriter.replaceOpWithNewOp<IE::ReshapeOp>(origOp, prevOp->getOperand(0), nullptr, false, outputShapeAttr);
    return mlir::success();
}

}  // namespace

//
// FuseWithReshape
//

namespace {
class FuseWithReshape final : public mlir::OpRewritePattern<IE::AffineReshapeOp> {
public:
    using mlir::OpRewritePattern<IE::AffineReshapeOp>::OpRewritePattern;

public:
    mlir::LogicalResult matchAndRewrite(IE::AffineReshapeOp origOp, mlir::PatternRewriter& rewriter) const final;
};

mlir::LogicalResult FuseWithReshape::matchAndRewrite(IE::AffineReshapeOp origOp,
                                                     mlir::PatternRewriter& rewriter) const {
    auto prevOp = origOp.input().getDefiningOp();
    if (prevOp == nullptr) {
        return mlir::failure();
    }
    if (!mlir::isa<IE::SqueezeOp, IE::UnsqueezeOp, IE::ReshapeOp>(prevOp)) {
        return mlir::failure();
    }
    const auto outputShape = origOp.getType().getShape();
    const auto outputShapeAttr = getIntArrayAttr(getContext(), outputShape);

    // Fusing AffineReshape with any of the above mentioned ops might result in another AffineReshape or not,
    // depending on the resulting input and output shapes.
    // If the Reshape that replaces the two ops ends up being a valid AffineReshape, then it will be converted by
    // Reshape's canonicalizer.
    rewriter.replaceOpWithNewOp<IE::ReshapeOp>(origOp, prevOp->getOperand(0), nullptr, false, outputShapeAttr);
    return mlir::success();
}

}  // namespace

//
// getCanonicalizationPatterns
//

void vpux::IE::AffineReshapeOp::getCanonicalizationPatterns(mlir::RewritePatternSet& patterns, mlir::MLIRContext* ctx) {
    patterns.add<FuseAffineReshapes>(ctx);
    patterns.add<FuseWithReshape>(ctx);
}
