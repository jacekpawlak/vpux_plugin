//
// Copyright (C) 2022 Intel Corporation.
// SPDX-License-Identifier: Apache 2.0
//

#include "vpux/compiler/dialect/VPU/attributes.hpp"
#include "vpux/compiler/dialect/VPU/ops.hpp"
#include "vpux/compiler/dialect/VPU/utils/const_utils.hpp"

#include "vpux/compiler/core/attributes/shape.hpp"
#include "vpux/compiler/core/layers.hpp"
#include "vpux/compiler/dialect/VPUIP/graph-schema/utils.hpp"
#include "vpux/compiler/utils/attributes.hpp"
#include "vpux/compiler/utils/empty_node.hpp"
#include "vpux/compiler/utils/error.hpp"

#include "vpux/utils/core/checked_cast.hpp"
#include "vpux/utils/core/error.hpp"

#include <mlir/IR/BlockAndValueMapping.h>
#include <mlir/IR/PatternMatch.h>

#include <ngraph/coordinate.hpp>
#include <ngraph/validation_util.hpp>

using namespace vpux;

mlir::LogicalResult vpux::VPU::GroupConvolutionOp::inferReturnTypes(
        mlir::MLIRContext* ctx, mlir::Optional<mlir::Location> optLoc, mlir::ValueRange operands,
        mlir::DictionaryAttr attrs, mlir::RegionRange /*regions*/,
        mlir::SmallVectorImpl<mlir::Type>& inferredReturnTypes) {
    const auto loc = optLoc.value_or(mlir::UnknownLoc::get(ctx));

    VPU::GroupConvolutionOpAdaptor conv(operands, attrs);
    if (mlir::failed(conv.verify(loc))) {
        return mlir::failure();
    }

    auto inShape = to_small_vector(conv.input().getType().cast<vpux::NDTypeInterface>().getShape().raw());
    const auto inType = conv.input().getType().cast<vpux::NDTypeInterface>();
    auto filterShape = to_small_vector(conv.filter().getType().cast<vpux::NDTypeInterface>().getShape().raw());

    const auto dataPaddingBelow = parseIntArrayAttr<int64_t>(conv.pads_end());
    const auto dataPaddingAbove = parseIntArrayAttr<int64_t>(conv.pads_begin());
    const auto windowStrides = parseIntArrayAttr<int64_t>(conv.strides());
    const auto windowDilations = parseIntArrayAttr<int64_t>(conv.dilations());

    int64_t groups = 0;
    if (conv.groups().value_or(0) != 0) {
        if (filterShape.size() != inShape.size()) {
            return errorAt(loc, "Input size '{0}' does not match filter size '{1}'. (groups != 0)", inShape.size(),
                           filterShape.size());
        }

        groups = conv.groups().value();
    } else {
        if (filterShape.size() != inShape.size() + 1) {
            return errorAt(loc, "Input size '{0}' does not match filter size '{1}'. (groups == 0)", inShape.size() + 1,
                           filterShape.size());
        }

        groups = filterShape[0];

        // we need to adjust filters_shape to reuse helpers for normal convolution
        filterShape[1] *= groups;
        filterShape.erase(filterShape.begin());
    }

    inShape[1] /= groups;

    const auto outputShape =
            ngraph::infer_convolution_forward(EmptyNode::instance(), ngraph::Shape(inShape.begin(), inShape.end()),
                                              ngraph::Strides(windowStrides.size(), 1),  // dummy data dilations
                                              ngraph::CoordinateDiff(dataPaddingBelow.begin(), dataPaddingBelow.end()),
                                              ngraph::CoordinateDiff(dataPaddingAbove.begin(), dataPaddingAbove.end()),
                                              ngraph::Shape(filterShape.begin(), filterShape.end()),
                                              ngraph::Strides(windowStrides.begin(), windowStrides.end()),
                                              ngraph::Strides(windowDilations.begin(), windowDilations.end()));

    const auto shapeI64 = to_small_vector(outputShape.get_shape() | transformed([](size_t val) {
                                              return checked_cast<int64_t>(val);
                                          }));

    const auto outType = inType.changeShape(Shape(shapeI64));
    inferredReturnTypes.push_back(outType);

    return mlir::success();
}

