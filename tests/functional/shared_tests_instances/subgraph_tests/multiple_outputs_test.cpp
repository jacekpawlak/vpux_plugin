//
// Copyright (C) 2022-2023 Intel Corporation.
// SPDX-License-Identifier: Apache 2.0
//

#include "shared_test_classes/subgraph/multiple_outputs.hpp"
#include "common_test_utils/test_constants.hpp"
// #include "vpux_private_config.hpp"

#include <vector>

#include "vpu_ov1_layer_test.hpp"

namespace SubgraphTestsDefinitions {
class VPUXMultipleoutputTest : public MultioutputTest, virtual public LayerTestsUtils::VpuOv1LayerTestsCommon {};

class VPUXMultipleoutputTest_VPU3700 : public VPUXMultipleoutputTest {
    /* tests dumping intermediate outputs

        input
          |
        conv1 -> Output
          |
        conv2
          |
        Pool
          |
        output
    */
};

TEST_P(VPUXMultipleoutputTest_VPU3700, HW) {
    setPlatformVPU3700();
    setDefaultHardwareModeMLIR();
    Run();
};

}  // namespace SubgraphTestsDefinitions

using namespace SubgraphTestsDefinitions;
using namespace ngraph::helpers;

namespace {

const std::vector<InferenceEngine::Precision> netPrecisions = {InferenceEngine::Precision::FP16};

const std::vector<std::map<std::string, std::string>> configs = {{{"LOG_LEVEL", "LOG_INFO"}}};

std::vector<convParams> convParams = {
        std::make_tuple(std::vector<size_t>{1, 3, 16, 16},  // InputShape
                        std::vector<size_t>{3, 3},          // KernelShape
                        1)                                  // Stride
};

std::vector<size_t> outputChannels = {16};

INSTANTIATE_TEST_SUITE_P(smoke_MultipleOutputs, VPUXMultipleoutputTest_VPU3700,
                         ::testing::Combine(::testing::ValuesIn(netPrecisions),
                                            ::testing::Values(LayerTestsUtils::testPlatformTargetDevice()),
                                            ::testing::ValuesIn(configs), ::testing::ValuesIn(convParams),
                                            ::testing::ValuesIn(outputChannels)),
                         MultioutputTest::getTestCaseName);
}  // namespace
