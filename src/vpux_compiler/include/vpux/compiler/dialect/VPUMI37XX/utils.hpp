//
// Copyright (C) 2022 Intel Corporation.
// SPDX-License-Identifier: Apache 2.0
//

#include "vpux/compiler/dialect/VPUMI37XX/ops.hpp"
#include "vpux/compiler/dialect/VPUMI37XX/types.hpp"
#include "vpux/compiler/utils/logging.hpp"
#include "vpux/utils/core/checked_cast.hpp"
#include "vpux/utils/core/error.hpp"
#include "vpux/utils/core/format.hpp"
#include "vpux/utils/core/numeric.hpp"

#include <mlir/Dialect/MemRef/IR/MemRef.h>
#include <mlir/IR/Value.h>

namespace vpux {
namespace VPUMI37XX {

std::pair<uint8_t, uint32_t> getMaxVID(mlir::Operation::operand_range range);
uint64_t computeMask(mlir::Operation::operand_range barriers);

}  // namespace VPUMI37XX
}  // namespace vpux
