//
// Copyright (C) 2023 Intel Corporation.
// SPDX-License-Identifier: Apache 2.0
//

// RUN: vpux-opt --split-input-file --init-compiler="vpu-arch=VPUX37XX compilation-mode=DefaultHW" --vertical-fusion-tiling %s | FileCheck %s


#NHWC = affine_map<(d0, d1, d2, d3) -> (d0, d2, d3, d1)>

func.func @VfTilingDepthConvAndConv(%arg0: tensor<1x576x65x65xf16, {order = #NHWC}>) -> tensor<1x160x65x65xf16, {order = #NHWC}>   {
    %cst_0 = const.Declare tensor<576x32x1x1xf16, {order = #NHWC}> = dense<1.0> : tensor<576x32x1x1xf16>, [#const.Reorder<#NHWC>]
    %cst_1 = const.Declare tensor<576x1x1x4xsi32> = dense<1> : tensor<576x1x1x4xsi32>
    %cst_2 = const.Declare tensor<1x1x1x16xui8> = dense<[[[[31, 0, 240, 1, 0, 31, 0, 240, 1, 0, 31, 0, 0, 0, 0, 0]]]]> : tensor<1x1x1x16xui8>
    %cst_3 = const.Declare tensor<160x576x1x1xf16, {order = #NHWC}> = dense<1.0> : tensor<160x576x1x1xf16>, [#const.Reorder<#NHWC>]
    %cst_4 = const.Declare tensor<160x1x1x4xsi32> = dense<0> : tensor<160x1x1x4xsi32>
    
    %0 = VPU.VerticalFusion (%arg0 as %arg1: tensor<1x576x65x65xf16, {order = #NHWC}>, %cst_0 as %arg2: tensor<576x32x1x1xf16, {order = #NHWC}>, %cst_1 as %arg3: tensor<576x1x1x4xsi32>, %cst_2 as %arg4: tensor<1x1x1x16xui8>, %cst_3 as %arg5: tensor<160x576x1x1xf16, {order = #NHWC}>, %cst_4 as %arg6: tensor<160x1x1x4xsi32>) attributes {tilingStrategy = [1, 1, 6, 1]} -> tensor<1x160x65x65xf16, {order = #NHWC}> {
      %1 = VPU.NCE.DepthConvolution(%arg1, %arg2, %arg3, %arg4) 
         {activation_window_channel_length = 40 : i64, multiClusterStrategy = #VPU.multi_cluster_strategy<SplitOverKernel>, 
         pad = #VPU.Padding<left = 2 : i64, right = 2 : i64, top = 2 : i64, bottom = 2 : i64>, 
         ppe = #VPU.PPETask<mode = <LRELUX>, clamp_low = 0 : i64, clamp_high = 255 : i64, lrelu_mult = 1 : i64, lrelu_shift = 0 : i64, fp_prelu_alpha = 1.000000e+00 : f64>, rawFilterShape = [576, 1, 5, 5], strides = [1, 1]}
         -> tensor<1x576x65x65xf16, {order = #NHWC}> 
      %2 = VPU.NCE.Convolution(%1, %arg5, %arg6) 
         {multiClusterStrategy = #VPU.multi_cluster_strategy<SplitOverKernel>, 
         pad = #VPU.Padding<left = 0 : i64, right = 0 : i64, top = 0 : i64, bottom = 0 : i64>, 
         ppe = #VPU.PPETask<mode = <NOOP>, clamp_low = 0 : i64, clamp_high = 255 : i64, lrelu_mult = 1 : i64, lrelu_shift = 0 : i64, fp_prelu_alpha = 1.000000e+00 : f64>, 
         rawFilterShape = [160, 576, 1, 1], strides = [1, 1]} -> tensor<1x160x65x65xf16, {order = #NHWC}> 
      VPU.Yield %2 
    }

    return %0 : tensor<1x160x65x65xf16, {order = #NHWC}>

    // CHECK: [[SLICEARG0TILE0:%.+]] = VPU.Slice %arg0 [0, 0, 0, 0] [1, 576, 13, 65] 
    // CHECK: [[DEPTHCONVTILE0:%.+]] = VPU.NCE.DepthConvolution([[SLICEARG0TILE0]], %cst, %cst_0, %cst_1) 
    // CHECK-SAME: {activation_window_channel_length = 40 : i64, multiClusterStrategy = #VPU.multi_cluster_strategy<SplitOverKernel>, pad = #VPU.Padding<left = 2 : i64, right = 2 : i64, top = 2 : i64, bottom = 0 : i64>, ppe = #VPU.PPETask<mode = <LRELUX>, clamp_low = 0 : i64, clamp_high = 255 : i64, lrelu_mult = 1 : i64, lrelu_shift = 0 : i64, fp_prelu_alpha = 1.000000e+00 : f64>, rawFilterShape = [576, 1, 5, 5], strides = [1, 1]} -> tensor<1x576x11x65xf16, {order = #NHWC}> 
    // CHECK: [[CONVTILE0:%.+]] = VPU.NCE.Convolution([[DEPTHCONVTILE0]], %cst_2, %cst_3) 
    // CHECK-SAME: {multiClusterStrategy = #VPU.multi_cluster_strategy<SplitOverKernel>, pad = #VPU.Padding<left = 0 : i64, right = 0 : i64, top = 0 : i64, bottom = 0 : i64>, ppe = #VPU.PPETask<mode = <NOOP>, clamp_low = 0 : i64, clamp_high = 255 : i64, lrelu_mult = 1 : i64, lrelu_shift = 0 : i64, fp_prelu_alpha = 1.000000e+00 : f64>, rawFilterShape = [160, 576, 1, 1], strides = [1, 1]} -> tensor<1x160x11x65xf16, {order = #NHWC}> 
    // CHECK: [[SLICEARG0TILE1:%.+]] = VPU.Slice %arg0 [0, 0, 9, 0] [1, 576, 15, 65] 
    // CHECK: [[DEPTHCONVTILE1:%.+]] = VPU.NCE.DepthConvolution([[SLICEARG0TILE1]], %cst, %cst_0, %cst_1) 
    // CHECK-SAME: {activation_window_channel_length = 40 : i64, multiClusterStrategy = #VPU.multi_cluster_strategy<SplitOverKernel>, pad = #VPU.Padding<left = 2 : i64, right = 2 : i64, top = 0 : i64, bottom = 0 : i64>, ppe = #VPU.PPETask<mode = <LRELUX>, clamp_low = 0 : i64, clamp_high = 255 : i64, lrelu_mult = 1 : i64, lrelu_shift = 0 : i64, fp_prelu_alpha = 1.000000e+00 : f64>, rawFilterShape = [576, 1, 5, 5], strides = [1, 1]} -> tensor<1x576x11x65xf16, {order = #NHWC}> 
    // CHECK: [[CONVTILE1:%.+]] = VPU.NCE.Convolution([[DEPTHCONVTILE1]], %cst_2, %cst_3) 
    // CHECK-SAME: {multiClusterStrategy = #VPU.multi_cluster_strategy<SplitOverKernel>, pad = #VPU.Padding<left = 0 : i64, right = 0 : i64, top = 0 : i64, bottom = 0 : i64>, ppe = #VPU.PPETask<mode = <NOOP>, clamp_low = 0 : i64, clamp_high = 255 : i64, lrelu_mult = 1 : i64, lrelu_shift = 0 : i64, fp_prelu_alpha = 1.000000e+00 : f64>, rawFilterShape = [160, 576, 1, 1], strides = [1, 1]} -> tensor<1x160x11x65xf16, {order = #NHWC}> 
    // CHECK: [[SLICEARG0TILE2:%.+]] = VPU.Slice %arg0 [0, 0, 20, 0] [1, 576, 15, 65] 
    // CHECK: [[DEPTHCONVTILE2:%.+]] = VPU.NCE.DepthConvolution([[SLICEARG0TILE2]], %cst, %cst_0, %cst_1) 
    // CHECK-SAME: {activation_window_channel_length = 40 : i64, multiClusterStrategy = #VPU.multi_cluster_strategy<SplitOverKernel>, pad = #VPU.Padding<left = 2 : i64, right = 2 : i64, top = 0 : i64, bottom = 0 : i64>, ppe = #VPU.PPETask<mode = <LRELUX>, clamp_low = 0 : i64, clamp_high = 255 : i64, lrelu_mult = 1 : i64, lrelu_shift = 0 : i64, fp_prelu_alpha = 1.000000e+00 : f64>, rawFilterShape = [576, 1, 5, 5], strides = [1, 1]} -> tensor<1x576x11x65xf16, {order = #NHWC}> 
    // CHECK: [[CONVTILE2:%.+]] = VPU.NCE.Convolution([[DEPTHCONVTILE2]], %cst_2, %cst_3) 
    // CHECK-SAME: {multiClusterStrategy = #VPU.multi_cluster_strategy<SplitOverKernel>, pad = #VPU.Padding<left = 0 : i64, right = 0 : i64, top = 0 : i64, bottom = 0 : i64>, ppe = #VPU.PPETask<mode = <NOOP>, clamp_low = 0 : i64, clamp_high = 255 : i64, lrelu_mult = 1 : i64, lrelu_shift = 0 : i64, fp_prelu_alpha = 1.000000e+00 : f64>, rawFilterShape = [160, 576, 1, 1], strides = [1, 1]} -> tensor<1x160x11x65xf16, {order = #NHWC}> 
    // CHECK: [[SLICEARG0TILE3:%.+]] = VPU.Slice %arg0 [0, 0, 31, 0] [1, 576, 15, 65] 
    // CHECK: [[DEPTHCONVTILE3:%.+]] = VPU.NCE.DepthConvolution([[SLICEARG0TILE3]], %cst, %cst_0, %cst_1) 
    // CHECK-SAME: {activation_window_channel_length = 40 : i64, multiClusterStrategy = #VPU.multi_cluster_strategy<SplitOverKernel>, pad = #VPU.Padding<left = 2 : i64, right = 2 : i64, top = 0 : i64, bottom = 0 : i64>, ppe = #VPU.PPETask<mode = <LRELUX>, clamp_low = 0 : i64, clamp_high = 255 : i64, lrelu_mult = 1 : i64, lrelu_shift = 0 : i64, fp_prelu_alpha = 1.000000e+00 : f64>, rawFilterShape = [576, 1, 5, 5], strides = [1, 1]} -> tensor<1x576x11x65xf16, {order = #NHWC}> 
    // CHECK: [[CONVTILE3:%.+]] = VPU.NCE.Convolution([[DEPTHCONVTILE3]], %cst_2, %cst_3) 
    // CHECK-SAME: {multiClusterStrategy = #VPU.multi_cluster_strategy<SplitOverKernel>, pad = #VPU.Padding<left = 0 : i64, right = 0 : i64, top = 0 : i64, bottom = 0 : i64>, ppe = #VPU.PPETask<mode = <NOOP>, clamp_low = 0 : i64, clamp_high = 255 : i64, lrelu_mult = 1 : i64, lrelu_shift = 0 : i64, fp_prelu_alpha = 1.000000e+00 : f64>, rawFilterShape = [160, 576, 1, 1], strides = [1, 1]} -> tensor<1x160x11x65xf16, {order = #NHWC}> 
    // CHECK: [[SLICEARG0TILE4:%.+]] = VPU.Slice %arg0 [0, 0, 42, 0] [1, 576, 15, 65] 
    // CHECK: [[DEPTHCONVTILE4:%.+]] = VPU.NCE.DepthConvolution([[SLICEARG0TILE4]], %cst, %cst_0, %cst_1) 
    // CHECK-SAME: {activation_window_channel_length = 40 : i64, multiClusterStrategy = #VPU.multi_cluster_strategy<SplitOverKernel>, pad = #VPU.Padding<left = 2 : i64, right = 2 : i64, top = 0 : i64, bottom = 0 : i64>, ppe = #VPU.PPETask<mode = <LRELUX>, clamp_low = 0 : i64, clamp_high = 255 : i64, lrelu_mult = 1 : i64, lrelu_shift = 0 : i64, fp_prelu_alpha = 1.000000e+00 : f64>, rawFilterShape = [576, 1, 5, 5], strides = [1, 1]} -> tensor<1x576x11x65xf16, {order = #NHWC}> 
    // CHECK: [[CONVTILE4:%.+]] = VPU.NCE.Convolution([[DEPTHCONVTILE4]], %cst_2, %cst_3) 
    // CHECK-SAME: {multiClusterStrategy = #VPU.multi_cluster_strategy<SplitOverKernel>, pad = #VPU.Padding<left = 0 : i64, right = 0 : i64, top = 0 : i64, bottom = 0 : i64>, ppe = #VPU.PPETask<mode = <NOOP>, clamp_low = 0 : i64, clamp_high = 255 : i64, lrelu_mult = 1 : i64, lrelu_shift = 0 : i64, fp_prelu_alpha = 1.000000e+00 : f64>, rawFilterShape = [160, 576, 1, 1], strides = [1, 1]} -> tensor<1x160x11x65xf16, {order = #NHWC}> 
    // CHECK: [[SLICEARG0TILE5:%.+]] = VPU.Slice %arg0 [0, 0, 53, 0] [1, 576, 12, 65] 
    // CHECK: [[DEPTHCONVTILE5:%.+]] = VPU.NCE.DepthConvolution([[SLICEARG0TILE5]], %cst, %cst_0, %cst_1) 
    // CHECK-SAME: {activation_window_channel_length = 40 : i64, multiClusterStrategy = #VPU.multi_cluster_strategy<SplitOverKernel>, pad = #VPU.Padding<left = 2 : i64, right = 2 : i64, top = 0 : i64, bottom = 2 : i64>, ppe = #VPU.PPETask<mode = <LRELUX>, clamp_low = 0 : i64, clamp_high = 255 : i64, lrelu_mult = 1 : i64, lrelu_shift = 0 : i64, fp_prelu_alpha = 1.000000e+00 : f64>, rawFilterShape = [576, 1, 5, 5], strides = [1, 1]} -> tensor<1x576x10x65xf16, {order = #NHWC}> 
    // CHECK: [[CONVTILE5:%.+]] = VPU.NCE.Convolution([[DEPTHCONVTILE5]], %cst_2, %cst_3) 
    // CHECK-SAME: {multiClusterStrategy = #VPU.multi_cluster_strategy<SplitOverKernel>, pad = #VPU.Padding<left = 0 : i64, right = 0 : i64, top = 0 : i64, bottom = 0 : i64>, ppe = #VPU.PPETask<mode = <NOOP>, clamp_low = 0 : i64, clamp_high = 255 : i64, lrelu_mult = 1 : i64, lrelu_shift = 0 : i64, fp_prelu_alpha = 1.000000e+00 : f64>, rawFilterShape = [160, 576, 1, 1], strides = [1, 1]} -> tensor<1x160x10x65xf16, {order = #NHWC}>
    // CHECK: [[CONCAT:%.+]] = VPU.Concat([[CONVTILE0]], [[CONVTILE1]], [[CONVTILE2]], [[CONVTILE3]], [[CONVTILE4]], [[CONVTILE5]]) {static_offsets = {{\[\[}}0, 0, 0, 0], [0, 0, 11, 0], [0, 0, 22, 0], [0, 0, 33, 0], [0, 0, 44, 0], [0, 0, 55, 0]]} : tensor<1x160x11x65xf16, {order = #NHWC}>, tensor<1x160x11x65xf16, {order = #NHWC}>, tensor<1x160x11x65xf16, {order = #NHWC}>, tensor<1x160x11x65xf16, {order = #NHWC}>, tensor<1x160x11x65xf16, {order = #NHWC}>, tensor<1x160x10x65xf16, {order = #NHWC}> -> tensor<1x160x65x65xf16, {order = #NHWC}>
    // CHECK: return [[CONCAT]] : tensor<1x160x65x65xf16, {order = #NHWC}>
}

// -----

#NHWC = affine_map<(d0, d1, d2, d3) -> (d0, d2, d3, d1)>

func.func @VfTilingBetweenEltwise(%input: tensor<1x64x256x256xf16, {order = #NHWC}>, %cst_1: tensor<256x64x1x1xf16, {order = #NHWC}>, %cst_2: tensor<256x1x1x4xsi32>, %cst_3: tensor<1x256x256x256xf16, {order = #NHWC}>, %cst_4: tensor<64x256x1x1xf16, {order = #NHWC}>, %cst_5: tensor<64x1x1x4xsi32>, %cst_6: tensor<64x64x3x3xf16, {order = #NHWC}>, %cst_7: tensor<64x1x1x4xsi32>, %cst_8: tensor<256x64x1x1xf16, {order = #NHWC}>, %cst_9: tensor<256x1x1x4xsi32>) -> tensor<1x256x256x256xf16>  {

  %0 = VPU.VerticalFusion (%input as %arg1: tensor<1x64x256x256xf16, {order = #NHWC}>, %cst_1 as %arg2: tensor<256x64x1x1xf16, {order = #NHWC}>, %cst_2 as %arg3: tensor<256x1x1x4xsi32>, %cst_3 as %arg4: tensor<1x256x256x256xf16, {order = #NHWC}>) attributes {tilingStrategy = [1, 1, 32, 1]} -> tensor<1x256x256x256xf16, {order = #NHWC}> {
    %2 = VPU.NCE.Convolution(%arg1, %arg2, %arg3) {
        multiClusterStrategy = #VPU.multi_cluster_strategy<SplitOverHeight>,
        pad = #VPU.Padding<left = 0 : i64, right = 0 : i64, top = 0 : i64, bottom = 0 : i64>,
        ppe = #VPU.PPETask<clamp_high = 255 : i64, clamp_low = 0 : i64,
               fp_prelu_alpha = 1.000000e+00 : f64, lrelu_mult = 1 : i64, lrelu_shift = 0 : i64, mode = <NOOP>>,
        rawFilterShape = [256, 64, 1, 1], strides = [1, 1]} -> tensor<1x256x256x256xf16, {order = #NHWC}>

    %3 = VPU.NCE.Eltwise(%2, %arg4) {
        is_inplace = true, multiClusterStrategy = #VPU.multi_cluster_strategy<SplitOverHeight>, op_type = #VPU.eltwise_type<ADD>,
        ppe = #VPU.PPETask<clamp_high = 255 : i64, clamp_low = 0 : i64,
               fp_prelu_alpha = 1.000000e+00 : f64, in1_quant_mult = [17726], in2_quant_mult = [17916],
               lrelu_mult = 1 : i64, lrelu_shift = 0 : i64, mode = <LRELUX>, quant_mult = [30048],
               quant_post_shift = 0 : i64, quant_shift = [28]>} -> tensor<1x256x256x256xf16, {order = #NHWC}>
    VPU.Yield %3
  }

  %1 = VPU.VerticalFusion (%0 as %arg1: tensor<1x256x256x256xf16, {order = #NHWC}>, %cst_4 as %arg2: tensor<64x256x1x1xf16, {order = #NHWC}>, %cst_5 as %arg3: tensor<64x1x1x4xsi32>, %cst_6 as %arg4: tensor<64x64x3x3xf16, {order = #NHWC}>, %cst_7 as %arg5: tensor<64x1x1x4xsi32>, %cst_8 as %arg6: tensor<256x64x1x1xf16, {order = #NHWC}>, %cst_9 as %arg7: tensor<256x1x1x4xsi32>) attributes {tilingStrategy = [1, 1, 32, 1]} -> tensor<1x256x256x256xf16> {
    %2 = VPU.NCE.Convolution(%arg1, %arg2, %arg3) {
        multiClusterStrategy = #VPU.multi_cluster_strategy<SplitOverHeight>,
        pad = #VPU.Padding<left = 0 : i64, right = 0 : i64, top = 0 : i64, bottom = 0 : i64>,
        ppe = #VPU.PPETask<clamp_high = 255 : i64, clamp_low = 0 : i64,
               fp_prelu_alpha = 1.000000e+00 : f64, lrelu_mult = 1 : i64, lrelu_shift = 0 : i64, mode = <NOOP>>,
               rawFilterShape = [64, 256, 1, 1], strides = [1, 1]} -> tensor<1x64x256x256xf16, {order = #NHWC}>

    %3 = VPU.NCE.Convolution(%2, %arg4, %arg5) {multiClusterStrategy = #VPU.multi_cluster_strategy<SplitOverHeight>,
        pad = #VPU.Padding<left = 1 : i64, right = 1 : i64, top = 1 : i64, bottom = 1 : i64>,
        ppe = #VPU.PPETask<clamp_high = 255 : i64, clamp_low = 0 : i64,
               fp_prelu_alpha = 1.000000e+00 : f64, lrelu_mult = 1 : i64, lrelu_shift = 0 : i64, mode = <NOOP>>,
               rawFilterShape = [64, 64, 3, 3], strides = [1, 1]} -> tensor<1x64x256x256xf16, {order = #NHWC}>

    %4 = VPU.NCE.Convolution(%3, %arg6, %arg7) {
        multiClusterStrategy = #VPU.multi_cluster_strategy<SplitOverHeight>,
        pad = #VPU.Padding<left = 0 : i64, right = 0 : i64, top = 0 : i64, bottom = 0 : i64>,
        ppe = #VPU.PPETask<clamp_high = 255 : i64, clamp_low = 0 : i64,
               fp_prelu_alpha = 1.000000e+00 : f64, lrelu_mult = 1 : i64, lrelu_shift = 0 : i64, mode = <NOOP>>,
               rawFilterShape = [256, 64, 1, 1], strides = [1, 1]} -> tensor<1x256x256x256xf16, {order = #NHWC}>
    %5 = VPU.NCE.Eltwise(%4, %arg1) {
        multiClusterStrategy = #VPU.multi_cluster_strategy<SplitOverHeight>, op_type = #VPU.eltwise_type<ADD>,
        ppe = #VPU.PPETask<clamp_high = 2147483647 : i64, clamp_low = -2147483648 : i64,
               fp_prelu_alpha = 1.000000e+00 : f64, in1_quant_mult = [23267], in2_quant_mult = [17866],
               lrelu_mult = 1 : i64, lrelu_shift = 0 : i64, mode = <NOOP>, quant_mult = [16384],
               quant_post_shift = 0 : i64, quant_shift = [34]>} -> tensor<1x256x256x256xf16>
    VPU.Yield %5
  }
  return %1 : tensor<1x256x256x256xf16>

    // CHECK: [[VF1_ELT_SLICE:%.*]] = VPU.Slice %arg3 [0, 0, 0, 0] [1, 256, 8, 256] :
    // CHECK:                  tensor<1x256x256x256xf16, {order = #NHWC}> to tensor<1x256x8x256xf16, {order = #NHWC}>
    // CHECK: [[VF1_ELT:%.*]] = VPU.NCE.Eltwise
    // CHECK:                   -> tensor<1x256x8x256xf16, {order = #NHWC}>

    // CHECK: [[VF1_ELT_SLICE:%.*]] = VPU.Slice %arg3 [0, 0, 8, 0] [1, 256, 8, 256] :
    // CHECK:                   tensor<1x256x256x256xf16, {order = #NHWC}> to tensor<1x256x8x256xf16, {order = #NHWC}>
    // CHECK: [[VF1_ELT:%.*]] = VPU.NCE.Eltwise
    // CHECK:                   -> tensor<1x256x8x256xf16, {order = #NHWC}>
    // ...
    // CHECK: [[VF1_ELT_SLICE:%.*]] = VPU.Slice %arg3 [0, 0, 240, 0] [1, 256, 8, 256] :
    // CHECK:                  tensor<1x256x256x256xf16, {order = #NHWC}> to tensor<1x256x8x256xf16, {order = #NHWC}>
    // CHECK: [[VF1_ELT:%.*]] = VPU.NCE.Eltwise
    // CHECK:                   -> tensor<1x256x8x256xf16, {order = #NHWC}>

    // CHECK: [[VF1_ELT_SLICE:%.*]] = VPU.Slice %arg3 [0, 0, 248, 0] [1, 256, 8, 256] :
    // CHECK:                   tensor<1x256x256x256xf16, {order = #NHWC}> to tensor<1x256x8x256xf16, {order = #NHWC}>
    // CHECK: [[VF1_ELT:%.*]] = VPU.NCE.Eltwise
    // CHECK:                   -> tensor<1x256x8x256xf16, {order = #NHWC}>

    // CHECK: [[VF1_CONCAT:%.*]] = VPU.Concat

    // CHECK: [[VF2_CONV_SLICE:%.*]] = VPU.Slice [[VF1_CONCAT]] [0, 0, 0, 0] [1, 256, 9, 256]
    // CHECK: [[VF2_ELT_SLICE_0:%.*]] = VPU.Slice [[VF1_CONCAT]] [0, 0, 0, 0] [1, 256, 9, 256]
    // CHECK: [[VF2_ELT_SLICE_1:%.*]] = VPU.Slice [[VF2_ELT_SLICE_0]] [0, 0, 0, 0] [1, 256, 8, 256] :
    // CHECK:                    tensor<1x256x9x256xf16, {order = #NHWC}> to tensor<1x256x8x256xf16, {order = #NHWC}>
    // CHECK: [[VF2_ELT:%.*]] = VPU.NCE.Eltwise
    // CHECK:                    -> tensor<1x256x8x256xf16>

    // CHECK: [[VF2_CONV_SLICE:%.*]] = VPU.Slice [[VF1_CONCAT]] [0, 0, 7, 0] [1, 256, 10, 256]
    // CHECK: [[VF2_ELT_SLICE_0:%.*]] = VPU.Slice [[VF1_CONCAT]] [0, 0, 7, 0] [1, 256, 10, 256]
    // CHECK: [[VF2_ELT_SLICE_1:%.*]] = VPU.Slice [[VF2_ELT_SLICE_0]] [0, 0, 1, 0] [1, 256, 8, 256] :
    // CHECK:                     tensor<1x256x10x256xf16, {order = #NHWC}> to tensor<1x256x8x256xf16, {order = #NHWC}>
    // CHECK: [[VF2_ELT:%.*]] = VPU.NCE.Eltwise
    // CHECK:                     -> tensor<1x256x8x256xf16>
    // ...
    // CHECK: [[VF2_CONV_SLICE:%.*]] = VPU.Slice [[VF1_CONCAT]] [0, 0, 239, 0] [1, 256, 10, 256]
    // CHECK: [[VF2_ELT_SLICE_0:%.*]] = VPU.Slice [[VF1_CONCAT]] [0, 0, 239, 0] [1, 256, 10, 256]
    // CHECK: [[VF2_ELT_SLICE_1:%.*]] = VPU.Slice [[VF2_ELT_SLICE_0]] [0, 0, 1, 0] [1, 256, 8, 256] :
    // CHECK:                    tensor<1x256x10x256xf16, {order = #NHWC}> to tensor<1x256x8x256xf16, {order = #NHWC}>
    // CHECK: [[VF2_ELT:%.*]] = VPU.NCE.Eltwise
    // CHECK:                    -> tensor<1x256x8x256xf16>

    // CHECK: [[VF2_CONV_SLICE:%.*]] = VPU.Slice [[VF1_CONCAT]] [0, 0, 247, 0] [1, 256, 9, 256]
    // CHECK: [[VF2_ELT_SLICE_0:%.*]] = VPU.Slice [[VF1_CONCAT]] [0, 0, 247, 0] [1, 256, 9, 256]
    // CHECK: [[VF2_ELT_SLICE_1:%.*]] = VPU.Slice [[VF2_ELT_SLICE_0]] [0, 0, 1, 0] [1, 256, 8, 256] :
    // CHECK:                     tensor<1x256x9x256xf16, {order = #NHWC}> to tensor<1x256x8x256xf16, {order = #NHWC}>
    // CHECK: [[VF2_ELT:%.*]] = VPU.NCE.Eltwise
    // CHECK:                     -> tensor<1x256x8x256xf16>

    // CHECK: [[VF2_CONCAT:%.*]] = VPU.Concat
    // CHECK: return [[VF2_CONCAT]] : tensor<1x256x256x256xf16>
}
