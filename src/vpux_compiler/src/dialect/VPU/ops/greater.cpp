//
// Copyright (C) 2022 Intel Corporation.
// SPDX-License-Identifier: Apache 2.0
//

#include "vpux/compiler/dialect/IE/utils/shape_infer.hpp"
#include "vpux/compiler/dialect/VPU/ops.hpp"

#include "vpux/utils/core/checked_cast.hpp"

using namespace vpux;

mlir::LogicalResult vpux::VPU::GreaterOp::inferReturnTypes(mlir::MLIRContext* ctx,
                                                           mlir::Optional<mlir::Location> optLoc,
                                                           mlir::ValueRange operands, mlir::DictionaryAttr attrs,
                                                           mlir::RegionRange /*regions*/,
                                                           mlir::SmallVectorImpl<mlir::Type>& inferredReturnTypes) {
    const auto loc = optLoc.value_or(mlir::UnknownLoc::get(ctx));

    VPU::GreaterOpAdaptor greater(operands, attrs);
    if (mlir::failed(greater.verify(loc))) {
        return mlir::failure();
    }

    const auto in1Type = greater.input1().getType().cast<vpux::NDTypeInterface>();
    const auto in2Type = greater.input2().getType().cast<vpux::NDTypeInterface>();

    const auto outShapeRes = IE::broadcastEltwiseShape(in1Type.getShape().raw(), in2Type.getShape().raw(),
                                                       greater.auto_broadcast(), loc);

    if (mlir::succeeded(outShapeRes)) {
        const auto outType = in1Type.changeShape(Shape(outShapeRes.value()));
        inferredReturnTypes.push_back(outType);
    }

    return mlir::success();
}

//
// serialize
//

EMU::BlobWriter::SpecificTask vpux::VPU::GreaterOp::serialize(EMU::BlobWriter& writer) {
    EMU::BlobWriter::String type;
    type = writer.createString("comparegt");

    MVCNN::EltwiseParamsBuilder builder(writer);
    builder.add_operation(type);
    const auto paramsOff = builder.Finish();

    return writer.createUPALayerTask(*this, {paramsOff.Union(), MVCNN::SoftwareLayerParams_EltwiseParams});
}
