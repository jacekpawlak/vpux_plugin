//
// Copyright (C) 2022-2023 Intel Corporation.
// SPDX-License-Identifier: Apache 2.0
//

// RUN: vpux-opt --split-input-file --init-compiler="vpu-arch=VPUX37XX" --convert-VPUIP-to-VPUMI37XX %s | FileCheck %s
//

module @Test {

IE.CNNNetwork
    entryPoint : @main
    inputsInfo : {
        IE.DataInfo "input" : tensor<1x1000xf16>
    }
    outputsInfo : {
        IE.DataInfo "softmax" : tensor<1x1000xf16>
    }

// Sub-module, which holds SW kernel declarations and optional implementations.
// Used to group those declarations for faster access.
module @VPU.SW {
    // The declaration should match C++ params structure in decomposed form.
    // `memref` will be translated to `MemRefData`, while raw scalars will be translated as is.
    func.func private @builtin_softmax(%input : memref<*xf16>, %output : memref<*xf16>, %axis : i64)
        attributes {
            VPU.kernel_code = "singleShaveSoftmax.cpp",
            VPU.kernel_entry = "singleShaveSoftmax"
        }
}

func.func @main(%1: memref<1x1x1x1000xf16>, %2: memref<1x1x1x1000xf16>) -> memref<1x1x1x1000xf16> {

    %in_tile0_cmx  = VPURT.DeclareBuffer <CMX_NN> [0] <0> -> memref<1x1x1x1000xf16, [@CMX_NN, 0]>
    %out_tile0_cmx = VPURT.DeclareBuffer <CMX_NN> [0] <2000> -> memref<1x1x1x1000xf16, [@CMX_NN, 0]>

    %b0 = VPURT.ConfigureBarrier<0> -> !VPURT.Barrier
    %b1 = VPURT.ConfigureBarrier<1> -> !VPURT.Barrier

    VPURT.Task updates(%b0 : !VPURT.Barrier) {
        VPUIP.NNDMA inputs(%1 : memref<1x1x1x1000xf16>) outputs(%in_tile0_cmx : memref<1x1x1x1000xf16, [@CMX_NN, 0]>) -> memref<1x1x1x1000xf16, [@CMX_NN, 0]>
    }

    // Genetic Kernel information for the scheduler.
    VPURT.Task  waits(%b0 : !VPURT.Barrier) updates(%b1 : !VPURT.Barrier) {
        VPUIP.SW.Kernel {result_segment_sizes = dense<[1, 0]> : vector<2xi32>}
                    @VPU.SW::@builtin_softmax            // The reference to the Kernel function.
                    inputs(%in_tile0_cmx as %arg0: memref<1x1x1x1000xf16, [@CMX_NN, 0]>)     // Inputs/outputs buffers for generic operation interface
                    outputs(%out_tile0_cmx as %arg1: memref<1x1x1x1000xf16, [@CMX_NN, 0]>)   // and their mapping to inner region.
                    on tile 0                           // The tile index to execute on.
        -> memref<1x1x1x1000xf16, [@CMX_NN, 0]> {

                // The arguments mapping, the order must match the kernel parameter structure.
                VPUIP.SW.Kernel.run {attrs = [0]}(%arg0, %arg1)
                    : memref<1x1x1x1000xf16, [@CMX_NN, 0]>
                    , memref<1x1x1x1000xf16, [@CMX_NN, 0]>
        }
    }

    VPURT.Task waits(%b1 : !VPURT.Barrier) {
        VPUIP.NNDMA inputs(%out_tile0_cmx : memref<1x1x1x1000xf16, [@CMX_NN, 0]>) outputs(%2 : memref<1x1x1x1000xf16>) -> memref<1x1x1x1000xf16>
    }
    return %2: memref<1x1x1x1000xf16>

}


}

//CHECK: %[[VAL0:.*]] = VPURT.DeclareBuffer <CMX_NN> [0] <0> -> memref<1x1x1x1000xf16, [@CMX_NN, 0]>
//CHECK-NEXT: %[[VAL1:.*]] = VPURT.DeclareBuffer <CMX_NN> [0] <2000> -> memref<1x1x1x1000xf16, [@CMX_NN, 0]>
//CHECK-NEXT: %[[VAL2:.*]] = VPUMI37XX.ConfigureBarrier {consumer_count = 1 : ui8, producer_count = 1 : ui8}<0, -1> -> !VPURegMapped.Index<0:0:0>
//CHECK-NEXT: %[[VAL3:.*]] = VPUMI37XX.ConfigureBarrier {consumer_count = 1 : ui8, producer_count = 1 : ui8}<1, -1> -> !VPURegMapped.Index<0:0:1>
//CHECK-NOT: VPURT.Task
//CHECK-NEXT: %[[VAL4:.*]] = VPUMI37XX.NNDMA {port = 0 : i64} inputs(%[[VAL5:.*]] : memref<1x1x1x1000xf16>) outputs(%[[VAL0]] : memref<1x1x1x1000xf16, [@CMX_NN, 0]>) updates(%[[VAL2]] : !VPURegMapped.Index<0:0:0>) start_after(0) clean_after(0) -> !VPURegMapped.Index<0:0:0>
//CHECK-NEXT: %[[VAL6:.*]] = VPUMI37XX.DeclareKernelText kernel_path([[VAL7:.*]]) -> !VPURegMapped.Index<0:0:0>
//CHECK-NEXT: %[[VAL8:.*]] = VPUMI37XX.DeclareKernelEntry kernel_path([[VAL7]]) -> !VPURegMapped.Index<0:0:0>
//CHECK-NEXT: %[[VAL9:.*]] = VPUMI37XX.DeclareKernelArgs kernel_path([[VAL7]]) -> !VPURegMapped.Index<0:0:0>
//CHECK-NEXT: %[[VAL10:.*]] = VPUMI37XX.ActKernelRange kernel_text_index(%[[VAL6]] : <0:0:0>) kernel_args_index(%[[VAL9]] : <0:0:0>) kernel_entry_index(%[[VAL8]] : <0:0:0>) -> !VPURegMapped.Index<0:0:0>
//CHECK-NEXT: %[[VAL11:.*]] = VPUMI37XX.ActKernelInvocation range_index(%[[VAL10]] : <0:0:0>) waits(%[[VAL2]] : !VPURegMapped.Index<0:0:0>) updates(%[[VAL3]] : !VPURegMapped.Index<0:0:1>) tile(0) start_after(0) clean_after(0) -> !VPURegMapped.Index<0:0:0>
//CHECK-NEXT: %[[VAL12:.*]] = VPUMI37XX.KernelParams inputs(%[[VAL0]] : memref<1x1x1x1000xf16, [@CMX_NN, 0]>) outputs(%[[VAL1]] : memref<1x1x1x1000xf16, [@CMX_NN, 0]>) kernel_type([[VAL7]]) kernel_params({{.*}}) -> !VPURegMapped.Index<0:0:0>
//CHECK-NOT: VPURT.Task
//CHECK-NEXT: %[[VAL13:.*]] = VPUMI37XX.NNDMA {port = 0 : i64} inputs(%[[VAL1]] : memref<1x1x1x1000xf16, [@CMX_NN, 0]>) outputs(%[[VAL14:.*]] : memref<1x1x1x1000xf16>) previousDMA(%[[VAL4]] : !VPURegMapped.Index<0:0:0>) waits(%[[VAL3]] : !VPURegMapped.Index<0:0:1>) start_after(0) clean_after(0) -> !VPURegMapped.Index<0:0:1>
