//
// Copyright (C) 2022 Intel Corporation.
// SPDX-License-Identifier: Apache 2.0
//

#pragma once

#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "ngraph_functions/builders.hpp"
#include "ngraph_functions/utils/ngraph_helpers.hpp"
#include "shared_test_classes/base/layer_test_utils.hpp"

namespace SubgraphTestsDefinitions {

typedef std::tuple<std::vector<size_t>,  // Input Shapes
                   std::vector<size_t>,  // Kernel Shape
                   size_t                // Stride
                   >
        convParams;

typedef std::tuple<InferenceEngine::Precision,          // Network Precision
                   std::string,                         // Target Device
                   std::map<std::string, std::string>,  // Configuration
                   convParams,                          // Convolution Params
                   size_t                               // Output Channels
                   >
        multiOutputTestParams;

class MultioutputTest :
        public testing::WithParamInterface<multiOutputTestParams>,
        virtual public LayerTestsUtils::LayerTestsCommon {
public:
    static std::string getTestCaseName(testing::TestParamInfo<multiOutputTestParams> obj);
    InferenceEngine::Blob::Ptr GenerateInput(const InferenceEngine::InputInfo& info) const override;

protected:
    void SetUp() override;
};

}  // namespace SubgraphTestsDefinitions
