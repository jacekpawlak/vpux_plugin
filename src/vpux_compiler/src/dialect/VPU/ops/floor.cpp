//
// Copyright (C) 2022 Intel Corporation.
// SPDX-License-Identifier: Apache 2.0
//

#include "vpux/compiler/dialect/VPU/ops.hpp"

using namespace vpux;

mlir::LogicalResult vpux::VPU::FloorOp::inferReturnTypes(mlir::MLIRContext* ctx, mlir::Optional<mlir::Location> optLoc,
                                                         mlir::ValueRange operands, mlir::DictionaryAttr attrs,
                                                         mlir::RegionRange /*regions*/,
                                                         mlir::SmallVectorImpl<mlir::Type>& inferredReturnTypes) {
    const auto loc = optLoc.value_or(mlir::UnknownLoc::get(ctx));

    VPU::FloorOpAdaptor floor(operands, attrs);
    if (mlir::failed(floor.verify(loc))) {
        return mlir::failure();
    }

    const auto inType = floor.input().getType();
    inferredReturnTypes.push_back(inType);

    return mlir::success();
}

//
// serialize
//

EMU::BlobWriter::SpecificTask vpux::VPU::FloorOp::serialize(EMU::BlobWriter& writer) {
    const auto floor = MVCNN::CreateFloorParams(writer);

    MVCNN::PostOpsParamsBuilder builder(writer);
    builder.add_nested_params_type(MVCNN::PostOpsNestedParams_FloorParams);
    builder.add_nested_params(floor.Union());
    const auto paramsOff = builder.Finish();

    return writer.createUPALayerTask(*this, {paramsOff.Union(), MVCNN::SoftwareLayerParams_PostOpsParams});
}
