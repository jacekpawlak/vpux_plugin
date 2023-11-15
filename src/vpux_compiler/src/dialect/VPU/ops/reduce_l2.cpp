//
// Copyright (C) 2022 Intel Corporation.
// SPDX-License-Identifier: Apache 2.0
//

#include "vpux/compiler/dialect/VPU/ops.hpp"

#include "vpux/compiler/dialect/IE/utils/reduce_infer.hpp"
#include "vpux/compiler/dialect/IE/utils/shape_infer.hpp"
#include "vpux/compiler/dialect/VPU/utils/type_infer.hpp"
#include "vpux/compiler/utils/attributes.hpp"
#include "vpux/compiler/utils/error.hpp"

#include "vpux/utils/core/checked_cast.hpp"

using namespace vpux;

mlir::LogicalResult vpux::VPU::ReduceL2Op::inferReturnTypes(mlir::MLIRContext* ctx,
                                                            mlir::Optional<mlir::Location> optLoc,
                                                            mlir::ValueRange operands, mlir::DictionaryAttr attrs,
                                                            mlir::RegionRange /*regions*/,
                                                            mlir::SmallVectorImpl<mlir::Type>& inferredReturnTypes) {
    const auto loc = optLoc.value_or(mlir::UnknownLoc::get(ctx));

    VPU::ReduceL2OpAdaptor reduceL2(operands, attrs);
    if (mlir::failed(reduceL2.verify(loc))) {
        return mlir::failure();
    }

    const auto input = reduceL2.input();
    const auto keepDims = reduceL2.keep_dims();

    auto axesValue = parseIntArrayAttr<int64_t>(reduceL2.axes_value());

    return VPU::inferReduceReturnTypes(loc, input, keepDims, axesValue, inferredReturnTypes);
}

//
// fold
//

mlir::OpFoldResult vpux::VPU::ReduceL2Op::fold(ArrayRef<mlir::Attribute>) {
    if (input().getType() == output().getType()) {
        return input();
    }

    return nullptr;
}

//
// serialize
//

EMU::BlobWriter::SpecificTask vpux::VPU::ReduceL2Op::serialize(EMU::BlobWriter& writer) {
    EMU::BlobWriter::String type;
    type = writer.createString("l2");
    const auto axes = writer.createVector(parseIntArrayAttr<int64_t>(axes_value()));

    MVCNN::ReduceParamsBuilder builder(writer);
    builder.add_keep_dims(checked_cast<bool>(keep_dims()));
    builder.add_operation(type);
    builder.add_axes_value(axes);

    const auto paramsOff = builder.Finish();

    return writer.createUPALayerTask(*this, {paramsOff.Union(), MVCNN::SoftwareLayerParams_ReduceParams});
}
