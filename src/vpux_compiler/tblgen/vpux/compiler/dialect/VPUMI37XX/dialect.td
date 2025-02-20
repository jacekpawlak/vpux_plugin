//
// Copyright (C) 2022 Intel Corporation.
// SPDX-License-Identifier: Apache 2.0
//

#ifndef VPUX_COMPILER_DIALECT_VPUMI37XX
#define VPUX_COMPILER_DIALECT_VPUMI37XX

include "mlir/IR/OpBase.td"

def VPUMI37XX_Dialect : Dialect {
    let summary = "VPU NN Mapped Inference RunTime Dialect for arch VPUX37XX";

    let description = [{
The **VPUMI37XX Dialect** represents NN RunTime IR together with MappedInference
    constructs in terms of the MLIR framework.
    This is specific for VPU37XX HW architecture.

[./VPUMI37XX/_ops_interfaces.md]
    }];

    let name = "VPUMI37XX";

    let cppNamespace = "vpux::VPUMI37XX";

    let dependentDialects = [
        "vpux::Const::ConstDialect",
        "mlir::func::FuncDialect",
        "mlir::quant::QuantizationDialect",
        "vpux::VPURegMapped::VPURegMappedDialect"
    ];

    let extraClassDeclaration = [{
        void registerTypes();
        void registerAttributes();
    }];

    let useDefaultAttributePrinterParser = 1;

    let emitAccessorPrefix = kEmitAccessorPrefix_Prefixed;
}

#endif
