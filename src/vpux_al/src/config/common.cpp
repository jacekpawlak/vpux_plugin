//
// Copyright (C) 2022 Intel Corporation.
// SPDX-License-Identifier: Apache 2.0
//

#include "vpux/al/config/common.hpp"

using namespace vpux;
using namespace ov::intel_vpux;

//
// register
//

void vpux::registerCommonOptions(OptionsDesc& desc) {
    desc.add<PERFORMANCE_HINT>();
    desc.add<PERFORMANCE_HINT_NUM_REQUESTS>();
    desc.add<PERF_COUNT>();
    desc.add<LOG_LEVEL>();
    desc.add<PLATFORM>();
    desc.add<DEVICE_ID>();
    desc.add<CACHE_DIR>();
}

//
// PERFORMANCE_HINT
//

StringLiteral ov::hint::stringifyEnum(PerformanceMode val) {
    switch (val) {
    case PerformanceMode::LATENCY:
        return "LATENCY";
    case PerformanceMode::THROUGHPUT:
        return "THROUGHPUT";
    case PerformanceMode::CUMULATIVE_THROUGHPUT:
        return "CUMULATIVE_THROUGHPUT";
    case PerformanceMode::UNDEFINED:
        return "LATENCY";
    default:
        return "<UNKNOWN>";
    }
}

ov::hint::PerformanceMode vpux::PERFORMANCE_HINT::parse(StringRef val) {
    if (val == "LATENCY") {
        return ov::hint::PerformanceMode::LATENCY;
    } else if (val == "THROUGHPUT") {
        return ov::hint::PerformanceMode::THROUGHPUT;
    } else if (val == "CUMULATIVE_THROUGHPUT") {
        return ov::hint::PerformanceMode::CUMULATIVE_THROUGHPUT;
    } else if (val.empty() || val == "UNDEFINED") {
        return ov::hint::PerformanceMode::LATENCY;
    }

    VPUX_THROW("Value '{0}' is not a valid PERFORMANCE_HINT option", val);
}

//
// LOG_LEVEL
//

ov::log::Level vpux::cvtLogLevel(LogLevel lvl) {
    switch (lvl) {
    case LogLevel::None:
        return ov::log::Level::NO;
    case LogLevel::Fatal:
    case LogLevel::Error:
        return ov::log::Level::ERR;
    case LogLevel::Warning:
        return ov::log::Level::WARNING;
    case LogLevel::Info:
        return ov::log::Level::INFO;
    case LogLevel::Debug:
        return ov::log::Level::DEBUG;
    case LogLevel::Trace:
        return ov::log::Level::TRACE;
    default:
        return ov::log::Level::NO;
    }
}

//
// PLATFORM
//

StringLiteral InferenceEngine::VPUXConfigParams::stringifyEnum(VPUXPlatform val) {
    switch (val) {
    case VPUXPlatform::AUTO_DETECT:
        return "AUTO_DETECT";
    case VPUXPlatform::EMULATOR:
        return "EMULATOR";
    case VPUXPlatform::VPU3700:
        return "VPU3700";
    case VPUXPlatform::VPU3720:
        return "VPU3720";
    default:
        return "<UNKNOWN>";
    }
}

InferenceEngine::VPUXConfigParams::VPUXPlatform vpux::PLATFORM::parse(StringRef val) {
    if (val == "AUTO_DETECT") {
        return ov::intel_vpux::cvtVPUXPlatform(ov::intel_vpux::VPUXPlatform::AUTO_DETECT);
    } else if (val == "3700" || val == "VPU3700" || val == "NPU3700") {
        return ov::intel_vpux::cvtVPUXPlatform(ov::intel_vpux::VPUXPlatform::VPU3700);
    } else if (val == "3720" || val == "VPU3720" || val == "NPU3720") {
        return ov::intel_vpux::cvtVPUXPlatform(ov::intel_vpux::VPUXPlatform::VPU3720);
    } else if (val == "3700_EMU" || val == "3720_EMU") {
        return ov::intel_vpux::cvtVPUXPlatform(ov::intel_vpux::VPUXPlatform::EMULATOR);
    }

    VPUX_THROW("Value '{0}' is not a valid PLATFORM option", val);
}
