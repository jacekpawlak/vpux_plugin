#
# Copyright (C) 2022 Intel Corporation.
# SPDX-License-Identifier: Apache 2.0
#

set(TARGET_NAME "npu-loader")

add_tool_target(
    NAME ${TARGET_NAME}
    ROOT ${CMAKE_CURRENT_SOURCE_DIR}
    ENABLE_WARNINGS_AS_ERRORS
    ADD_CLANG_FORMAT
    LINK_LIBRARIES
         vpux_elf
         npu_utils
         npu_mlir_compiler_static
         npu_translate_utils_static
)
