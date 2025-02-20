//
// Copyright (C) 2022-2023 Intel Corporation.
// SPDX-License-Identifier: Apache 2.0
//

// RUN: vpux-opt --split-input-file --init-compiler="vpu-arch=VPUX37XX" --unroll-space-to-depth-dma  %s | FileCheck %s

#NHWC = affine_map<(d0, d1, d2, d3) -> (d0, d2, d3, d1)>

func.func @UnrollSpaceToDepthDMABlockFirstNHWC(%input: memref<1x2x4x6xf16, #NHWC>, %output: memref<1x8x2x3xf16, #NHWC>) -> memref<1x8x2x3xf16, #NHWC> {
    %bar0 = VPURT.DeclareVirtualBarrier -> !VPURT.Barrier
    %bar1 = VPURT.DeclareVirtualBarrier -> !VPURT.Barrier

    %inBuffer = VPURT.DeclareBuffer <CMX_NN> [0] <0> -> memref<1x2x4x6xf16, #NHWC, [@CMX_NN, 0]>
    %outBuffer = VPURT.DeclareBuffer <CMX_NN> [0] <128> -> memref<1x8x2x3xf16, #NHWC, [@CMX_NN, 0]>

    VPURT.Task updates(%bar0: !VPURT.Barrier)  {
        VPUIP.NNDMA {port = 0 : i64} inputs(%input : memref<1x2x4x6xf16, #NHWC>) outputs(%inBuffer : memref<1x2x4x6xf16, #NHWC, [@CMX_NN, 0]>) -> memref<1x2x4x6xf16, #NHWC, [@CMX_NN, 0]>
    }

    VPURT.Task waits(%bar0: !VPURT.Barrier) updates(%bar1: !VPURT.Barrier) {
        VPUIP.SpaceToDepthDMA {block_size = 2 : i64, mode = #IE.space_to_depth_mode<BLOCKS_FIRST>, output_channel = 2 : i64, output_width = 6 : i64, port = 0 : i64}
                inputs(%inBuffer : memref<1x2x4x6xf16, #NHWC, [@CMX_NN, 0]>)
                outputs(%outBuffer : memref<1x8x2x3xf16, #NHWC, [@CMX_NN, 0]>) -> memref<1x8x2x3xf16, #NHWC, [@CMX_NN, 0]>
    }

    VPURT.Task waits(%bar1: !VPURT.Barrier) {
        VPUIP.NNDMA {port = 0 : i64} inputs(%outBuffer : memref<1x8x2x3xf16, #NHWC, [@CMX_NN, 0]>) outputs(%output :  memref<1x8x2x3xf16, #NHWC>) -> memref<1x8x2x3xf16, #NHWC>
    }

    return %output: memref<1x8x2x3xf16, #NHWC>
    //CHECK:    [[BARRIER_0:%.*]] = VPURT.DeclareVirtualBarrier -> !VPURT.Barrier
    //CHECK:    [[BARRIER_1:%.*]] = VPURT.DeclareVirtualBarrier -> !VPURT.Barrier

    //CHECK-DAG:    [[INPUT_BUFFER:%.*]] = VPURT.DeclareBuffer <CMX_NN> [0] <0> -> memref<1x2x4x6xf16, #NHWC, [@CMX_NN, 0]>
    //CHECK-DAG:    [[INPUT_0:%.*]] = VPURT.DeclareBuffer <CMX_NN> [0] <0> -> memref<1x2x4x6xf16, #NHWC, [@CMX_NN, 0]>
    //CHECK-DAG:    [[OUTPUT_BUFFER:%.*]] = VPURT.DeclareBuffer <CMX_NN> [0] <128> -> memref<1x8x2x3xf16, #NHWC, [@CMX_NN, 0]>
    //CHECK-DAG:    [[OUTPUT_0:%.*]] = VPURT.DeclareBuffer <CMX_NN> [0] <128> -> memref<1x2x4x6xf16, #NHWC, [@CMX_NN, 0]>

    //CHECK:    VPURT.Task updates([[BARRIER_0]] : !VPURT.Barrier)  {
    //CHECK:        VPUIP.NNDMA {port = 0 : i64}
    //CHECK:            inputs(%arg0 : memref<1x2x4x6xf16, #NHWC>)
    //CHECK:            outputs([[INPUT_BUFFER]] : memref<1x2x4x6xf16, #NHWC, [@CMX_NN, 0]>) -> memref<1x2x4x6xf16, #NHWC, [@CMX_NN, 0]>
    //CHECK:    }

    //CHECK:    VPURT.Task waits([[BARRIER_0]] : !VPURT.Barrier) updates([[BARRIER_1]] : !VPURT.Barrier) attributes {isTrailingSWLayer = false} {
    //CHECK:        VPUIP.SpaceToDepthDMA {
    //CHECK-SAME:           block_size = 2 : i64,
    //CHECK-SAME:           dma_descriptor = #VPUIP.DMADescriptorAttr<numPlanes = 2 : i64, len = 48 : i64, srcWidth = 24 : i64, srcStride = 48 : i64, srcPlaneStride = 24 : i64, dstWidth = 8 : i64, dstStride = 16 : i64, dstPlaneStride = 8 : i64>,
    //CHECK-SAME:           mode = #IE.space_to_depth_mode<BLOCKS_FIRST>,
    //CHECK-SAME:           port = 0 : i64}
    //CHECK:            inputs([[INPUT_0]] : memref<1x2x4x6xf16, #NHWC, [@CMX_NN, 0]>)
    //CHECK:            outputs([[OUTPUT_0]] : memref<1x2x4x6xf16, #NHWC, [@CMX_NN, 0]>) -> memref<1x2x4x6xf16, #NHWC, [@CMX_NN, 0]>
    //CHECK:    }

    //CHECK:    VPURT.Task waits([[BARRIER_1]] : !VPURT.Barrier)  {
    //CHECK:        VPUIP.NNDMA {port = 0 : i64}
    //CHECK:            inputs([[OUTPUT_BUFFER]] : memref<1x8x2x3xf16, #NHWC, [@CMX_NN, 0]>)
    //CHECK:            outputs(%arg1 : memref<1x8x2x3xf16, #NHWC>) -> memref<1x8x2x3xf16, #NHWC>
    //CHECK:    }

    //CHECK:    return %arg1 : memref<1x8x2x3xf16, #NHWC>
}

func.func @UnrollSpaceToDepthDMABlockFirstNCHW(%input: memref<1x2x4x6xf16>, %output: memref<1x8x2x3xf16>) -> memref<1x8x2x3xf16> {
    %bar0 = VPURT.DeclareVirtualBarrier -> !VPURT.Barrier
    %bar1 = VPURT.DeclareVirtualBarrier -> !VPURT.Barrier

    %inBuffer = VPURT.DeclareBuffer <CMX_NN> [0] <0> -> memref<1x2x4x6xf16, [@CMX_NN, 0]>
    %outBuffer = VPURT.DeclareBuffer <CMX_NN> [0] <128> -> memref<1x8x2x3xf16, [@CMX_NN, 0]>

    VPURT.Task updates(%bar0: !VPURT.Barrier)  {
        VPUIP.NNDMA {port = 0 : i64} inputs(%input : memref<1x2x4x6xf16>) outputs(%inBuffer : memref<1x2x4x6xf16, [@CMX_NN, 0]>) -> memref<1x2x4x6xf16, [@CMX_NN, 0]>
    }

    VPURT.Task waits(%bar0: !VPURT.Barrier) updates(%bar1: !VPURT.Barrier) {
        VPUIP.SpaceToDepthDMA {block_size = 2 : i64, mode = #IE.space_to_depth_mode<BLOCKS_FIRST>, output_channel = 2 : i64, output_width = 6 : i64, port = 0 : i64}
                inputs(%inBuffer : memref<1x2x4x6xf16, [@CMX_NN, 0]>)
                outputs(%outBuffer : memref<1x8x2x3xf16, [@CMX_NN, 0]>) -> memref<1x8x2x3xf16, [@CMX_NN, 0]>
    }

    VPURT.Task waits(%bar1: !VPURT.Barrier) {
        VPUIP.NNDMA {port = 0 : i64} inputs(%outBuffer : memref<1x8x2x3xf16, [@CMX_NN, 0]>) outputs(%output :  memref<1x8x2x3xf16>) -> memref<1x8x2x3xf16>
    }

    return %output: memref<1x8x2x3xf16>
    //CHECK:    [[BARRIER_0:%.*]] = VPURT.DeclareVirtualBarrier -> !VPURT.Barrier
    //CHECK:    [[BARRIER_1:%.*]] = VPURT.DeclareVirtualBarrier -> !VPURT.Barrier

    //CHECK-DAG:    [[INPUT_BUFFER:%.*]] = VPURT.DeclareBuffer <CMX_NN> [0] <0> -> memref<1x2x4x6xf16, [@CMX_NN, 0]>
    //CHECK-DAG:    [[INPUT_3:%.*]] = VPURT.DeclareBuffer <CMX_NN> [0] <72> -> memref<1x1x2x6xf16, [@CMX_NN, 0]>
    //CHECK-DAG:    [[INPUT_2:%.*]] = VPURT.DeclareBuffer <CMX_NN> [0] <48> -> memref<1x1x2x6xf16, [@CMX_NN, 0]>
    //CHECK-DAG:    [[INPUT_1:%.*]] = VPURT.DeclareBuffer <CMX_NN> [0] <24> -> memref<1x1x2x6xf16, [@CMX_NN, 0]>
    //CHECK-DAG:    [[INPUT_0:%.*]] = VPURT.DeclareBuffer <CMX_NN> [0] <0> -> memref<1x1x2x6xf16, [@CMX_NN, 0]>
    //CHECK-DAG:    [[OUTPUT_BUFFER:%.*]] = VPURT.DeclareBuffer <CMX_NN> [0] <128> -> memref<1x8x2x3xf16, [@CMX_NN, 0]>
    //CHECK-DAG:    [[OUTPUT_3:%.*]] = VPURT.DeclareBuffer <CMX_NN> [0] <146> -> memref<1x1x2x6xf16, [@CMX_NN, 0]>
    //CHECK-DAG:    [[OUTPUT_2:%.*]] = VPURT.DeclareBuffer <CMX_NN> [0] <140> -> memref<1x1x2x6xf16, [@CMX_NN, 0]>
    //CHECK-DAG:    [[OUTPUT_1:%.*]] = VPURT.DeclareBuffer <CMX_NN> [0] <134> -> memref<1x1x2x6xf16, [@CMX_NN, 0]>
    //CHECK-DAG:    [[OUTPUT_0:%.*]] = VPURT.DeclareBuffer <CMX_NN> [0] <128> -> memref<1x1x2x6xf16, [@CMX_NN, 0]>

    //CHECK:    VPURT.Task updates([[BARRIER_0]] : !VPURT.Barrier)  {
    //CHECK:        VPUIP.NNDMA {port = 0 : i64}
    //CHECK:            inputs(%arg0 : memref<1x2x4x6xf16>)
    //CHECK:            outputs([[INPUT_BUFFER]] : memref<1x2x4x6xf16, [@CMX_NN, 0]>) -> memref<1x2x4x6xf16, [@CMX_NN, 0]>
    //CHECK:    }

    //CHECK:    VPURT.Task waits([[BARRIER_0]] : !VPURT.Barrier) updates([[BARRIER_1]] : !VPURT.Barrier) attributes {isTrailingSWLayer = false} {
    //CHECK:        VPUIP.SpaceToDepthDMA {
    //CHECK-SAME:           block_size = 2 : i64,
    //CHECK-SAME:           dma_descriptor = #VPUIP.DMADescriptorAttr<numPlanes = 2 : i64, len = 12 : i64, srcWidth = 2 : i64, srcStride = 4 : i64, srcPlaneStride = 2 : i64, dstWidth = 6 : i64, dstStride = 48 : i64, dstPlaneStride = 24 : i64>,
    //CHECK-SAME:           mode = #IE.space_to_depth_mode<BLOCKS_FIRST>,
    //CHECK-SAME:           port = 0 : i64}
    //CHECK:            inputs([[INPUT_0]] : memref<1x1x2x6xf16, [@CMX_NN, 0]>)
    //CHECK:            outputs([[OUTPUT_0]] : memref<1x1x2x6xf16, [@CMX_NN, 0]>) -> memref<1x1x2x6xf16, [@CMX_NN, 0]>
    //CHECK:    }

    //CHECK:    VPURT.Task waits([[BARRIER_0]] : !VPURT.Barrier) updates([[BARRIER_1]] : !VPURT.Barrier) attributes {isTrailingSWLayer = false} {
    //CHECK:        VPUIP.SpaceToDepthDMA {
    //CHECK-SAME:           block_size = 2 : i64,
    //CHECK-SAME:           dma_descriptor = #VPUIP.DMADescriptorAttr<numPlanes = 2 : i64, len = 12 : i64, srcWidth = 2 : i64, srcStride = 4 : i64, srcPlaneStride = 2 : i64, dstWidth = 6 : i64, dstStride = 48 : i64, dstPlaneStride = 24 : i64>,
    //CHECK-SAME:           mode = #IE.space_to_depth_mode<BLOCKS_FIRST>,
    //CHECK-SAME:           port = 1 : i64}
    //CHECK:            inputs([[INPUT_1]] : memref<1x1x2x6xf16, [@CMX_NN, 0]>)
    //CHECK:            outputs([[OUTPUT_1]] : memref<1x1x2x6xf16, [@CMX_NN, 0]>) -> memref<1x1x2x6xf16, [@CMX_NN, 0]>
    //CHECK:    }

    //CHECK:    VPURT.Task waits([[BARRIER_0]] : !VPURT.Barrier) updates([[BARRIER_1]] : !VPURT.Barrier) attributes {isTrailingSWLayer = false} {
    //CHECK:        VPUIP.SpaceToDepthDMA {
    //CHECK-SAME:           block_size = 2 : i64,
    //CHECK-SAME:           dma_descriptor = #VPUIP.DMADescriptorAttr<numPlanes = 2 : i64, len = 12 : i64, srcWidth = 2 : i64, srcStride = 4 : i64, srcPlaneStride = 2 : i64, dstWidth = 6 : i64, dstStride = 48 : i64, dstPlaneStride = 24 : i64>,
    //CHECK-SAME:           mode = #IE.space_to_depth_mode<BLOCKS_FIRST>,
    //CHECK-SAME:           port = 0 : i64}
    //CHECK:            inputs([[INPUT_2]] : memref<1x1x2x6xf16, [@CMX_NN, 0]>)
    //CHECK:            outputs([[OUTPUT_2]] : memref<1x1x2x6xf16, [@CMX_NN, 0]>) -> memref<1x1x2x6xf16, [@CMX_NN, 0]>
    //CHECK:    }

    //CHECK:    VPURT.Task waits([[BARRIER_0]] : !VPURT.Barrier) updates([[BARRIER_1]] : !VPURT.Barrier) attributes {isTrailingSWLayer = false} {
    //CHECK:        VPUIP.SpaceToDepthDMA {
    //CHECK-SAME:           block_size = 2 : i64,
    //CHECK-SAME:           dma_descriptor = #VPUIP.DMADescriptorAttr<numPlanes = 2 : i64, len = 12 : i64, srcWidth = 2 : i64, srcStride = 4 : i64, srcPlaneStride = 2 : i64, dstWidth = 6 : i64, dstStride = 48 : i64, dstPlaneStride = 24 : i64>,
    //CHECK-SAME:           mode = #IE.space_to_depth_mode<BLOCKS_FIRST>,
    //CHECK-SAME:           port = 1 : i64}
    //CHECK:            inputs([[INPUT_3]] : memref<1x1x2x6xf16, [@CMX_NN, 0]>)
    //CHECK:            outputs([[OUTPUT_3]] : memref<1x1x2x6xf16, [@CMX_NN, 0]>) -> memref<1x1x2x6xf16, [@CMX_NN, 0]>
    //CHECK:    }

    //CHECK:    VPURT.Task waits([[BARRIER_1]] : !VPURT.Barrier)  {
    //CHECK:        VPUIP.NNDMA {port = 0 : i64}
    //CHECK:            inputs([[OUTPUT_BUFFER]] : memref<1x8x2x3xf16, [@CMX_NN, 0]>)
    //CHECK:            outputs(%arg1 : memref<1x8x2x3xf16>) -> memref<1x8x2x3xf16>
    //CHECK:    }

    //CHECK:    return %arg1 : memref<1x8x2x3xf16>
}

func.func @UnrollSpaceToDepthDMADepthFirstNHWC(%input: memref<1x2x4x6xf16, #NHWC>, %output: memref<1x8x2x3xf16, #NHWC>) -> memref<1x8x2x3xf16, #NHWC> {
    %bar0 = VPURT.DeclareVirtualBarrier -> !VPURT.Barrier
    %bar1 = VPURT.DeclareVirtualBarrier -> !VPURT.Barrier

    %inBuffer = VPURT.DeclareBuffer <CMX_NN> [0] <0> -> memref<1x2x4x6xf16, #NHWC, [@CMX_NN, 0]>
    %outBuffer = VPURT.DeclareBuffer <CMX_NN> [0] <128> -> memref<1x8x2x3xf16, #NHWC, [@CMX_NN, 0]>

    VPURT.Task updates(%bar0: !VPURT.Barrier)  {
        VPUIP.NNDMA {port = 0 : i64} inputs(%input : memref<1x2x4x6xf16, #NHWC>) outputs(%inBuffer : memref<1x2x4x6xf16, #NHWC, [@CMX_NN, 0]>) -> memref<1x2x4x6xf16, #NHWC, [@CMX_NN, 0]>
    }

    VPURT.Task waits(%bar0: !VPURT.Barrier) updates(%bar1: !VPURT.Barrier) {
        VPUIP.SpaceToDepthDMA {block_size = 2 : i64, mode = #IE.space_to_depth_mode<DEPTH_FIRST>, output_channel = 2 : i64, output_width = 6 : i64, port = 0 : i64}
                inputs(%inBuffer : memref<1x2x4x6xf16, #NHWC, [@CMX_NN, 0]>)
                outputs(%outBuffer : memref<1x8x2x3xf16, #NHWC, [@CMX_NN, 0]>) -> memref<1x8x2x3xf16, #NHWC, [@CMX_NN, 0]>
    }

    VPURT.Task waits(%bar1: !VPURT.Barrier) {
        VPUIP.NNDMA {port = 0 : i64} inputs(%outBuffer : memref<1x8x2x3xf16, #NHWC, [@CMX_NN, 0]>) outputs(%output :  memref<1x8x2x3xf16, #NHWC>) -> memref<1x8x2x3xf16, #NHWC>
    }

    return %output: memref<1x8x2x3xf16, #NHWC>
    //CHECK:    [[BARRIER_0:%.*]] = VPURT.DeclareVirtualBarrier -> !VPURT.Barrier
    //CHECK:    [[BARRIER_1:%.*]] = VPURT.DeclareVirtualBarrier -> !VPURT.Barrier

    //CHECK-DAG:    [[INPUT_BUFFER:%.*]] = VPURT.DeclareBuffer <CMX_NN> [0] <0> -> memref<1x2x4x6xf16, #NHWC, [@CMX_NN, 0]>
    //CHECK-DAG:    [[INPUT_3:%.*]] = VPURT.DeclareBuffer <CMX_NN> [0] <72> -> memref<1x2x1x6xf16, #NHWC, [@CMX_NN, 0]>
    //CHECK-DAG:    [[INPUT_2:%.*]] = VPURT.DeclareBuffer <CMX_NN> [0] <48> -> memref<1x2x1x6xf16, #NHWC, [@CMX_NN, 0]>
    //CHECK-DAG:    [[INPUT_1:%.*]] = VPURT.DeclareBuffer <CMX_NN> [0] <24> -> memref<1x2x1x6xf16, #NHWC, [@CMX_NN, 0]>
    //CHECK-DAG:    [[INPUT_0:%.*]] = VPURT.DeclareBuffer <CMX_NN> [0] <0> -> memref<1x2x1x6xf16, #NHWC, [@CMX_NN, 0]>
    //CHECK-DAG:    [[OUTPUT_BUFFER:%.*]] = VPURT.DeclareBuffer <CMX_NN> [0] <128> -> memref<1x8x2x3xf16, #NHWC, [@CMX_NN, 0]>
    //CHECK-DAG:    [[OUTPUT_3:%.*]] = VPURT.DeclareBuffer <CMX_NN> [0] <180> -> memref<1x2x1x6xf16, #NHWC, [@CMX_NN, 0]>
    //CHECK-DAG:    [[OUTPUT_2:%.*]] = VPURT.DeclareBuffer <CMX_NN> [0] <176> -> memref<1x2x1x6xf16, #NHWC, [@CMX_NN, 0]>
    //CHECK-DAG:    [[OUTPUT_1:%.*]] = VPURT.DeclareBuffer <CMX_NN> [0] <132> -> memref<1x2x1x6xf16, #NHWC, [@CMX_NN, 0]>
    //CHECK-DAG:    [[OUTPUT_0:%.*]] = VPURT.DeclareBuffer <CMX_NN> [0] <128> -> memref<1x2x1x6xf16, #NHWC, [@CMX_NN, 0]>


    //CHECK:    VPURT.Task updates([[BARRIER_0]] : !VPURT.Barrier)  {
    //CHECK:        VPUIP.NNDMA {port = 0 : i64}
    //CHECK:            inputs(%arg0 : memref<1x2x4x6xf16, #NHWC>)
    //CHECK:            outputs([[INPUT_BUFFER]] : memref<1x2x4x6xf16, #NHWC, [@CMX_NN, 0]>) -> memref<1x2x4x6xf16, #NHWC, [@CMX_NN, 0]>
    //CHECK:    }

    //CHECK:    VPURT.Task waits([[BARRIER_0]] : !VPURT.Barrier) updates([[BARRIER_1]] : !VPURT.Barrier) attributes {isTrailingSWLayer = false} {
    //CHECK:        VPUIP.SpaceToDepthDMA {
    //CHECK-SAME:           block_size = 2 : i64,
    //CHECK-SAME:           dma_descriptor = #VPUIP.DMADescriptorAttr<numPlanes = 2 : i64, len = 12 : i64, srcWidth = 2 : i64, srcStride = 4 : i64, srcPlaneStride = 2 : i64, dstWidth = 4 : i64, dstStride = 16 : i64, dstPlaneStride = 8 : i64>,
    //CHECK-SAME:           mode = #IE.space_to_depth_mode<DEPTH_FIRST>,
    //CHECK-SAME:           port = 0 : i64}
    //CHECK:            inputs([[INPUT_0]] : memref<1x2x1x6xf16, #NHWC, [@CMX_NN, 0]>)
    //CHECK:            outputs([[OUTPUT_0]] : memref<1x2x1x6xf16, #NHWC, [@CMX_NN, 0]>) -> memref<1x2x1x6xf16, #NHWC, [@CMX_NN, 0]>
    //CHECK:    }

    //CHECK:    VPURT.Task waits([[BARRIER_0]] : !VPURT.Barrier) updates([[BARRIER_1]] : !VPURT.Barrier) attributes {isTrailingSWLayer = false} {
    //CHECK:        VPUIP.SpaceToDepthDMA {
    //CHECK-SAME:           block_size = 2 : i64,
    //CHECK-SAME:           dma_descriptor = #VPUIP.DMADescriptorAttr<numPlanes = 2 : i64, len = 12 : i64, srcWidth = 2 : i64, srcStride = 4 : i64, srcPlaneStride = 2 : i64, dstWidth = 4 : i64, dstStride = 16 : i64, dstPlaneStride = 8 : i64>,
    //CHECK-SAME:           mode = #IE.space_to_depth_mode<DEPTH_FIRST>,
    //CHECK-SAME:           port = 1 : i64}
    //CHECK:            inputs([[INPUT_1]] : memref<1x2x1x6xf16, #NHWC, [@CMX_NN, 0]>)
    //CHECK:            outputs([[OUTPUT_1]] : memref<1x2x1x6xf16, #NHWC, [@CMX_NN, 0]>) -> memref<1x2x1x6xf16, #NHWC, [@CMX_NN, 0]>
    //CHECK:    }

    //CHECK:    VPURT.Task waits([[BARRIER_0]] : !VPURT.Barrier) updates([[BARRIER_1]] : !VPURT.Barrier) attributes {isTrailingSWLayer = false} {
    //CHECK:        VPUIP.SpaceToDepthDMA {
    //CHECK-SAME:           block_size = 2 : i64,
    //CHECK-SAME:           dma_descriptor = #VPUIP.DMADescriptorAttr<numPlanes = 2 : i64, len = 12 : i64, srcWidth = 2 : i64, srcStride = 4 : i64, srcPlaneStride = 2 : i64, dstWidth = 4 : i64, dstStride = 16 : i64, dstPlaneStride = 8 : i64>,
    //CHECK-SAME:           mode = #IE.space_to_depth_mode<DEPTH_FIRST>,
    //CHECK-SAME:           port = 0 : i64}
    //CHECK:            inputs([[INPUT_2]] : memref<1x2x1x6xf16, #NHWC, [@CMX_NN, 0]>)
    //CHECK:            outputs([[OUTPUT_2]] : memref<1x2x1x6xf16, #NHWC, [@CMX_NN, 0]>) -> memref<1x2x1x6xf16, #NHWC, [@CMX_NN, 0]>
    //CHECK:    }

    //CHECK:    VPURT.Task waits([[BARRIER_0]] : !VPURT.Barrier) updates([[BARRIER_1]] : !VPURT.Barrier) attributes {isTrailingSWLayer = false} {
    //CHECK:        VPUIP.SpaceToDepthDMA {
    //CHECK-SAME:           block_size = 2 : i64,
    //CHECK-SAME:           dma_descriptor = #VPUIP.DMADescriptorAttr<numPlanes = 2 : i64, len = 12 : i64, srcWidth = 2 : i64, srcStride = 4 : i64, srcPlaneStride = 2 : i64, dstWidth = 4 : i64, dstStride = 16 : i64, dstPlaneStride = 8 : i64>,
    //CHECK-SAME:           mode = #IE.space_to_depth_mode<DEPTH_FIRST>,
    //CHECK-SAME:           port = 1 : i64}
    //CHECK:            inputs([[INPUT_3]] : memref<1x2x1x6xf16, #NHWC, [@CMX_NN, 0]>)
    //CHECK:            outputs([[OUTPUT_3]] : memref<1x2x1x6xf16, #NHWC, [@CMX_NN, 0]>) -> memref<1x2x1x6xf16, #NHWC, [@CMX_NN, 0]>
    //CHECK:    }

    //CHECK:    VPURT.Task waits([[BARRIER_1]] : !VPURT.Barrier)  {
    //CHECK:        VPUIP.NNDMA {port = 0 : i64}
    //CHECK:            inputs([[OUTPUT_BUFFER]] : memref<1x8x2x3xf16, #NHWC, [@CMX_NN, 0]>)
    //CHECK:            outputs(%arg1 : memref<1x8x2x3xf16, #NHWC>) -> memref<1x8x2x3xf16, #NHWC>
    //CHECK:    }

    //CHECK:    return %arg1 : memref<1x8x2x3xf16, #NHWC>
}

func.func @UnrollSpaceToDepthDMADepthFirstNCHW(%input: memref<1x2x4x6xf16>, %output: memref<1x8x2x3xf16>) -> memref<1x8x2x3xf16> {
    %bar0 = VPURT.DeclareVirtualBarrier -> !VPURT.Barrier
    %bar1 = VPURT.DeclareVirtualBarrier -> !VPURT.Barrier

    %inBuffer = VPURT.DeclareBuffer <CMX_NN> [0] <0> -> memref<1x2x4x6xf16, [@CMX_NN, 0]>
    %outBuffer = VPURT.DeclareBuffer <CMX_NN> [0] <128> -> memref<1x8x2x3xf16, [@CMX_NN, 0]>

    VPURT.Task updates(%bar0: !VPURT.Barrier)  {
        VPUIP.NNDMA {port = 0 : i64} inputs(%input : memref<1x2x4x6xf16>) outputs(%inBuffer : memref<1x2x4x6xf16, [@CMX_NN, 0]>) -> memref<1x2x4x6xf16, [@CMX_NN, 0]>
    }

    VPURT.Task waits(%bar0: !VPURT.Barrier) updates(%bar1: !VPURT.Barrier) {
        VPUIP.SpaceToDepthDMA {block_size = 2 : i64, mode = #IE.space_to_depth_mode<DEPTH_FIRST>, output_channel = 2 : i64, output_width = 6 : i64, port = 0 : i64}
                inputs(%inBuffer : memref<1x2x4x6xf16, [@CMX_NN, 0]>)
                outputs(%outBuffer : memref<1x8x2x3xf16, [@CMX_NN, 0]>) -> memref<1x8x2x3xf16, [@CMX_NN, 0]>
    }

    VPURT.Task waits(%bar1: !VPURT.Barrier) {
        VPUIP.NNDMA {port = 0 : i64} inputs(%outBuffer : memref<1x8x2x3xf16, [@CMX_NN, 0]>) outputs(%output :  memref<1x8x2x3xf16>) -> memref<1x8x2x3xf16>
    }

    return %output: memref<1x8x2x3xf16>
    //CHECK:    [[BARRIER_0:%.*]] = VPURT.DeclareVirtualBarrier -> !VPURT.Barrier
    //CHECK:    [[BARRIER_1:%.*]] = VPURT.DeclareVirtualBarrier -> !VPURT.Barrier

    //CHECK-DAG:    [[INPUT_BUFFER:%.*]] = VPURT.DeclareBuffer <CMX_NN> [0] <0> -> memref<1x2x4x6xf16, [@CMX_NN, 0]>
    //CHECK-DAG:    [[INPUT_3:%.*]] = VPURT.DeclareBuffer <CMX_NN> [0] <72> -> memref<1x1x2x6xf16, [@CMX_NN, 0]>
    //CHECK-DAG:    [[INPUT_2:%.*]] = VPURT.DeclareBuffer <CMX_NN> [0] <48> -> memref<1x1x2x6xf16, [@CMX_NN, 0]>
    //CHECK-DAG:    [[INPUT_1:%.*]] = VPURT.DeclareBuffer <CMX_NN> [0] <24> -> memref<1x1x2x6xf16, [@CMX_NN, 0]>
    //CHECK-DAG:    [[INPUT_0:%.*]] = VPURT.DeclareBuffer <CMX_NN> [0] <0> -> memref<1x1x2x6xf16, [@CMX_NN, 0]>
    //CHECK-DAG:    [[OUTPUT_BUFFER:%.*]] = VPURT.DeclareBuffer <CMX_NN> [0] <128> -> memref<1x8x2x3xf16, [@CMX_NN, 0]>
    //CHECK-DAG:    [[OUTPUT_3:%.*]] = VPURT.DeclareBuffer <CMX_NN> [0] <182> -> memref<1x1x2x6xf16, [@CMX_NN, 0]>
    //CHECK-DAG:    [[OUTPUT_2:%.*]] = VPURT.DeclareBuffer <CMX_NN> [0] <176> -> memref<1x1x2x6xf16, [@CMX_NN, 0]>
    //CHECK-DAG:    [[OUTPUT_1:%.*]] = VPURT.DeclareBuffer <CMX_NN> [0] <134> -> memref<1x1x2x6xf16, [@CMX_NN, 0]>
    //CHECK-DAG:    [[OUTPUT_0:%.*]] = VPURT.DeclareBuffer <CMX_NN> [0] <128> -> memref<1x1x2x6xf16, [@CMX_NN, 0]>

    //CHECK:    VPURT.Task updates([[BARRIER_0]] : !VPURT.Barrier)  {
    //CHECK:        VPUIP.NNDMA {port = 0 : i64}
    //CHECK:            inputs(%arg0 : memref<1x2x4x6xf16>)
    //CHECK:            outputs([[INPUT_BUFFER]] : memref<1x2x4x6xf16, [@CMX_NN, 0]>) -> memref<1x2x4x6xf16, [@CMX_NN, 0]>
    //CHECK:    }

    //CHECK:    VPURT.Task waits([[BARRIER_0]] : !VPURT.Barrier) updates([[BARRIER_1]] : !VPURT.Barrier) attributes {isTrailingSWLayer = false} {
    //CHECK:        VPUIP.SpaceToDepthDMA {
    //CHECK-SAME:           block_size = 2 : i64,
    //CHECK-SAME:           dma_descriptor = #VPUIP.DMADescriptorAttr<numPlanes = 2 : i64, len = 12 : i64, srcWidth = 2 : i64, srcStride = 4 : i64, srcPlaneStride = 2 : i64, dstWidth = 6 : i64, dstStride = 24 : i64, dstPlaneStride = 12 : i64>,
    //CHECK-SAME:           mode = #IE.space_to_depth_mode<DEPTH_FIRST>,
    //CHECK-SAME:           port = 0 : i64}
    //CHECK:            inputs([[INPUT_0]] : memref<1x1x2x6xf16, [@CMX_NN, 0]>)
    //CHECK:            outputs([[OUTPUT_0]] : memref<1x1x2x6xf16, [@CMX_NN, 0]>) -> memref<1x1x2x6xf16, [@CMX_NN, 0]>
    //CHECK:    }

    //CHECK:    VPURT.Task waits([[BARRIER_0]] : !VPURT.Barrier) updates([[BARRIER_1]] : !VPURT.Barrier) attributes {isTrailingSWLayer = false} {
    //CHECK:        VPUIP.SpaceToDepthDMA {
    //CHECK-SAME:           block_size = 2 : i64,
    //CHECK-SAME:           dma_descriptor = #VPUIP.DMADescriptorAttr<numPlanes = 2 : i64, len = 12 : i64, srcWidth = 2 : i64, srcStride = 4 : i64, srcPlaneStride = 2 : i64, dstWidth = 6 : i64, dstStride = 24 : i64, dstPlaneStride = 12 : i64>,
    //CHECK-SAME:           mode = #IE.space_to_depth_mode<DEPTH_FIRST>,
    //CHECK-SAME:           port = 1 : i64}
    //CHECK:            inputs([[INPUT_1]] : memref<1x1x2x6xf16, [@CMX_NN, 0]>)
    //CHECK:            outputs([[OUTPUT_1]] : memref<1x1x2x6xf16, [@CMX_NN, 0]>) -> memref<1x1x2x6xf16, [@CMX_NN, 0]>
    //CHECK:    }

    //CHECK:    VPURT.Task waits([[BARRIER_0]] : !VPURT.Barrier) updates([[BARRIER_1]] : !VPURT.Barrier) attributes {isTrailingSWLayer = false} {
    //CHECK:        VPUIP.SpaceToDepthDMA {
    //CHECK-SAME:           block_size = 2 : i64,
    //CHECK-SAME:           dma_descriptor = #VPUIP.DMADescriptorAttr<numPlanes = 2 : i64, len = 12 : i64, srcWidth = 2 : i64, srcStride = 4 : i64, srcPlaneStride = 2 : i64, dstWidth = 6 : i64, dstStride = 24 : i64, dstPlaneStride = 12 : i64>,
    //CHECK-SAME:           mode = #IE.space_to_depth_mode<DEPTH_FIRST>,
    //CHECK-SAME:           port = 0 : i64}
    //CHECK:            inputs([[INPUT_2]] : memref<1x1x2x6xf16, [@CMX_NN, 0]>)
    //CHECK:            outputs([[OUTPUT_2]] : memref<1x1x2x6xf16, [@CMX_NN, 0]>) -> memref<1x1x2x6xf16, [@CMX_NN, 0]>
    //CHECK:    }

    //CHECK:    VPURT.Task waits([[BARRIER_0]] : !VPURT.Barrier) updates([[BARRIER_1]] : !VPURT.Barrier) attributes {isTrailingSWLayer = false} {
    //CHECK:        VPUIP.SpaceToDepthDMA {
    //CHECK-SAME:           block_size = 2 : i64,
    //CHECK-SAME:           dma_descriptor = #VPUIP.DMADescriptorAttr<numPlanes = 2 : i64, len = 12 : i64, srcWidth = 2 : i64, srcStride = 4 : i64, srcPlaneStride = 2 : i64, dstWidth = 6 : i64, dstStride = 24 : i64, dstPlaneStride = 12 : i64>,
    //CHECK-SAME:           mode = #IE.space_to_depth_mode<DEPTH_FIRST>,
    //CHECK-SAME:           port = 1 : i64}
    //CHECK:            inputs([[INPUT_3]] : memref<1x1x2x6xf16, [@CMX_NN, 0]>)
    //CHECK:            outputs([[OUTPUT_3]] : memref<1x1x2x6xf16, [@CMX_NN, 0]>) -> memref<1x1x2x6xf16, [@CMX_NN, 0]>
    //CHECK:    }

    //CHECK:    VPURT.Task waits([[BARRIER_1]] : !VPURT.Barrier)  {
    //CHECK:        VPUIP.NNDMA {port = 0 : i64}
    //CHECK:            inputs([[OUTPUT_BUFFER]] : memref<1x8x2x3xf16, [@CMX_NN, 0]>)
    //CHECK:            outputs(%arg1 : memref<1x8x2x3xf16>) -> memref<1x8x2x3xf16>
    //CHECK:    }

    //CHECK:    return %arg1 : memref<1x8x2x3xf16>
}

func.func @UnrollSpaceToDepthDMABlockFirstNCHWToNHWC(%input: memref<1x2x4x6xf16>, %output: memref<1x8x2x3xf16, #NHWC>) -> memref<1x8x2x3xf16, #NHWC> {
    %bar0 = VPURT.DeclareVirtualBarrier -> !VPURT.Barrier
    %bar1 = VPURT.DeclareVirtualBarrier -> !VPURT.Barrier

    %inBuffer = VPURT.DeclareBuffer <CMX_NN> [0] <0> -> memref<1x2x4x6xf16, [@CMX_NN, 0]>
    %outBuffer = VPURT.DeclareBuffer <CMX_NN> [0] <128> -> memref<1x8x2x3xf16, #NHWC, [@CMX_NN, 0]>

    VPURT.Task updates(%bar0: !VPURT.Barrier)  {
        VPUIP.NNDMA {port = 0 : i64} inputs(%input : memref<1x2x4x6xf16>) outputs(%inBuffer : memref<1x2x4x6xf16, [@CMX_NN, 0]>) -> memref<1x2x4x6xf16, [@CMX_NN, 0]>
    }

    VPURT.Task waits(%bar0: !VPURT.Barrier) updates(%bar1: !VPURT.Barrier) {
        VPUIP.SpaceToDepthDMA {block_size = 2 : i64, mode = #IE.space_to_depth_mode<BLOCKS_FIRST>, output_channel = 2 : i64, output_width = 6 : i64, port = 0 : i64}
                inputs(%inBuffer : memref<1x2x4x6xf16, [@CMX_NN, 0]>)
                outputs(%outBuffer : memref<1x8x2x3xf16, #NHWC, [@CMX_NN, 0]>) -> memref<1x8x2x3xf16, #NHWC, [@CMX_NN, 0]>
    }

    VPURT.Task waits(%bar1: !VPURT.Barrier) {
        VPUIP.NNDMA {port = 0 : i64} inputs(%outBuffer : memref<1x8x2x3xf16, #NHWC, [@CMX_NN, 0]>) outputs(%output :  memref<1x8x2x3xf16, #NHWC>) -> memref<1x8x2x3xf16, #NHWC>
    }

    return %output: memref<1x8x2x3xf16, #NHWC>
    //CHECK:    [[BARRIER_0:%.*]] = VPURT.DeclareVirtualBarrier -> !VPURT.Barrier
    //CHECK:    [[BARRIER_1:%.*]] = VPURT.DeclareVirtualBarrier -> !VPURT.Barrier

    //CHECK-DAG:    [[INPUT_BUFFER:%.*]] = VPURT.DeclareBuffer <CMX_NN> [0] <0> -> memref<1x2x4x6xf16, [@CMX_NN, 0]>
    //CHECK-DAG:    [[INPUT_3:%.*]] = VPURT.DeclareBuffer <CMX_NN> [0] <72> -> memref<1x1x2x6xf16, [@CMX_NN, 0]>
    //CHECK-DAG:    [[INPUT_2:%.*]] = VPURT.DeclareBuffer <CMX_NN> [0] <48> -> memref<1x1x2x6xf16, [@CMX_NN, 0]>
    //CHECK-DAG:    [[INPUT_1:%.*]] = VPURT.DeclareBuffer <CMX_NN> [0] <24> -> memref<1x1x2x6xf16, [@CMX_NN, 0]>
    //CHECK-DAG:    [[INPUT_0:%.*]] = VPURT.DeclareBuffer <CMX_NN> [0] <0> -> memref<1x1x2x6xf16, [@CMX_NN, 0]>
    //CHECK-DAG:    [[OUTPUT_BUFFER:%.*]] = VPURT.DeclareBuffer <CMX_NN> [0] <128> -> memref<1x8x2x3xf16, #NHWC, [@CMX_NN, 0]>
    //CHECK-DAG:    [[OUTPUT_3:%.*]] = VPURT.DeclareBuffer <CMX_NN> [0] <178> -> memref<1x1x2x6xf16, #NHWC, [@CMX_NN, 0]>
    //CHECK-DAG:    [[OUTPUT_2:%.*]] = VPURT.DeclareBuffer <CMX_NN> [0] <130> -> memref<1x1x2x6xf16, #NHWC, [@CMX_NN, 0]>
    //CHECK-DAG:    [[OUTPUT_1:%.*]] = VPURT.DeclareBuffer <CMX_NN> [0] <176> -> memref<1x1x2x6xf16, #NHWC, [@CMX_NN, 0]>
    //CHECK-DAG:    [[OUTPUT_0:%.*]] = VPURT.DeclareBuffer <CMX_NN> [0] <128> -> memref<1x1x2x6xf16, #NHWC, [@CMX_NN, 0]>


    //CHECK:    VPURT.Task updates([[BARRIER_0]] : !VPURT.Barrier)  {
    //CHECK:        VPUIP.NNDMA {port = 0 : i64}
    //CHECK:            inputs(%arg0 : memref<1x2x4x6xf16>)
    //CHECK:            outputs([[INPUT_BUFFER]] : memref<1x2x4x6xf16, [@CMX_NN, 0]>) -> memref<1x2x4x6xf16, [@CMX_NN, 0]>
    //CHECK:    }

    //CHECK:    VPURT.Task waits([[BARRIER_0]] : !VPURT.Barrier) updates([[BARRIER_1]] : !VPURT.Barrier) attributes {isTrailingSWLayer = false} {
    //CHECK:        VPUIP.SpaceToDepthDMA {
    //CHECK-SAME:           block_size = 2 : i64,
    //CHECK-SAME:           dma_descriptor = #VPUIP.DMADescriptorAttr<numPlanes = 3 : i64, len = 8 : i64, srcWidth = 4 : i64, srcStride = 12 : i64, srcPlaneStride = 4 : i64, dstWidth = 2 : i64, dstStride = 4 : i64, dstPlaneStride = 16 : i64>,
    //CHECK-SAME:           mode = #IE.space_to_depth_mode<BLOCKS_FIRST>,
    //CHECK-SAME:           port = 0 : i64}
    //CHECK:            inputs([[INPUT_0]] : memref<1x1x2x6xf16, [@CMX_NN, 0]>)
    //CHECK:            outputs([[OUTPUT_0]] : memref<1x1x2x6xf16, #NHWC, [@CMX_NN, 0]>) -> memref<1x1x2x6xf16, #NHWC, [@CMX_NN, 0]>
    //CHECK:    }

    //CHECK:    VPURT.Task waits([[BARRIER_0]] : !VPURT.Barrier) updates([[BARRIER_1]] : !VPURT.Barrier) attributes {isTrailingSWLayer = false} {
    //CHECK:        VPUIP.SpaceToDepthDMA {
    //CHECK-SAME:           block_size = 2 : i64,
    //CHECK-SAME:           dma_descriptor = #VPUIP.DMADescriptorAttr<numPlanes = 3 : i64, len = 8 : i64, srcWidth = 4 : i64, srcStride = 12 : i64, srcPlaneStride = 4 : i64, dstWidth = 2 : i64, dstStride = 4 : i64, dstPlaneStride = 16 : i64>,
    //CHECK-SAME:           mode = #IE.space_to_depth_mode<BLOCKS_FIRST>,
    //CHECK-SAME:           port = 1 : i64}
    //CHECK:            inputs([[INPUT_1]] : memref<1x1x2x6xf16, [@CMX_NN, 0]>)
    //CHECK:            outputs([[OUTPUT_1]] : memref<1x1x2x6xf16, #NHWC, [@CMX_NN, 0]>) -> memref<1x1x2x6xf16, #NHWC, [@CMX_NN, 0]>
    //CHECK:    }

    //CHECK:    VPURT.Task waits([[BARRIER_0]] : !VPURT.Barrier) updates([[BARRIER_1]] : !VPURT.Barrier) attributes {isTrailingSWLayer = false} {
    //CHECK:        VPUIP.SpaceToDepthDMA {
    //CHECK-SAME:           block_size = 2 : i64,
    //CHECK-SAME:           dma_descriptor = #VPUIP.DMADescriptorAttr<numPlanes = 3 : i64, len = 8 : i64, srcWidth = 4 : i64, srcStride = 12 : i64, srcPlaneStride = 4 : i64, dstWidth = 2 : i64, dstStride = 4 : i64, dstPlaneStride = 16 : i64>,
    //CHECK-SAME:           mode = #IE.space_to_depth_mode<BLOCKS_FIRST>,
    //CHECK-SAME:           port = 0 : i64}
    //CHECK:            inputs([[INPUT_2]] : memref<1x1x2x6xf16, [@CMX_NN, 0]>)
    //CHECK:            outputs([[OUTPUT_2]] : memref<1x1x2x6xf16, #NHWC, [@CMX_NN, 0]>) -> memref<1x1x2x6xf16, #NHWC, [@CMX_NN, 0]>
    //CHECK:    }

    //CHECK:    VPURT.Task waits([[BARRIER_0]] : !VPURT.Barrier) updates([[BARRIER_1]] : !VPURT.Barrier) attributes {isTrailingSWLayer = false} {
    //CHECK:        VPUIP.SpaceToDepthDMA {
    //CHECK-SAME:           block_size = 2 : i64,
    //CHECK-SAME:           dma_descriptor = #VPUIP.DMADescriptorAttr<numPlanes = 3 : i64, len = 8 : i64, srcWidth = 4 : i64, srcStride = 12 : i64, srcPlaneStride = 4 : i64, dstWidth = 2 : i64, dstStride = 4 : i64, dstPlaneStride = 16 : i64>,
    //CHECK-SAME:           mode = #IE.space_to_depth_mode<BLOCKS_FIRST>,
    //CHECK-SAME:           port = 1 : i64}
    //CHECK:            inputs([[INPUT_3]] : memref<1x1x2x6xf16, [@CMX_NN, 0]>)
    //CHECK:            outputs([[OUTPUT_3]] : memref<1x1x2x6xf16, #NHWC, [@CMX_NN, 0]>) -> memref<1x1x2x6xf16, #NHWC, [@CMX_NN, 0]>
    //CHECK:    }

    //CHECK:    VPURT.Task waits([[BARRIER_1]] : !VPURT.Barrier)  {
    //CHECK:        VPUIP.NNDMA {port = 0 : i64}
    //CHECK:            inputs([[OUTPUT_BUFFER]] : memref<1x8x2x3xf16, #NHWC, [@CMX_NN, 0]>)
    //CHECK:            outputs(%arg1 : memref<1x8x2x3xf16, #NHWC>) -> memref<1x8x2x3xf16, #NHWC>
    //CHECK:    }

    //CHECK:    return %arg1 : memref<1x8x2x3xf16, #NHWC>
}

func.func @UnrollSpaceToDepthDMADepthFirstNCHWToNHWC(%input: memref<1x2x4x6xf16>, %output: memref<1x8x2x3xf16, #NHWC>) -> memref<1x8x2x3xf16, #NHWC> {
    %bar0 = VPURT.DeclareVirtualBarrier -> !VPURT.Barrier
    %bar1 = VPURT.DeclareVirtualBarrier -> !VPURT.Barrier

    %inBuffer = VPURT.DeclareBuffer <CMX_NN> [0] <0> -> memref<1x2x4x6xf16, [@CMX_NN, 0]>
    %outBuffer = VPURT.DeclareBuffer <CMX_NN> [0] <128> -> memref<1x8x2x3xf16, #NHWC, [@CMX_NN, 0]>

    VPURT.Task updates(%bar0: !VPURT.Barrier)  {
        VPUIP.NNDMA {port = 0 : i64} inputs(%input : memref<1x2x4x6xf16>) outputs(%inBuffer : memref<1x2x4x6xf16, [@CMX_NN, 0]>) -> memref<1x2x4x6xf16, [@CMX_NN, 0]>
    }

    VPURT.Task waits(%bar0: !VPURT.Barrier) updates(%bar1: !VPURT.Barrier) {
        VPUIP.SpaceToDepthDMA {block_size = 2 : i64, mode = #IE.space_to_depth_mode<DEPTH_FIRST>, output_channel = 2 : i64, output_width = 6 : i64, port = 0 : i64}
                inputs(%inBuffer : memref<1x2x4x6xf16, [@CMX_NN, 0]>)
                outputs(%outBuffer : memref<1x8x2x3xf16, #NHWC, [@CMX_NN, 0]>) -> memref<1x8x2x3xf16, #NHWC, [@CMX_NN, 0]>
    }

    VPURT.Task waits(%bar1: !VPURT.Barrier) {
        VPUIP.NNDMA {port = 0 : i64} inputs(%outBuffer : memref<1x8x2x3xf16, #NHWC, [@CMX_NN, 0]>) outputs(%output :  memref<1x8x2x3xf16, #NHWC>) -> memref<1x8x2x3xf16, #NHWC>
    }

    return %output: memref<1x8x2x3xf16, #NHWC>
    //CHECK:    [[BARRIER_0:%.*]] = VPURT.DeclareVirtualBarrier -> !VPURT.Barrier
    //CHECK:    [[BARRIER_1:%.*]] = VPURT.DeclareVirtualBarrier -> !VPURT.Barrier

    //CHECK-DAG:    [[INPUT_BUFFER:%.*]] = VPURT.DeclareBuffer <CMX_NN> [0] <0> -> memref<1x2x4x6xf16, [@CMX_NN, 0]>
    //CHECK-DAG:    [[INPUT_1:%.*]] = VPURT.DeclareBuffer <CMX_NN> [0] <48> -> memref<1x1x4x6xf16, [@CMX_NN, 0]>
    //CHECK-DAG:    [[INPUT_0:%.*]] = VPURT.DeclareBuffer <CMX_NN> [0] <0> -> memref<1x1x4x6xf16, [@CMX_NN, 0]>
    //CHECK-DAG:    [[OUTPUT_BUFFER:%.*]] = VPURT.DeclareBuffer <CMX_NN> [0] <128> -> memref<1x8x2x3xf16, #NHWC, [@CMX_NN, 0]>
    //CHECK-DAG:    [[OUTPUT_1:%.*]] = VPURT.DeclareBuffer <CMX_NN> [0] <136> -> memref<1x1x4x6xf16, #NHWC, [@CMX_NN, 0]>
    //CHECK-DAG:    [[OUTPUT_0:%.*]] = VPURT.DeclareBuffer <CMX_NN> [0] <128> -> memref<1x1x4x6xf16, #NHWC, [@CMX_NN, 0]>


    //CHECK:    VPURT.Task updates([[BARRIER_0]] : !VPURT.Barrier)  {
    //CHECK:        VPUIP.NNDMA {port = 0 : i64}
    //CHECK:            inputs(%arg0 : memref<1x2x4x6xf16>)
    //CHECK:            outputs([[INPUT_BUFFER]] : memref<1x2x4x6xf16, [@CMX_NN, 0]>) -> memref<1x2x4x6xf16, [@CMX_NN, 0]>
    //CHECK:    }

    //CHECK:    VPURT.Task waits([[BARRIER_0]] : !VPURT.Barrier) updates([[BARRIER_1]] : !VPURT.Barrier) attributes {isTrailingSWLayer = false} {
    //CHECK:        VPUIP.SpaceToDepthDMA {
    //CHECK-SAME:           block_size = 2 : i64,
    //CHECK-SAME:            dma_descriptor = #VPUIP.DMADescriptorAttr<numPlanes = 2 : i64, len = 24 : i64, srcWidth = 12 : i64, srcStride = 24 : i64, srcPlaneStride = 12 : i64, dstWidth = 4 : i64, dstStride = 16 : i64, dstPlaneStride = 24 : i64>,
    //CHECK-SAME:           mode = #IE.space_to_depth_mode<DEPTH_FIRST>,
    //CHECK-SAME:           port = 0 : i64}
    //CHECK:            inputs([[INPUT_0]] : memref<1x1x4x6xf16, [@CMX_NN, 0]>)
    //CHECK:            outputs([[OUTPUT_0]] : memref<1x1x4x6xf16, #NHWC, [@CMX_NN, 0]>) -> memref<1x1x4x6xf16, #NHWC, [@CMX_NN, 0]>
    //CHECK:    }

    //CHECK:    VPURT.Task waits([[BARRIER_0]] : !VPURT.Barrier) updates([[BARRIER_1]] : !VPURT.Barrier) attributes {isTrailingSWLayer = false} {
    //CHECK:        VPUIP.SpaceToDepthDMA {
    //CHECK-SAME:           block_size = 2 : i64,
    //CHECK-SAME:            dma_descriptor = #VPUIP.DMADescriptorAttr<numPlanes = 2 : i64, len = 24 : i64, srcWidth = 12 : i64, srcStride = 24 : i64, srcPlaneStride = 12 : i64, dstWidth = 4 : i64, dstStride = 16 : i64, dstPlaneStride = 24 : i64>,
    //CHECK-SAME:           mode = #IE.space_to_depth_mode<DEPTH_FIRST>,
    //CHECK-SAME:           port = 1 : i64}
    //CHECK:            inputs([[INPUT_1]] : memref<1x1x4x6xf16, [@CMX_NN, 0]>)
    //CHECK:            outputs([[OUTPUT_1]] : memref<1x1x4x6xf16, #NHWC, [@CMX_NN, 0]>) -> memref<1x1x4x6xf16, #NHWC, [@CMX_NN, 0]>
    //CHECK:    }

    //CHECK:    VPURT.Task waits([[BARRIER_1]] : !VPURT.Barrier)  {
    //CHECK:        VPUIP.NNDMA {port = 0 : i64}
    //CHECK:            inputs([[OUTPUT_BUFFER]] : memref<1x8x2x3xf16, #NHWC, [@CMX_NN, 0]>)
    //CHECK:            outputs(%arg1 : memref<1x8x2x3xf16, #NHWC>) -> memref<1x8x2x3xf16, #NHWC>
    //CHECK:    }

    //CHECK:    return %arg1 : memref<1x8x2x3xf16, #NHWC>
}

// -----

#NHWC = affine_map<(d0, d1, d2, d3) -> (d0, d2, d3, d1)>

!qElemType = !quant.uniform<u8:f16, 0.0173492431640625:114>

!OutputDistributed = !VPUIP.DistributedBuffer<
    1x12x2x3x!qElemType, #NHWC, @CMX_NN, {
    mode = "SEGMENTED",
    num_tiles = [1, 1, 2, 1],
    num_clusters =  2 : i64
}>

// CHECK-LABEL: @UnrollSegmentedClusterSpaceToDepthDMA
func.func @UnrollSegmentedClusterSpaceToDepthDMA() -> !OutputDistributed {
    %bar0 = VPURT.DeclareVirtualBarrier -> !VPURT.Barrier
    %bar1 = VPURT.DeclareVirtualBarrier -> !VPURT.Barrier
    %0 = VPURT.DeclareBuffer <CMX_NN> [0] <0> -> memref<1x3x4x6x!qElemType, #NHWC, [@CMX_NN, 0]>
    %1 = VPURT.DeclareBuffer <CMX_NN> <0> -> !OutputDistributed

    VPURT.Task waits(%bar0 : !VPURT.Barrier) updates(%bar1 : !VPURT.Barrier) attributes {cycleBegin = 990 : i64, cycleEnd = 1758 : i64, isTrailingSWLayer = false} {
        VPUIP.SpaceToDepthDMA {block_size = 2 : i64, mode = #IE.space_to_depth_mode<BLOCKS_FIRST>, port = 0 : i64} 
              inputs(%0 : memref<1x3x4x6x!qElemType, #NHWC, [@CMX_NN, 0]>) 
              outputs(%1 : !OutputDistributed) -> !OutputDistributed
    }
    return %1: !OutputDistributed

    //CHECK:    [[BAR0:%.*]] = VPURT.DeclareVirtualBarrier -> !VPURT.Barrier
    //CHECK:    [[BAR1:%.*]] = VPURT.DeclareVirtualBarrier -> !VPURT.Barrier
    //CHECK:    [[INPUT0:%.*]] = VPURT.DeclareBuffer <CMX_NN> [0] <0> -> memref<1x3x2x6x!qElemType, #NHWC, [@CMX_NN, 0]>
    //CHECK:    [[INPUT1:%.*]] = VPURT.DeclareBuffer <CMX_NN> [0] <36> -> memref<1x3x2x6x!qElemType, #NHWC, [@CMX_NN, 0]>
    //CHECK:    [[OUTDISTRIBUTION:%.*]] = VPURT.DeclareBuffer <CMX_NN> <0> -> !VPUIP.DistributedBuffer<1x12x2x3x!qElemType, #NHWC, @CMX_NN, {mode = "SEGMENTED", num_tiles = [1, 1, 2, 1], num_clusters = 2 : i64}>
    //CHECK:    [[OUTPUT0:%.*]] = VPURT.DeclareBuffer <CMX_NN> [0] <0> -> memref<1x3x2x6x!qElemType, #NHWC, [@CMX_NN, 0]>
    //CHECK:    [[OUTPUT1:%.*]] = VPURT.DeclareBuffer <CMX_NN> [1] <0> -> memref<1x3x2x6x!qElemType, #NHWC, [@CMX_NN, 1]>

    //CHECK:    VPURT.Task waits([[BAR0]] : !VPURT.Barrier) updates([[BAR1]] : !VPURT.Barrier) attributes {cycleBegin = 990 : i64, cycleEnd = 1758 : i64, isTrailingSWLayer = false} {
    //CHECK:      VPUIP.SpaceToDepthDMA {block_size = 2 : i64, dma_descriptor = #VPUIP.DMADescriptorAttr<numPlanes = 2 : i64, len = 18 : i64, srcWidth = 18 : i64, srcStride = 36 : i64, srcPlaneStride = 18 : i64, dstWidth = 6 : i64, dstStride = 12 : i64, dstPlaneStride = 6 : i64>, mode = #IE.space_to_depth_mode<BLOCKS_FIRST>, port = 0 : i64}
    //CHECK:                inputs([[INPUT0]] : memref<1x3x2x6x!qElemType, #NHWC, [@CMX_NN, 0]>)
    //CHECK:                outputs([[OUTPUT0]] : memref<1x3x2x6x!qElemType, #NHWC, [@CMX_NN, 0]>) -> memref<1x3x2x6x!qElemType, #NHWC, [@CMX_NN, 0]>

    //CHECK:    VPURT.Task waits([[BAR0]] : !VPURT.Barrier) updates([[BAR1]] : !VPURT.Barrier) attributes {cycleBegin = 990 : i64, cycleEnd = 1758 : i64, isTrailingSWLayer = false} {
    //CHECK:      VPUIP.SpaceToDepthDMA {block_size = 2 : i64, dma_descriptor = #VPUIP.DMADescriptorAttr<numPlanes = 2 : i64, len = 18 : i64, srcWidth = 18 : i64, srcStride = 36 : i64, srcPlaneStride = 18 : i64, dstWidth = 6 : i64, dstStride = 12 : i64, dstPlaneStride = 6 : i64>, mode = #IE.space_to_depth_mode<BLOCKS_FIRST>, port = 0 : i64}
    //CHECK:                inputs([[INPUT1]] : memref<1x3x2x6x!qElemType, #NHWC, [@CMX_NN, 0]>)
    //CHECK:                outputs([[OUTPUT1]] : memref<1x3x2x6x!qElemType, #NHWC, [@CMX_NN, 1]>) -> memref<1x3x2x6x!qElemType, #NHWC, [@CMX_NN, 1]>

    //CHECK:    return [[OUTDISTRIBUTION]] : !VPUIP.DistributedBuffer<1x12x2x3x!qElemType, #NHWC, @CMX_NN, {mode = "SEGMENTED", num_tiles = [1, 1, 2, 1], num_clusters = 2 : i64}>
}
