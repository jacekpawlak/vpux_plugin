//
// Copyright (C) 2022-2023 Intel Corporation.
// SPDX-License-Identifier: Apache 2.0
//

#include <vector>

#include "single_layer_tests/strided_slice.hpp"
#include "vpu_ov1_layer_test.hpp"
namespace LayerTestsDefinitions {
class VPUXStridedSliceLayerTest :
        public StridedSliceLayerTest,
        virtual public LayerTestsUtils::VpuOv1LayerTestsCommon {};
class VPUXStridedSliceLayerTest_VPU3700 : public VPUXStridedSliceLayerTest {};

class VPUXStridedSliceLayerTest_VPU3720 : public VPUXStridedSliceLayerTest {};
class VPUXStridedSliceLayerTilingTest_VPU3720 : public VPUXStridedSliceLayerTest {};

TEST_P(VPUXStridedSliceLayerTest_VPU3700, HW) {
    setPlatformVPU3700();
    setDefaultHardwareModeMLIR();
    Run();
}

TEST_P(VPUXStridedSliceLayerTest_VPU3720, SW) {
    setPlatformVPU3720();
    setReferenceSoftwareModeMLIR();
    Run();
}

TEST_P(VPUXStridedSliceLayerTilingTest_VPU3720, HW) {
    setPlatformVPU3720();
    setDefaultHardwareModeMLIR();
    Run();
}

}  // namespace LayerTestsDefinitions

using namespace LayerTestsDefinitions;

