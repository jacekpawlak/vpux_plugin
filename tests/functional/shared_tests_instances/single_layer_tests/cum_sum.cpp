//
// Copyright (C) 2022-2023 Intel Corporation.
// SPDX-License-Identifier: Apache 2.0
//

#include "single_layer_tests/cum_sum.hpp"
#include <vector>
#include "vpu_ov1_layer_test.hpp"

namespace LayerTestsDefinitions {

class VPUXCumSumLayerTest : public CumSumLayerTest, virtual public LayerTestsUtils::VpuOv1LayerTestsCommon {};
class VPUXCumSumLayerTest_VPU3720 : public VPUXCumSumLayerTest {};

TEST_P(VPUXCumSumLayerTest_VPU3720, HW) {
    setPlatformVPU3720();
    setDefaultHardwareModeMLIR();
    Run();
}

}  // namespace LayerTestsDefinitions

using namespace LayerTestsDefinitions;

namespace {
const std::vector<std::vector<size_t>> shapes = {{5, 14, 5, 7},
                                                 // Values from real neural networks
                                                 {1, 1},
                                                 {1, 1024},
                                                 {1, 128},
                                                 {1, 25, 36},
                                                 {1, 384},
                                                 {1, 5},
                                                 {1, 9},
                                                 {8, 128},
                                                 {8, 384}};

const std::vector<InferenceEngine::Precision> inputPrecision = {InferenceEngine::Precision::FP16,
                                                                InferenceEngine::Precision::FP32};

const std::vector<int64_t> axes = {0, 1};
const std::vector<int64_t> negativeAxes = {-2, -1};

const std::vector<bool> exclusive = {true, false};
const std::vector<bool> reverse = {true, false};

const auto testCaseAxis_0 =
        testing::Combine(testing::Values(shapes[0]), testing::Values(inputPrecision[0]), testing::Values(axes[0]),
                         testing::Values(exclusive[0]), testing::Values(reverse[1]),
                         testing::Values(LayerTestsUtils::testPlatformTargetDevice()));

const auto testCasesNegativeAxis =
        testing::Combine(testing::Values(shapes[0]), testing::Values(inputPrecision[1]),
                         testing::ValuesIn(negativeAxes), testing::Values(exclusive[1]), testing::Values(reverse[0]),
                         testing::Values(LayerTestsUtils::testPlatformTargetDevice()));

const auto testCasesRealNet =
        testing::Combine(testing::ValuesIn(std::vector<std::vector<size_t>>(shapes.begin() + 1, shapes.end())),
                         testing::Values(inputPrecision[0]), testing::Values(axes[1]), testing::Values(exclusive[1]),
                         testing::Values(reverse[1]), testing::Values(LayerTestsUtils::testPlatformTargetDevice()));

const auto testCasePrecommit =
        testing::Combine(testing::Values(shapes[4]), testing::Values(inputPrecision[0]),
                         testing::Values(negativeAxes[0]), testing::Values(exclusive[0]), testing::Values(reverse[0]),
                         testing::Values(LayerTestsUtils::testPlatformTargetDevice()));

// ------ VPU3720 ------
INSTANTIATE_TEST_SUITE_P(smoke_CumSum_axis_0, VPUXCumSumLayerTest_VPU3720, testCaseAxis_0,
                         VPUXCumSumLayerTest_VPU3720::getTestCaseName);
INSTANTIATE_TEST_SUITE_P(smoke_CumSum_negative_axis, VPUXCumSumLayerTest_VPU3720, testCasesNegativeAxis,
                         VPUXCumSumLayerTest_VPU3720::getTestCaseName);
INSTANTIATE_TEST_SUITE_P(smoke_CumSum_real_net, VPUXCumSumLayerTest_VPU3720, testCasesRealNet,
                         VPUXCumSumLayerTest_VPU3720::getTestCaseName);
INSTANTIATE_TEST_SUITE_P(smoke_precommit_CumSum, VPUXCumSumLayerTest_VPU3720, testCasePrecommit,
                         VPUXCumSumLayerTest_VPU3720::getTestCaseName);

}  // namespace
