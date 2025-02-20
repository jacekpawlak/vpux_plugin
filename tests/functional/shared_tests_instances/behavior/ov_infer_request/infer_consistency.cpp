//
// Copyright (C) 2018-2023 Intel Corporation.
// SPDX-License-Identifier: Apache 2.0
//

#include <string>
#include <vector>

#include "behavior/ov_infer_request/infer_consistency.hpp"

using namespace ov::test::behavior;

namespace {
// for deviceConfigs, the deviceConfigs[0] is target device which need to be tested.
// deviceConfigs[1], deviceConfigs[2],deviceConfigs[n] are the devices which will
// be compared with target device, the result of target should be in one of the compared
// device.
using Configs = std::vector<std::pair<std::string, ov::AnyMap>>;

auto configs = []() {
    return std::vector<Configs>{{{CommonTestUtils::DEVICE_KEEMBAY, {}}, {CommonTestUtils::DEVICE_KEEMBAY, {}}}};
};

auto AutoConfigs = []() {
    return std::vector<Configs>{{{CommonTestUtils::DEVICE_AUTO + std::string(":") + CommonTestUtils::DEVICE_KEEMBAY,
                                  {ov::hint::performance_mode(ov::hint::PerformanceMode::LATENCY)}},
                                 {CommonTestUtils::DEVICE_KEEMBAY, {}}},
                                {{CommonTestUtils::DEVICE_AUTO + std::string(":") + CommonTestUtils::DEVICE_KEEMBAY,
                                  {ov::hint::performance_mode(ov::hint::PerformanceMode::THROUGHPUT)}},
                                 {CommonTestUtils::DEVICE_KEEMBAY, {}}},
                                {{CommonTestUtils::DEVICE_AUTO + std::string(":") + CommonTestUtils::DEVICE_KEEMBAY,
                                  {ov::hint::performance_mode(ov::hint::PerformanceMode::CUMULATIVE_THROUGHPUT)}},
                                 {CommonTestUtils::DEVICE_KEEMBAY, {}}},
                                {{CommonTestUtils::DEVICE_AUTO + std::string(":") + CommonTestUtils::DEVICE_KEEMBAY +
                                          "," + CommonTestUtils::DEVICE_CPU,
                                  {ov::hint::performance_mode(ov::hint::PerformanceMode::LATENCY)}},
                                 {CommonTestUtils::DEVICE_KEEMBAY, {}},
                                 {CommonTestUtils::DEVICE_CPU, {}}},
                                {{CommonTestUtils::DEVICE_AUTO + std::string(":") + CommonTestUtils::DEVICE_KEEMBAY +
                                          "," + CommonTestUtils::DEVICE_CPU,
                                  {ov::hint::performance_mode(ov::hint::PerformanceMode::THROUGHPUT)}},
                                 {CommonTestUtils::DEVICE_KEEMBAY, {}},
                                 {CommonTestUtils::DEVICE_CPU, {}}},
                                {{CommonTestUtils::DEVICE_AUTO + std::string(":") + CommonTestUtils::DEVICE_KEEMBAY +
                                          "," + CommonTestUtils::DEVICE_CPU,
                                  {ov::hint::performance_mode(ov::hint::PerformanceMode::CUMULATIVE_THROUGHPUT)}},
                                 {CommonTestUtils::DEVICE_KEEMBAY, {}},
                                 {CommonTestUtils::DEVICE_CPU, {}}},
                                {{CommonTestUtils::DEVICE_AUTO + std::string(":") + CommonTestUtils::DEVICE_CPU + "," +
                                          CommonTestUtils::DEVICE_KEEMBAY,
                                  {ov::hint::performance_mode(ov::hint::PerformanceMode::CUMULATIVE_THROUGHPUT)}},
                                 {CommonTestUtils::DEVICE_CPU, {}},
                                 {CommonTestUtils::DEVICE_KEEMBAY, {}}}};
};

auto MultiConfigs = []() {
    return std::vector<Configs>{{{CommonTestUtils::DEVICE_MULTI + std::string(":") + CommonTestUtils::DEVICE_KEEMBAY,
                                  {ov::hint::performance_mode(ov::hint::PerformanceMode::LATENCY)}},
                                 {CommonTestUtils::DEVICE_KEEMBAY, {}}},
                                {{CommonTestUtils::DEVICE_MULTI + std::string(":") + CommonTestUtils::DEVICE_KEEMBAY,
                                  {ov::hint::performance_mode(ov::hint::PerformanceMode::THROUGHPUT)}},
                                 {CommonTestUtils::DEVICE_KEEMBAY, {}}},
                                {{CommonTestUtils::DEVICE_MULTI + std::string(":") + CommonTestUtils::DEVICE_KEEMBAY +
                                          "," + CommonTestUtils::DEVICE_CPU,
                                  {ov::hint::performance_mode(ov::hint::PerformanceMode::LATENCY)}},
                                 {CommonTestUtils::DEVICE_KEEMBAY, {}},
                                 {CommonTestUtils::DEVICE_CPU, {}}},
                                {{CommonTestUtils::DEVICE_MULTI + std::string(":") + CommonTestUtils::DEVICE_KEEMBAY +
                                          "," + CommonTestUtils::DEVICE_CPU,
                                  {ov::hint::performance_mode(ov::hint::PerformanceMode::THROUGHPUT)}},
                                 {CommonTestUtils::DEVICE_KEEMBAY, {}},
                                 {CommonTestUtils::DEVICE_CPU, {}}}};
};

// 3x5 configuration takes ~65 seconds to run, which is already pretty long time
INSTANTIATE_TEST_SUITE_P(smoke_BehaviorTests, OVInferConsistencyTest,
                         ::testing::Combine(::testing::Values(3),  // inferRequest num
                                            ::testing::Values(5),  // infer counts
                                            ::testing::ValuesIn(configs())),
                         OVInferConsistencyTest::getTestCaseName);

INSTANTIATE_TEST_SUITE_P(smoke_Auto_BehaviorTests, OVInferConsistencyTest,
                         ::testing::Combine(::testing::Values(3),  // inferRequest num
                                            ::testing::Values(5),  // infer counts
                                            ::testing::ValuesIn(AutoConfigs())),
                         OVInferConsistencyTest::getTestCaseName);

INSTANTIATE_TEST_SUITE_P(smoke_Multi_BehaviorTests, OVInferConsistencyTest,
                         ::testing::Combine(::testing::Values(3),  // inferRequest num
                                            ::testing::Values(5),  // infer counts
                                            ::testing::ValuesIn(MultiConfigs())),
                         OVInferConsistencyTest::getTestCaseName);
}  // namespace