namespace {

std::vector<StridedSliceSpecificParams> tests = {
        // custom tests
        {{1, 32, 12, 64}, {0, 0, 1, 0}, {1, 32, 12, 64}, {1, 1, 1, 1}, {1, 1, 0, 1}, {1, 1, 1, 1}, {}, {}, {}},
        {{1, 32, 64, 128}, {0, 0, 53, 0}, {1, 32, 64, 128}, {1, 1, 1, 1}, {1, 1, 0, 1}, {1, 1, 1, 1}, {}, {}, {}},
        {{1, 32, 64, 256}, {0, 0, 54, 0}, {1, 32, 64, 128}, {1, 1, 1, 1}, {1, 1, 0, 1}, {1, 1, 1, 1}, {}, {}, {}},
        {{1, 32, 64, 512}, {0, 0, 55, 0}, {1, 32, 64, 128}, {1, 1, 1, 1}, {1, 1, 0, 1}, {1, 1, 1, 1}, {}, {}, {}},

        {{32, 32}, {0, 0, 0}, {0, 0, 0}, {1, 1, 1}, {1, 1, 1}, {1, 1, 1}, {1, 0, 0}, {0, 0, 0}, {0, 0, 0}},
        {{32, 32}, {0, 0, 0}, {0, 0, 0}, {1, 1, 1}, {1, 1, 1}, {1, 1, 1}, {0, 1, 0}, {0, 0, 0}, {0, 0, 0}},
        {{32, 32}, {0, 0, 0}, {0, 0, 0}, {1, 1, 1}, {1, 1, 1}, {1, 1, 1}, {0, 0, 1}, {0, 0, 0}, {0, 0, 0}},
        {{32, 32, 32}, {0, 0}, {0, 0}, {1, 1}, {1, 1}, {1, 1}, {0, 0}, {1, 0}, {0, 0}},
        {{32, 32, 32}, {0, 0}, {0, 0}, {1, 1}, {1, 1}, {1, 1}, {0, 0}, {0, 1}, {0, 0}},

        // from MKLDNN plugin
        {{32, 32}, {0, 20}, {32, 30}, {1, 1}, {0, 0}, {0, 0}, {}, {}, {}},
        {{32, 20}, {2, 10}, {32, 20}, {1, 1}, {0, 0}, {0, 0}, {}, {}, {}},
        {{32, 20}, {2, 10}, {32, 20}, {1, 2}, {0, 1}, {1, 0}, {}, {}, {}},
        {{32, 20}, {2, 10}, {32, 20}, {2, 1}, {0, 0}, {1, 0}, {}, {}, {}},
        {{1, 5, 32, 32}, {0, 2, 5, 4}, {1, 4, 28, 27}, {1, 1, 1, 1}, {0, 0, 0, 0}, {0, 0, 0, 0}, {}, {}, {}},
        {{1, 5, 32, 20}, {0, 1, 0, 0}, {1, 3, 32, 20}, {1, 1, 1, 1}, {0, 0, 0, 0}, {0, 0, 0, 0}, {}, {}, {}},
        {{2, 5, 32, 20}, {0, 0, 10, 0}, {1, 3, 20, 20}, {1, 1, 1, 1}, {0, 0, 0, 0}, {0, 1, 0, 0}, {}, {}, {}},
        {{1, 5, 32, 32}, {0, 0, 20, 20}, {1, 5, 25, 26}, {1, 1, 1, 2}, {0, 0, 0, 0}, {0, 0, 0, 0}, {}, {}, {}},
        {{2, 5, 32, 32}, {0, 0, 0, 20}, {1, 2, 30, 30}, {1, 1, 2, 1}, {0, 0, 0, 1}, {0, 1, 0, 1}, {}, {}, {}},
        {{1, 5, 32, 20}, {0, 0, 2, 10}, {1, 3, 32, 20}, {1, 1, 1, 1}, {0, 0, 1, 1}, {0, 0, 0, 0}, {}, {}, {}},
        {{2, 5, 32, 32}, {0, 1, 0, 10}, {1, 5, 32, 30}, {1, 1, 1, 1}, {0, 1, 0, 0}, {0, 0, 0, 0}, {}, {}, {}},
        {{1, 5, 32, 20}, {0, 1, 2, 10}, {1, 5, 32, 18}, {1, 1, 1, 2}, {0, 0, 1, 0}, {0, 0, 0, 1}, {}, {}, {}},
        {{2, 8, 32, 20}, {0, 0, 2, 10}, {1, 8, 32, 18}, {1, 2, 1, 2}, {0, 0, 1, 0}, {0, 0, 0, 1}, {}, {}, {}},
        {{2, 8, 32, 20}, {0, 0, 10}, {0, 32, 18}, {1, 1, 1}, {1, 1, 0}, {1, 1, 0}, {}, {}, {1, 0, 0}},
        {{2, 8, 32, 20}, {0, 0, 10}, {1, 0, 20}, {1, 1, 1}, {1, 1, 0}, {0, 1, 1}, {}, {}, {0, 1, 0}},
        {{2, 8, 32, 20}, {0, 4, 10}, {2, 8, 0}, {1, 1, 1}, {1, 0, 1}, {1, 1, 1}, {}, {}, {0, 0, 1}},
        {{1, 16, 32, 32}, {0, 0, 5, 4}, {1, 16, 28, 27}, {1, 1, 1, 1}, {0, 0, 0, 0}, {0, 0, 0, 0}, {}, {}, {}},
        {{2, 32, 10, 10}, {0, 16, 0, 0}, {1, 32, 10, 10}, {1, 1, 1, 1}, {0, 0, 0, 0}, {0, 0, 0, 0}, {}, {}, {}},
        {{1, 16, 32, 20}, {0, 0, 10, 0}, {1, 16, 20, 10}, {1, 1, 1, 1}, {0, 0, 0, 0}, {0, 0, 0, 1}, {}, {}, {}},
        {{2, 32, 32, 32}, {0, 0, 20, 20}, {1, 32, 25, 25}, {1, 1, 1, 1}, {0, 1, 0, 0}, {0, 1, 0, 0}, {}, {}, {}},
        {{1, 48, 32, 32}, {0, 16, 0, 20}, {1, 32, 32, 30}, {1, 1, 1, 2}, {1, 0, 1, 0}, {1, 0, 1, 0}, {}, {}, {}},
        {{2, 32, 32, 20}, {0, 16, 2, 10}, {1, 32, 32, 20}, {1, 1, 2, 1}, {0, 0, 0, 0}, {0, 0, 0, 0}, {}, {}, {}},
        {{2, 64, 32, 20}, {0, 16, 0, 0}, {2, 64, 32, 20}, {1, 1, 1, 1}, {0, 0, 0, 0}, {0, 0, 0, 0}, {}, {}, {}},
        {{2, 64, 32, 20}, {0, 32, 0, 0}, {2, 50, 32, 20}, {1, 1, 1, 1}, {0, 0, 0, 0}, {0, 0, 0, 0}, {}, {}, {}},
        {{2, 64, 32, 20}, {0, 0, 0, 0}, {2, 12, 32, 20}, {1, 1, 1, 1}, {0, 0, 0, 0}, {0, 0, 0, 0}, {}, {}, {}},
        {{1, 64, 32, 20}, {0, -16, 0, 10}, {2, 100, 32, 20}, {1, 1, 1, 1}, {0, 0, 0, 0}, {0, 0, 0, 0}, {}, {}, {}},
        {{2, 32, 32, 20}, {0, -16, 0, 0}, {2, -4, 32, 20}, {1, 1, 1, 1}, {0, 0, 0, 0}, {0, 0, 0, 0}, {}, {}, {}},
        {{2, 32, 32, 20}, {0, -32, 0, 0}, {2, -12, 32, 20}, {1, 1, 1, 1}, {0, 0, 0, 0}, {0, 0, 0, 0}, {}, {}, {}},
        {{2, 32, 32, 20}, {0, 10}, {0, 20}, {1, 1}, {1, 0}, {1, 0}, {}, {}, {1, 0}},
        {{2, 32, 32, 20}, {0, 16, 0}, {2, 32, 0}, {1, 1, 1}, {1, 0, 1}, {1, 1, 1}, {}, {}, {0, 0, 1}},
};

std::vector<StridedSliceSpecificParams> precommit_tests = {
        {{32, 20}, {2, 10}, {32, 20}, {1, 2}, {0, 1}, {1, 0}, {}, {}, {}},
        {{32, 32, 32}, {0, 0}, {0, 0}, {1, 2}, {1, 1}, {1, 1}, {0, 0}, {0, 1}, {0, 0}},
        {{2, 5, 32, 32}, {0, 0, 0, 20}, {1, 2, 30, 30}, {1, 1, 2, 1}, {0, 0, 0, 1}, {0, 1, 0, 1}, {}, {}, {}},
        {{5, 16, 16, 16}, {1, 4, 5, 10}, {0, 0, 0, 0}, {2, 7, 5, 3}, {0, 0, 0, 0}, {1, 1, 1, 1}, {}, {}, {}},
        {{1, 48, 32, 32}, {0, 16, 0, 20}, {1, 32, 32, 30}, {1, 1, 1, 2}, {1, 0, 1, 0}, {1, 0, 1, 0}, {}, {}, {}},
};

std::vector<StridedSliceSpecificParams> tests_5d = {
        {{1, 5, 20, 32, 32},
         {0, 0, 0, 0, 0},
         {1, 5, 20, 32, 32},
         {1, 1, 1, 1, 2},
         {0, 0, 0, 0, 0},
         {0, 0, 0, 0, 0},
         {},
         {},
         {}},
        {{1, 1, 1, 51, 1},
         {0, 0, 0, 0, 0},
         {0, 0, 1, 0, 0},
         {1, 1, 1, 2, 1},
         {1, 1, 0, 1, 1},
         {1, 1, 0, 1, 1},
         {0, 0, 0, 0, 0},
         {0, 0, 1, 0, 0},
         {0, 0, 0, 0, 0}},
};

std::vector<StridedSliceSpecificParams> tiling_tests = {
        {{1, 8, 80, 1280},
         {0, 0, 0, 0},
         {0, 0, 2147483647, 0},  // The 2147483647 value from ends is supported beacause of ResolveStridedSlice pass.
         {1, 1, 4, 1},
         {1, 1, 0, 1},
         {1, 1, 0, 1},
         {},
         {},
         {}},
};

std::vector<StridedSliceSpecificParams> precomit_tiling_tests = {
        {{1, 3, 640, 640},
         {0, 0, 0, 0},
         {0, 0, 2147483647, 0},  // The 2147483647 value from ends is supported beacause of ResolveStridedSlice pass.
         {1, 1, 2, 1},
         {1, 1, 1, 1},
         {1, 1, 0, 1},
         {},
         {},
         {}},
};

[[maybe_unused]] std::vector<StridedSliceSpecificParams> testsWithNegativeStrides = {
        {{10, 12}, {-1, 1}, {-9999, 0}, {-1, 1}, {0, 1}, {0, 1}, {0, 0}, {0, 0}, {0, 0}},
        {{1, 2, 4, 2}, {1, 0, 0, 0}, {1, 2, 4, 2}, {1, 1, -2, -1}, {1, 1, 1, 1}, {1, 1, 1, 1}, {}, {}, {}},
        {{2, 2, 4, 2}, {1, 0, 0, 0}, {1, 2, 4, 2}, {1, 1, -2, -1}, {0, 1, 1, 1}, {1, 1, 1, 1}, {}, {}, {}},
        {{5, 5, 5, 5}, {-1, 0, -1, 0}, {-50, 0, -60, 0}, {-1, 1, -1, 1}, {0, 0, 0, 0}, {0, 1, 0, 1}, {}, {}, {}},
        {{1, 12, 100}, {0, 9, 0}, {0, 7, 0}, {-1, -1, -1}, {1, 0, 1}, {1, 0, 1}, {}, {}, {}},
        {{1, 12, 100}, {0, 7, 0}, {0, 9, 0}, {-1, 1, -1}, {1, 0, 1}, {1, 0, 1}, {}, {}, {}},
        {{1, 12, 100}, {0, 4, 0}, {0, 9, 0}, {-1, 2, -1}, {1, 0, 1}, {1, 0, 1}, {}, {}, {}},
        {{1, 12, 100}, {0, 4, 0}, {0, 10, 0}, {-1, 2, -1}, {1, 0, 1}, {1, 0, 1}, {}, {}, {}},
        {{1, 12, 100}, {0, 9, 0}, {0, 4, 0}, {-1, -2, -1}, {1, 0, 1}, {1, 0, 1}, {}, {}, {}},
        {{1, 12, 100}, {0, 10, 0}, {0, 4, 0}, {-1, -2, -1}, {1, 0, 1}, {1, 0, 1}, {}, {}, {}},
        {{1, 12, 100}, {0, 11, 0}, {0, 0, 0}, {-1, -2, -1}, {1, 0, 1}, {1, 0, 1}, {}, {}, {}},
        {{1, 12, 100}, {0, -6, 0}, {0, -8, 0}, {-1, -2, -1}, {1, 0, 1}, {1, 0, 1}, {}, {}, {}},
};

const std::vector<InferenceEngine::Precision> InputPrecisions = {
        InferenceEngine::Precision::FP16,
        InferenceEngine::Precision::U8,
};

using Config = std::map<std::string, std::string>;

INSTANTIATE_TEST_SUITE_P(DISABLED_TMP_smoke_StridedSlice, VPUXStridedSliceLayerTest_VPU3700,
                         ::testing::Combine(::testing::ValuesIn(tests),
                                            ::testing::Values(InferenceEngine::Precision::FP16),
                                            ::testing::ValuesIn(InputPrecisions),
                                            ::testing::Values(InferenceEngine::Precision::UNSPECIFIED),
                                            ::testing::Values(InferenceEngine::Layout::ANY),
                                            ::testing::Values(InferenceEngine::Layout::ANY),
                                            ::testing::Values(LayerTestsUtils::testPlatformTargetDevice()),
                                            ::testing::Values(Config{})),
                         StridedSliceLayerTest::getTestCaseName);

INSTANTIATE_TEST_SUITE_P(smoke_precommit_StridedSlice, VPUXStridedSliceLayerTest_VPU3720,
                         ::testing::Combine(::testing::ValuesIn(precommit_tests),
                                            ::testing::Values(InferenceEngine::Precision::FP16),
                                            ::testing::ValuesIn(InputPrecisions),
                                            ::testing::Values(InferenceEngine::Precision::UNSPECIFIED),
                                            ::testing::Values(InferenceEngine::Layout::ANY),
                                            ::testing::Values(InferenceEngine::Layout::ANY),
                                            ::testing::Values(LayerTestsUtils::testPlatformTargetDevice()),
                                            ::testing::Values(Config{})),
                         StridedSliceLayerTest::getTestCaseName);

INSTANTIATE_TEST_SUITE_P(smoke_StridedSlice_5D, VPUXStridedSliceLayerTest_VPU3720,
                         ::testing::Combine(::testing::ValuesIn(tests_5d),
                                            ::testing::Values(InferenceEngine::Precision::FP16),
                                            ::testing::ValuesIn(InputPrecisions),
                                            ::testing::Values(InferenceEngine::Precision::UNSPECIFIED),
                                            ::testing::Values(InferenceEngine::Layout::ANY),
                                            ::testing::Values(InferenceEngine::Layout::ANY),
                                            ::testing::Values(LayerTestsUtils::testPlatformTargetDevice()),
                                            ::testing::Values(Config{})),
                         StridedSliceLayerTest::getTestCaseName);

INSTANTIATE_TEST_SUITE_P(
        smoke_tiling_StridedSlice, VPUXStridedSliceLayerTilingTest_VPU3720,
        ::testing::Combine(::testing::ValuesIn(tiling_tests), ::testing::Values(InferenceEngine::Precision::FP16),
                           ::testing::ValuesIn(InputPrecisions), ::testing::Values(InferenceEngine::Precision::FP16),
                           ::testing::Values(InferenceEngine::Layout::ANY),
                           ::testing::Values(InferenceEngine::Layout::ANY),
                           ::testing::Values(LayerTestsUtils::testPlatformTargetDevice()), ::testing::Values(Config{})),
        StridedSliceLayerTest::getTestCaseName);

INSTANTIATE_TEST_SUITE_P(smoke_precommit_tiling_StridedSlice, VPUXStridedSliceLayerTilingTest_VPU3720,
                         ::testing::Combine(::testing::ValuesIn(precomit_tiling_tests),
                                            ::testing::Values(InferenceEngine::Precision::FP16),
                                            ::testing::ValuesIn(InputPrecisions),
                                            ::testing::Values(InferenceEngine::Precision::FP16),
                                            ::testing::Values(InferenceEngine::Layout::ANY),
                                            ::testing::Values(InferenceEngine::Layout::ANY),
                                            ::testing::Values(LayerTestsUtils::testPlatformTargetDevice()),
                                            ::testing::Values(Config{})),
                         StridedSliceLayerTest::getTestCaseName);

}  // namespace
