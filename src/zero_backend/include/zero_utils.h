//
// Copyright (C) 2022 Intel Corporation.
// SPDX-License-Identifier: Apache 2.0
//

#pragma once

#include "vpux/al/config/runtime.hpp"
#include "ze_api.h"
#include "ze_graph_ext.h"

#include <string>

#include <ie_common.h>

namespace vpux {
namespace zeroUtils {

std::string result_to_string(const ze_result_t result);

static inline void throwOnFail(const std::string& step, const ze_result_t result) {
    if (ZE_RESULT_SUCCESS != result) {
        IE_THROW() << "L0 " << step << " result: " << result_to_string(result) << ", code 0x" << std::hex
                   << uint64_t(result);
    }
}

static inline ze_command_queue_priority_t toZeQueuePriority(const ov::hint::Priority& val) {
    switch (val) {
    case ov::hint::Priority::LOW:
        return ZE_COMMAND_QUEUE_PRIORITY_PRIORITY_LOW;
    case ov::hint::Priority::MEDIUM:
        return ZE_COMMAND_QUEUE_PRIORITY_NORMAL;
    case ov::hint::Priority::HIGH:
        return ZE_COMMAND_QUEUE_PRIORITY_PRIORITY_HIGH;
    default:
        IE_THROW() << "Incorrect queue priority.";
    }
}

static inline std::size_t precisionToSize(const ze_graph_argument_precision_t val) {
    switch (val) {
    case ZE_GRAPH_ARGUMENT_PRECISION_INT4:
        return 4;
    case ZE_GRAPH_ARGUMENT_PRECISION_UINT4:
        return 4;
    case ZE_GRAPH_ARGUMENT_PRECISION_INT8:
        return 8;
    case ZE_GRAPH_ARGUMENT_PRECISION_UINT8:
        return 8;
    case ZE_GRAPH_ARGUMENT_PRECISION_INT16:
        return 16;
    case ZE_GRAPH_ARGUMENT_PRECISION_UINT16:
        return 16;
    case ZE_GRAPH_ARGUMENT_PRECISION_INT32:
        return 32;
    case ZE_GRAPH_ARGUMENT_PRECISION_UINT32:
        return 32;
    case ZE_GRAPH_ARGUMENT_PRECISION_INT64:
        return 64;
    case ZE_GRAPH_ARGUMENT_PRECISION_UINT64:
        return 64;
    case ZE_GRAPH_ARGUMENT_PRECISION_BF16:
        return 16;
    case ZE_GRAPH_ARGUMENT_PRECISION_FP16:
        return 16;
    case ZE_GRAPH_ARGUMENT_PRECISION_FP32:
        return 32;
    case ZE_GRAPH_ARGUMENT_PRECISION_FP64:
        return 64;
    case ZE_GRAPH_ARGUMENT_PRECISION_BIN:
        return 1;
    default:
        IE_THROW() << "precisionToSize switch->default reached";
    }
}

static inline ze_graph_argument_precision_t getZePrecision(const InferenceEngine::Precision precision) {
    switch (precision) {
    case InferenceEngine::Precision::I4:
        return ZE_GRAPH_ARGUMENT_PRECISION_INT4;
    case InferenceEngine::Precision::U4:
        return ZE_GRAPH_ARGUMENT_PRECISION_UINT4;
    case InferenceEngine::Precision::I8:
        return ZE_GRAPH_ARGUMENT_PRECISION_INT8;
    case InferenceEngine::Precision::U8:
        return ZE_GRAPH_ARGUMENT_PRECISION_UINT8;
    case InferenceEngine::Precision::I16:
        return ZE_GRAPH_ARGUMENT_PRECISION_INT16;
    case InferenceEngine::Precision::U16:
        return ZE_GRAPH_ARGUMENT_PRECISION_UINT16;
    case InferenceEngine::Precision::I32:
        return ZE_GRAPH_ARGUMENT_PRECISION_INT32;
    case InferenceEngine::Precision::U32:
        return ZE_GRAPH_ARGUMENT_PRECISION_UINT32;
    case InferenceEngine::Precision::I64:
        return ZE_GRAPH_ARGUMENT_PRECISION_INT64;
    case InferenceEngine::Precision::U64:
        return ZE_GRAPH_ARGUMENT_PRECISION_UINT64;
    case InferenceEngine::Precision::BF16:
        return ZE_GRAPH_ARGUMENT_PRECISION_BF16;
    case InferenceEngine::Precision::FP16:
        return ZE_GRAPH_ARGUMENT_PRECISION_FP16;
    case InferenceEngine::Precision::FP32:
        return ZE_GRAPH_ARGUMENT_PRECISION_FP32;
    case InferenceEngine::Precision::FP64:
        return ZE_GRAPH_ARGUMENT_PRECISION_FP64;
    case InferenceEngine::Precision::BIN:
        return ZE_GRAPH_ARGUMENT_PRECISION_BIN;
    default:
        return ZE_GRAPH_ARGUMENT_PRECISION_UNKNOWN;
    }
}

static inline std::size_t layoutCount(const ze_graph_argument_layout_t val) {
    switch (val) {
    case ZE_GRAPH_ARGUMENT_LAYOUT_NCHW:
        return 4;
    case ZE_GRAPH_ARGUMENT_LAYOUT_NHWC:
        return 4;
    case ZE_GRAPH_ARGUMENT_LAYOUT_NCDHW:
        return 5;
    case ZE_GRAPH_ARGUMENT_LAYOUT_NDHWC:
        return 5;
    case ZE_GRAPH_ARGUMENT_LAYOUT_OIHW:
        return 4;
    case ZE_GRAPH_ARGUMENT_LAYOUT_C:
        return 1;
    case ZE_GRAPH_ARGUMENT_LAYOUT_CHW:
        return 3;
    case ZE_GRAPH_ARGUMENT_LAYOUT_HW:
        return 2;
    case ZE_GRAPH_ARGUMENT_LAYOUT_NC:
        return 2;
    case ZE_GRAPH_ARGUMENT_LAYOUT_CN:
        return 2;
    default:
        IE_THROW() << "layoutCount switch->default reached";
    }
}

static inline std::size_t getSizeIOBytes(const ze_graph_argument_properties_t& argument) {
    std::size_t num_elements = 1;
    for (std::size_t i = 0; i < layoutCount(argument.deviceLayout); ++i) {
        num_elements *= argument.dims[i];
    }
    const std::size_t size_in_bits = num_elements * precisionToSize(argument.devicePrecision);
    const std::size_t size_in_bytes = (size_in_bits + (CHAR_BIT - 1)) / CHAR_BIT;
    return size_in_bytes;
}

static inline uint32_t findGroupOrdinal(
        const std::vector<ze_command_queue_group_properties_t>& command_group_properties,
        const ze_device_properties_t& properties) {
    auto log = Logger::global().nest("findGroupOrdinal", 0);

    if (properties.flags & ZE_DEVICE_PROPERTY_FLAG_INTEGRATED) {
        for (uint32_t index = 0; index < command_group_properties.size(); ++index) {
            const auto& flags = command_group_properties[index].flags;
            if ((flags & ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE) != 0 &&
                (flags & ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COPY) == 0) {
                return index;
            }
        }

        // if we don't find a group where only the proper flag is enabled then search for a group where that flag is
        // enabled
        for (uint32_t index = 0; index < command_group_properties.size(); ++index) {
            const auto& flags = command_group_properties[index].flags;
            if (flags & ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE) {
                return index;
            }
        }

        // if still don't find compute flag, return a warning
        log.warning("Fail to find a command queue group that contains compute flag, it will be set to 0.");
        return 0;
    }

    for (uint32_t index = 0; index < command_group_properties.size(); ++index) {
        const auto& flags = command_group_properties[index].flags;
        if ((flags & ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE) != 0 &&
            (flags & ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COPY) != 0) {
            return index;
        }
    }

    // if still don't find compute and copy flag, return a warning
    log.warning("Fail to find a command queue group that contains compute and copy flags, it will be set to 0.");
    return 0;
}

}  // namespace zeroUtils
}  // namespace vpux
