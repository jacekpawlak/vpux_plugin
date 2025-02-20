//
// Copyright (C) 2022-2023 Intel Corporation.
// SPDX-License-Identifier: Apache 2.0
//

#include "vpu_ov1_layer_test.hpp"
#include "vpu_ov2_layer_test.hpp"

#include <ngraph_functions/builders.hpp>
#include <ngraph_functions/utils/ngraph_helpers.hpp>
#include <shared_test_classes/base/layer_test_utils.hpp>

namespace {

struct SubtractTestParams {
    LayerTestsUtils::TargetDevice device;
    InferenceEngine::SizeVector input1_shape;
    InferenceEngine::SizeVector input2_shape;
};

//
//         [DeclareOp]
//             |
//         [Subtract] --- [DeclareOp]
//             |
//          [output]
//

class VPUXSubtractSubGraphTest_VPU3720 :
        public LayerTestsUtils::VpuOv1LayerTestsCommon,
        public testing::WithParamInterface<SubtractTestParams> {
public:
    void SetUp() override {
        const auto test_params = GetParam();
        targetDevice = test_params.device;

        const InferenceEngine::SizeVector input1Shape = test_params.input1_shape;
        const InferenceEngine::SizeVector input2Shape = test_params.input2_shape;

        const auto params = ngraph::builder::makeParams(ngraph::element::f16, {input1Shape, input2Shape});
        const auto paramOuts =
                ngraph::helpers::convert2OutputVector(ngraph::helpers::castOps2Nodes<ngraph::op::Parameter>(params));

        const auto subtract = std::make_shared<ngraph::op::v1::Subtract>(paramOuts[0], paramOuts[1]);

        const ngraph::ResultVector results{std::make_shared<ngraph::opset1::Result>(subtract)};

        function = std::make_shared<ngraph::Function>(results, params, "SubtractTest");

        threshold = 0.5f;
    }
};

TEST_P(VPUXSubtractSubGraphTest_VPU3720, HW) {
    setPlatformVPU3720();
    setDefaultHardwareModeMLIR();
    Run();
}

INSTANTIATE_TEST_SUITE_P(smoke_subtract_same_shape_const_inputs, VPUXSubtractSubGraphTest_VPU3720,
                         ::testing::Values(SubtractTestParams{
                                 LayerTestsUtils::testPlatformTargetDevice(),  // _device
                                 {1, 1, 2, 2},                                 // input1 shape
                                 {1, 1, 2, 2},                                 // input2 shape
                         }));

INSTANTIATE_TEST_SUITE_P(DISABLED_TMP_smoke_subtract_diff_shape_const_inputs, VPUXSubtractSubGraphTest_VPU3720,
                         ::testing::Values(SubtractTestParams{
                                 LayerTestsUtils::testPlatformTargetDevice(),  // _device
                                 {1, 1, 2, 1},                                 // input1 shape
                                 {1, 1, 2, 4},                                 // input2 shape
                         }));

//
//         [DeclareOp]
//             |
//       [FakeQuantize]
//             |
//         [Subtract] --- [FakeQuantize] --- [DeclareOp]
//             |
//       [FakeQuantize]
//             |
//          [output]
//

class VPUXSubtractFqSubGraphTest_VPU3720 :
        public LayerTestsUtils::VpuOv1LayerTestsCommon,
        public testing::WithParamInterface<SubtractTestParams> {
public:
    void SetUp() override {
        const auto test_params = GetParam();
        targetDevice = test_params.device;

        const InferenceEngine::SizeVector input1Shape = test_params.input1_shape;
        const InferenceEngine::SizeVector input2Shape = test_params.input2_shape;

        const auto params = ngraph::builder::makeParams(ngraph::element::f16, {input1Shape, input2Shape});
        const auto paramOuts =
                ngraph::helpers::convert2OutputVector(ngraph::helpers::castOps2Nodes<ngraph::op::Parameter>(params));

        const size_t dataLevels = 255;
        const std::vector<float> dataLow = {0.0f};
        const std::vector<float> dataHigh = {255.0f};

        const auto input1 = ngraph::builder::makeFakeQuantize(paramOuts[0], ngraph::element::f16, dataLevels, {},
                                                              dataLow, dataHigh, dataLow, dataHigh);
        const auto input2 = ngraph::builder::makeFakeQuantize(paramOuts[0], ngraph::element::f16, dataLevels, {},
                                                              dataLow, dataHigh, dataLow, dataHigh);

        const auto subtract = std::make_shared<ngraph::op::v1::Subtract>(input2, input2);
        const auto subtractFq = ngraph::builder::makeFakeQuantize(subtract, ngraph::element::f16, dataLevels, {},
                                                                  dataLow, dataHigh, dataLow, dataHigh);

        const ngraph::ResultVector results{std::make_shared<ngraph::opset1::Result>(subtractFq)};

        function = std::make_shared<ngraph::Function>(results, params, "SubtractTest");

        threshold = 0.5f;
    }
};

TEST_P(VPUXSubtractFqSubGraphTest_VPU3720, HW) {
    setPlatformVPU3720();
    setDefaultHardwareModeMLIR();
    Run();
}

INSTANTIATE_TEST_SUITE_P(smoke_subtract_same_shape_const_fq_inputs, VPUXSubtractFqSubGraphTest_VPU3720,
                         ::testing::Values(SubtractTestParams{
                                 LayerTestsUtils::testPlatformTargetDevice(),  // _device
                                 {1, 1, 2, 2},                                 // input1 shape
                                 {1, 1, 2, 2},                                 // input2 shape
                         }));

INSTANTIATE_TEST_SUITE_P(smoke_subtract_diff_shape_const_fq_inputs, VPUXSubtractFqSubGraphTest_VPU3720,
                         ::testing::Values(SubtractTestParams{
                                 LayerTestsUtils::testPlatformTargetDevice(),  // _device
                                 {1, 1, 2, 4},                                 // input1 shape
                                 {1, 1, 2, 1},                                 // input2 shape
                         }));

//
//         [DeclareOp]
//             |
//           [ReLU]
//             |
//         [Subtract] --- [ReLU] --- [DeclareOp]
//             |
//          [output]
//

class VPUXSubtractActInputsSubGraphTest_VPU3720 :
        public LayerTestsUtils::VpuOv1LayerTestsCommon,
        public testing::WithParamInterface<SubtractTestParams> {
public:
    void SetUp() override {
        const auto test_params = GetParam();
        targetDevice = test_params.device;

        const InferenceEngine::SizeVector input1Shape = test_params.input1_shape;
        const InferenceEngine::SizeVector input2Shape = test_params.input2_shape;

        const auto params = ngraph::builder::makeParams(ngraph::element::f16, {input1Shape, input2Shape});
        const auto paramOuts =
                ngraph::helpers::convert2OutputVector(ngraph::helpers::castOps2Nodes<ngraph::op::Parameter>(params));

        const auto relu1 = std::make_shared<ngraph::op::v0::Relu>(paramOuts[0]);
        const auto relu2 = std::make_shared<ngraph::op::v0::Relu>(paramOuts[1]);

        const auto subtract = std::make_shared<ngraph::op::v1::Subtract>(relu1, relu2);

        const ngraph::ResultVector results{std::make_shared<ngraph::opset1::Result>(subtract)};

        function = std::make_shared<ngraph::Function>(results, params, "SubtractTest");

        threshold = 0.5f;
    }
};

TEST_P(VPUXSubtractActInputsSubGraphTest_VPU3720, HW) {
    setPlatformVPU3720();
    setDefaultHardwareModeMLIR();
    Run();
}

INSTANTIATE_TEST_SUITE_P(smoke_subtract_same_shapes_act_inputs, VPUXSubtractActInputsSubGraphTest_VPU3720,
                         ::testing::Values(SubtractTestParams{
                                 LayerTestsUtils::testPlatformTargetDevice(),  // _device
                                 {1, 1, 2, 2},                                 // input1 shape
                                 {1, 1, 2, 2},                                 // input2 shape
                         }));

INSTANTIATE_TEST_SUITE_P(smoke_subtract_diff_shapes_act_inputs, VPUXSubtractActInputsSubGraphTest_VPU3720,
                         ::testing::Values(SubtractTestParams{
                                 LayerTestsUtils::testPlatformTargetDevice(),  // _device
                                 {1, 1, 2, 4},                                 // input1 shape
                                 {1, 1, 2, 1},                                 // input2 shape
                         }));

//
//         [DeclareOp]
//             |
//           [ReLU]
//             |
//       [FakeQuantize]
//             |
//         [Subtract] --- [FakeQuantize] --- [ReLU] --- [DeclareOp]
//             |
//       [FakeQuantize]
//             |
//          [output]
//

class VPUXSubtractFqActInputsSubGraphTest_VPU3720 :
        public LayerTestsUtils::VpuOv1LayerTestsCommon,
        public testing::WithParamInterface<SubtractTestParams> {
public:
    void SetUp() override {
        const auto test_params = GetParam();
        targetDevice = test_params.device;

        const InferenceEngine::SizeVector input1Shape = test_params.input1_shape;
        const InferenceEngine::SizeVector input2Shape = test_params.input2_shape;

        const auto params = ngraph::builder::makeParams(ngraph::element::f16, {input1Shape, input2Shape});
        const auto paramOuts =
                ngraph::helpers::convert2OutputVector(ngraph::helpers::castOps2Nodes<ngraph::op::Parameter>(params));

        const size_t dataLevels = 255;
        const std::vector<float> dataLow = {0.0f};
        const std::vector<float> dataHigh = {255.0f};

        const auto relu1 = std::make_shared<ngraph::op::v0::Relu>(paramOuts[0]);
        const auto relu1Fq = ngraph::builder::makeFakeQuantize(relu1, ngraph::element::f16, dataLevels, {}, dataLow,
                                                               dataHigh, dataLow, dataHigh);

        const auto relu2 = std::make_shared<ngraph::op::v0::Relu>(paramOuts[1]);
        const auto relu2Fq = ngraph::builder::makeFakeQuantize(relu2, ngraph::element::f16, dataLevels, {}, dataLow,
                                                               dataHigh, dataLow, dataHigh);

        const auto subtract = std::make_shared<ngraph::op::v1::Subtract>(relu1Fq->output(0), relu2Fq->output(0));
        const auto outFq = ngraph::builder::makeFakeQuantize(subtract, ngraph::element::f16, dataLevels, {}, dataLow,
                                                             dataHigh, dataLow, dataHigh);

        const ngraph::ResultVector results{std::make_shared<ngraph::opset1::Result>(outFq)};
        function = std::make_shared<ngraph::Function>(results, params, "SubtractTest");

        threshold = 0.5f;
    }
};

TEST_P(VPUXSubtractFqActInputsSubGraphTest_VPU3720, HW) {
    setPlatformVPU3720();
    setDefaultHardwareModeMLIR();
    Run();
}

INSTANTIATE_TEST_SUITE_P(smoke_subtract_same_shapes_fq_act_inputs, VPUXSubtractFqActInputsSubGraphTest_VPU3720,
                         ::testing::Values(SubtractTestParams{
                                 LayerTestsUtils::testPlatformTargetDevice(),  // _device
                                 {1, 1, 2, 2},                                 // input1 shape
                                 {1, 1, 2, 2},                                 // input2 shape
                         }));

INSTANTIATE_TEST_SUITE_P(DISABLED_TMP_smoke_subtract_diff_shapes_fq_act_inputs,
                         VPUXSubtractFqActInputsSubGraphTest_VPU3720,
                         ::testing::Values(SubtractTestParams{
                                 LayerTestsUtils::testPlatformTargetDevice(),  // _device
                                 {1, 1, 2, 1},                                 // input1 shape
                                 {1, 1, 2, 4},                                 // input2 shape
                         }));

}  // namespace
