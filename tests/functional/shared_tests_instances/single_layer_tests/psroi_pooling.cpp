//
// Copyright (C) 2022 Intel Corporation.
// SPDX-License-Identifier: Apache 2.0
//

#include <vector>

#include <common/functions.h>
#include "common_test_utils/test_constants.hpp"
#include "single_layer_tests/psroi_pooling.hpp"
#include "vpu_ov1_layer_test.hpp"

namespace LayerTestsDefinitions {
class VPUXPSROIPoolingLayerTest : public PSROIPoolingLayerTest, virtual public LayerTestsUtils::VpuOv1LayerTestsCommon {
    void SkipBeforeLoad() override {
        std::string psROIPoolingMode;
        std::tie(std::ignore, std::ignore, std::ignore, std::ignore, std::ignore, std::ignore, std::ignore,
                 psROIPoolingMode, std::ignore, targetDevice) = this->GetParam();
        if (psROIPoolingMode == "bilinear") {
            throw LayerTestsUtils::VpuSkipTestException("BILINEAR mode is unsupported for now");
        }
    }
};

class VPUXPSROIPoolingLayerTest_VPU3700 : public VPUXPSROIPoolingLayerTest {};
class VPUXPSROIPoolingLayerTest_VPU3720 : public VPUXPSROIPoolingLayerTest {};

TEST_P(VPUXPSROIPoolingLayerTest_VPU3700, HW) {
    setPlatformVPU3700();
    setDefaultHardwareModeMLIR();
    Run();
}

TEST_P(VPUXPSROIPoolingLayerTest_VPU3720, HW) {
    setPlatformVPU3720();
    setDefaultHardwareModeMLIR();
    Run();
}

}  // namespace LayerTestsDefinitions

using namespace LayerTestsDefinitions;

const std::vector<InferenceEngine::Precision> netPrecisions = {InferenceEngine::Precision::FP32,
                                                               InferenceEngine::Precision::FP16};

const std::vector<std::vector<size_t>> inputShapeVector0 = {
        {2, 200, 20, 20}, {2, 200, 20, 16}, {2, 200, 16, 20}, {3, 200, 16, 16}};
const std::vector<std::vector<size_t>> inputShapeVector1 = {{1, 392, 14, 14}, {1, 392, 38, 64}};
const std::vector<std::vector<size_t>> inputShapeVector2 = {{1, 49 * 1, 14, 14}};
const std::vector<std::vector<size_t>> inputShapeVector3 = {{1, 3240, 38, 38}};

const std::vector<std::vector<size_t>> coordShapesVector0 = {{1, 5}};
const std::vector<std::vector<size_t>> coordShapesVector1 = {{300, 5}};
const std::vector<std::vector<size_t>> coordShapesVector2 = {{100, 5}};

const auto paramsAvg0 = testing::Combine(::testing::ValuesIn(inputShapeVector0),   // input
                                         ::testing::ValuesIn(coordShapesVector0),  // coord
                                         ::testing::Values(50),                    // outputDim
                                         ::testing::Values(2),                     // groupSize
                                         ::testing::Values(1.0f),                  // spatialScale
                                         ::testing::Values(1),                     // spatialBinX
                                         ::testing::Values(1),                     // spatialBinY
                                         ::testing::Values("average"),             // mode
                                         ::testing::ValuesIn(netPrecisions),
                                         ::testing::Values(LayerTestsUtils::testPlatformTargetDevice()));

const auto paramsAvg1 = testing::Combine(::testing::ValuesIn(inputShapeVector1),   // input
                                         ::testing::ValuesIn(coordShapesVector1),  // coord
                                         ::testing::Values(8),                     // outputDim
                                         ::testing::Values(7),                     // groupSize
                                         ::testing::Values(0.0625f),               // spatialScale
                                         ::testing::Values(1),                     // spatialBinX
                                         ::testing::Values(1),                     // spatialBinY
                                         ::testing::Values("average"),             // mode
                                         ::testing::ValuesIn(netPrecisions),
                                         ::testing::Values(LayerTestsUtils::testPlatformTargetDevice()));

const auto paramsAvg2 = testing::Combine(::testing::ValuesIn(inputShapeVector2),   // input
                                         ::testing::ValuesIn(coordShapesVector0),  // coord
                                         ::testing::Values(1),                     // outputDim
                                         ::testing::Values(7),                     // groupSize
                                         ::testing::Values(0.0625f),               // spatialScale
                                         ::testing::Values(1),                     // spatialBinX
                                         ::testing::Values(1),                     // spatialBinY
                                         ::testing::Values("average"),             // mode
                                         ::testing::ValuesIn(netPrecisions),
                                         ::testing::Values(LayerTestsUtils::testPlatformTargetDevice()));

const auto paramsBilinear = testing::Combine(::testing::ValuesIn(inputShapeVector3),   // input
                                             ::testing::ValuesIn(coordShapesVector2),  // coord
                                             ::testing::Values(360),                   // outputDim
                                             ::testing::Values(6),                     // groupSize
                                             ::testing::Values(1.0f),                  // spatialScale
                                             ::testing::Values(3),                     // spatialBinX
                                             ::testing::Values(3),                     // spatialBinY
                                             ::testing::Values("bilinear"),            // mode
                                             ::testing::ValuesIn(netPrecisions),
                                             ::testing::Values(LayerTestsUtils::testPlatformTargetDevice()));

// --------- VPU3700 ---------
INSTANTIATE_TEST_SUITE_P(smoke_PSROIPoolingBiliniarLayoutTest0, VPUXPSROIPoolingLayerTest_VPU3700, paramsBilinear,
                         VPUXPSROIPoolingLayerTest_VPU3700::getTestCaseName);

INSTANTIATE_TEST_SUITE_P(smoke_PSROIPoolingAverageLayoutTest0, VPUXPSROIPoolingLayerTest_VPU3700, paramsAvg0,
                         VPUXPSROIPoolingLayerTest_VPU3700::getTestCaseName);

INSTANTIATE_TEST_SUITE_P(smoke_PSROIPoolingAverageLayoutTest1, VPUXPSROIPoolingLayerTest_VPU3700, paramsAvg1,
                         VPUXPSROIPoolingLayerTest_VPU3700::getTestCaseName);

INSTANTIATE_TEST_SUITE_P(smoke_PSROIPoolingAverageLayoutTest2, VPUXPSROIPoolingLayerTest_VPU3700, paramsAvg2,
                         VPUXPSROIPoolingLayerTest_VPU3700::getTestCaseName);

// --------- VPU3720 ---------
INSTANTIATE_TEST_SUITE_P(smoke_PSROIPoolingAverageLayoutTest0, VPUXPSROIPoolingLayerTest_VPU3720, paramsAvg0,
                         VPUXPSROIPoolingLayerTest_VPU3720::getTestCaseName);

INSTANTIATE_TEST_SUITE_P(smoke_PSROIPoolingAverageLayoutTest1, VPUXPSROIPoolingLayerTest_VPU3720, paramsAvg2,
                         VPUXPSROIPoolingLayerTest_VPU3720::getTestCaseName);

INSTANTIATE_TEST_SUITE_P(smoke_PSROIPoolingBiliniarLayoutTest0, VPUXPSROIPoolingLayerTest_VPU3720, paramsBilinear,
                         VPUXPSROIPoolingLayerTest_VPU3720::getTestCaseName);
