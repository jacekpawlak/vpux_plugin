//
// Copyright (C) 2022 Intel Corporation.
// SPDX-License-Identifier: Apache 2.0
//

#pragma once

#include "vpux/compiler/dialect/VPU/ops.hpp"
#include "vpux/compiler/utils/rewriter.hpp"
#include "vpux/compiler/utils/types.hpp"

#include <mlir/IR/BlockAndValueMapping.h>

namespace vpux {
namespace VPU {

mlir::FailureOr<OutputTiling> getLayerTilingStrategy(VPU::TilingBuilderOpInterface origOp, bool enablePrefetchTiling,
                                                     Logger log, TilingMode& mode);
mlir::FailureOr<OutputTiling> getLayerTilingStrategy(VPU::TilingBuilderOpInterface origOp, bool enablePrefetchTiling,
                                                     Logger log);

mlir::LogicalResult checkAndAlignActInputTiling(vpux::VPU::NCEOpInterface nceOp, InputTiling& inputTiling,
                                                vpux::Logger log);
mlir::Value reifyTile(VPU::TilingBuilderOpInterface origOp, const TileInfo& outputTile, mlir::OpBuilder& builder,
                      Logger log);
mlir::LogicalResult applyTileStrategy(VPU::TilingBuilderOpInterface origOp, const OutputTiling& tiles,
                                      mlir::PatternRewriter& rewriter, Logger log);
mlir::Operation* getParentTargetOp(mlir::Operation* op);
bool prefetchTilingConditionSatisfied(mlir::Operation* op, Logger log);
bool largeConstPipelineConditionSatisfied(mlir::Operation* op, Logger log);
bool hasMultiBranches(mlir::Operation* op);

bool archSupportsSwLayerTiling(VPU::ArchKind arch);
bool opNeedsTiling(mlir::Operation* op, bool enablePrefetchTiling, Logger log);

}  // namespace VPU
}  // namespace vpux
