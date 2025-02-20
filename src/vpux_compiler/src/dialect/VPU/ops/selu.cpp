//
// Copyright (C) 2022 Intel Corporation.
// SPDX-License-Identifier: Apache 2.0
//

#include "vpux/compiler/dialect/VPU/ops.hpp"

using namespace vpux;

mlir::LogicalResult vpux::VPU::SeluOp::inferReturnTypes(mlir::MLIRContext* ctx, mlir::Optional<mlir::Location> optLoc,
                                                        mlir::ValueRange operands, mlir::DictionaryAttr attrs,
                                                        mlir::RegionRange /*regions*/,
                                                        mlir::SmallVectorImpl<mlir::Type>& inferredReturnTypes) {
    const auto loc = optLoc.value_or(mlir::UnknownLoc::get(ctx));

    VPU::SeluOpAdaptor selu(operands, attrs);
    if (mlir::failed(selu.verify(loc))) {
        return mlir::failure();
    }

    const auto inType = selu.input().getType();
    inferredReturnTypes.push_back(inType);

    return mlir::success();
}

//
// serialize
//

EMU::BlobWriter::SpecificTask vpux::VPU::SeluOp::serialize(EMU::BlobWriter& writer) {
    const auto alpha = alpha_valueAttr().getValueAsDouble();
    const auto lambda = lambda_valueAttr().getValueAsDouble();
    const auto selu = MVCNN::CreateSeluParams(writer, checked_cast<float>(alpha), checked_cast<float>(lambda));

    MVCNN::PostOpsParamsBuilder builder(writer);
    builder.add_nested_params_type(MVCNN::PostOpsNestedParams_SeluParams);
    builder.add_nested_params(selu.Union());
    const auto paramsOff = builder.Finish();

    return writer.createUPALayerTask(*this, {paramsOff.Union(), MVCNN::SoftwareLayerParams_PostOpsParams});
}
