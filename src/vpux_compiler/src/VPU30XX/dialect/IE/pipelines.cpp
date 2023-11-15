//
// Copyright (C) 2023 Intel Corporation.
// SPDX-License-Identifier: Apache 2.0
//

#include "vpux/compiler/pipelines.hpp"
#include "vpux/compiler/VPU30XX/dialect/IE/passes.hpp"
#include "vpux/compiler/utils/rewriter.hpp"

#include <mlir/Pass/PassManager.h>
#include <mlir/Transforms/Passes.h>

using namespace vpux;

void vpux::IE::arch30xx::buildOptimizeActivationsPipeline(mlir::OpPassManager& pm, Logger log) {
    const auto grc = getDefaultGreedyRewriteConfig();

    pm.addPass(IE::arch30xx::createInsertIdentityPoolBeforeOpPass(log));
    pm.addPass(IE::createFusePostOpsPass(log));
    pm.addPass(mlir::createCanonicalizerPass(grc));
}

void vpux::IE::arch30xx::buildMemPermuteProcessingPipeline(mlir::OpPassManager& pm, Logger log) {
    const auto grc = getDefaultGreedyRewriteConfig();

    pm.addPass(IE::createConvertToMemPermutePass(log));
    pm.addPass(mlir::createCanonicalizerPass(grc));
    pm.addPass(IE::createMovePermutePostEltwisePass(log));
    pm.addPass(mlir::createCanonicalizerPass(grc));
    pm.addPass(IE::createLegalizeNDMemPermutePass(log));
    pm.addPass(IE::createPropagateMemPermuteBeforeOpPass(log));
    pm.addPass(mlir::createCanonicalizerPass(grc));
    pm.addPass(IE::createPropagateMemPermuteThroughAddPass(log));
    pm.addPass(mlir::createCanonicalizerPass(grc));
    pm.addPass(IE::createUniquifyOpsPass(log));
}

//
// registerIEPipelines
//

void vpux::IE::arch30xx::registerIEPipelines() {
    mlir::PassPipelineRegistration<mlir::EmptyPipelineOptions>(
            "optimize-activations", "[OPTIMIZATION] Optimize activations for VPU target", [](mlir::OpPassManager& pm) {
                IE::arch30xx::buildOptimizeActivationsPipeline(pm);
            });

    mlir::PassPipelineRegistration<mlir::EmptyPipelineOptions>(
            "mempermute-processing",
            "[OPTIMIZATION] MemPermute processing is responsible for handling data transfromations ops (Transpose, "
            "Reshape etc), transform it to MemPermute and optimize final subgraph to avoid unnesesary data "
            "permutations",
            [](mlir::OpPassManager& pm) {
                IE::arch30xx::buildMemPermuteProcessingPipeline(pm);
            });
}