InputTiling vpux::VPU::GroupConvolutionOp::backInferTileInfo(const vpux::TileInfo& outputTile, vpux::Logger /*log*/) {
    const auto origInputShape = getShape(input());
    const auto origFilterShape = getShape(filter());
    const auto origBiasShape = bias() != nullptr ? getShape(bias()) : ShapeRef();
    const auto origPadding = PadInfo(pads_begin(), pads_end());
    const auto origGroups = groups().value_or(1);

    return backInferGroupConvTile(outputTile, origInputShape, origFilterShape, origBiasShape, strides(), origPadding,
                                  origGroups);
}

//
// fitIntoCMX
//

bool vpux::VPU::GroupConvolutionOp::fitIntoCMX(vpux::NDTypeInterface input, vpux::NDTypeInterface filter,
                                               vpux::NDTypeInterface output) {
    return fitIntoCMX(input, filter, output, Byte(0));
}

bool vpux::VPU::GroupConvolutionOp::fitIntoCMX(vpux::NDTypeInterface input, vpux::NDTypeInterface filter,
                                               vpux::NDTypeInterface output, Byte reservedMem) {
    SmallVector<Byte> buffers = {input.getTotalAllocSize(), filter.getTotalAllocSize(), output.getTotalAllocSize()};

    auto totalAvailableCMXSize = reservedMem.count() == 0 ? getTotalCMXSize(getOperation()).count()
                                                          : getTotalCMXFragmentationAwareSize(getOperation()).count();

    return vpux::VPU::calculateAlignedBuffersMemoryRequirement(getArch(getOperation()), buffers).count() +
                   reservedMem.count() <=
           totalAvailableCMXSize;
}

void vpux::VPU::GroupConvolutionOp::adjustAttrs(const TilingInfo& inputTiling, const TileInfo& /*outputTile*/) {
    const auto& inputTiles = inputTiling.tiles;
    VPUX_THROW_UNLESS(inputTiles.size() > 1, "Missed tile information. Got {0} tiles info, must be at least 2",
                      inputTiles.size());

    IE::adjustPaddings(this, inputTiling);

    const auto& inputTile = inputTiles[0];
    const auto& filterTile = inputTiles[1];
    const auto groups = inputTile.shape[Dims4D::Act::C] / filterTile.shape[Dims4D::Filter::IC];
    const auto groupsNewAttr = getIntAttr(getContext(), groups);

    groupsAttr(groupsNewAttr);
}

mlir::FailureOr<OutputTiling> vpux::VPU::GroupConvolutionOp::getTilingStrategy(TilingMode tilingMode, Logger log) {
    return vpux::getSWLayerTilingStrategy(this->getOperation(), tilingMode, log);
}

//
// serialize
//

EMU::BlobWriter::SpecificTask vpux::VPU::GroupConvolutionOp::serialize(EMU::BlobWriter& writer) {
    static const auto dY = Dim(2);
    static const auto dX = Dim(3);

    const auto strides = VPUIP::createOrder3(stridesAttr());
    const auto dilations = VPUIP::createOrder3(dilationsAttr());
    const auto padsBegin = VPUIP::createOrder3(pads_beginAttr());
    const auto padsEnd = VPUIP::createOrder3(pads_endAttr());

    const auto filterShape = getShape(filter());
    const auto kernel =
            MVCNN::order3(checked_cast<uint8_t>(filterShape[dX]), checked_cast<uint8_t>(filterShape[dY]), 0);

    if (groups().value() > 1) {
        MVCNN::ConvolutionParamsBuilder builder(writer);
        builder.add_kernel(&kernel);
        builder.add_strides(&strides);
        builder.add_dilations(&dilations);
        builder.add_pads_begin(&padsBegin);
        builder.add_pads_end(&padsEnd);
        builder.add_group(checked_cast<int32_t>(groups().value()));
        const auto paramsOff = builder.Finish();
        return writer.createUPALayerTask(*this, {paramsOff.Union(), MVCNN::SoftwareLayerParams_ConvolutionParams});
    } else {
        MVCNN::SWConvolutionParamsBuilder builder(writer);
        builder.add_kernel(&kernel);
        builder.add_strides(&strides);
        builder.add_dilations(&dilations);
        builder.add_pads_begin(&padsBegin);
        builder.add_pads_end(&padsEnd);
        builder.add_group(checked_cast<int32_t>(groups().value()));
        const auto paramsOff = builder.Finish();
        return writer.createUPALayerTask(*this, {paramsOff.Union(), MVCNN::SoftwareLayerParams_SWConvolutionParams});
    }
}
