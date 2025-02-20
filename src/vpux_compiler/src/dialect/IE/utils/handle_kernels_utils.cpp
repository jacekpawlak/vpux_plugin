//
// Copyright (C) 2022 Intel Corporation.
// SPDX-License-Identifier: Apache 2.0
//

#include "vpux/compiler/dialect/IE/utils/handle_kernels_utils.hpp"
#include "vpux/compiler/conversion.hpp"
#include "vpux/compiler/utils/attributes.hpp"
#include "vpux/compiler/utils/rewriter.hpp"
#include "vpux/compiler/utils/types.hpp"

using namespace vpux;

Optional<IE::KernelsInfo> vpux::IE::calculateKernelsInfo(ShapeRef origKernel, Logger log) {
    const auto KY = origKernel[Dims4D::Kernel::Y];
    const auto KX = origKernel[Dims4D::Kernel::X];

    const auto getPerAxisFactorsInfo = [&](const auto kernelSize, Dim kernelDim) -> llvm::Optional<FactorsInfo> {
        auto factorsResult = vpux::IE::getFactors(kernelSize);
        if (!factorsResult.has_value()) {
            log.trace("Cannot get '{0}' FactorsInfo", kernelDim);
            return None;
        }

        const auto factorsInfo = factorsResult.value();
        const auto factors = factorsInfo.factors;
        if (factors.second <= VPU::NCEInvariant::MAX_KERNEL_SIZE) {
            log.trace("KernelSize['{0}'] = '{1}', first factor: '{2}' , second factor: '{3}', "
                      "padValue: '{4}'",
                      kernelDim, kernelSize, factors.first, factors.second, factorsInfo.padValue);
        } else {
            log.trace("KernelSize['{0}'] = '{1}', first factor: '{2}' , second factor: '{3}', "
                      "padValue: '{4}' (Required further split!)",
                      kernelDim, kernelSize, factors.first, factors.second, factorsInfo.padValue);
        }
        return factorsInfo;
    };

    // Get Dims4D::Kernel::X/Y FactorsInfo separately
    // TODO(E#90029): Consider both Axis split result simultaneously to get better solution
    // For example: Pooling Op with kernel [20, 10], pads_begin = [0, 0], pads_end = [0, 0], strides = [20, 10]
    // Input shape: 1x16x40x20, Output shape: 1x16x2x2
    //  - Split 1: Kernel: [5, 10], Stride: [5, 10], PadBegin: [0, 0], PadEnd: [0, 0]
    //  - Split 2: Kernel: [4, 1], Stride: [4, 1], PadBegin: [0, 0], PadEnd: [0, 0]
    // And the first Op with large stride, will futher split at "handle large stride" pass
    // The better solution is to split it with:
    //  - Split 1: Kernel: [5, 5], Stride: [5, 5], PadBegin: [0, 0], PadEnd: [0, 0]
    //  - Split 2: Kernel: [4, 2], Stride: [4, 2], PadBegin: [0, 0], PadEnd: [0, 0]
    const auto factorsInfoAtY = getPerAxisFactorsInfo(KY, Dims4D::Kernel::Y);
    const auto factorsInfoAtX = getPerAxisFactorsInfo(KX, Dims4D::Kernel::X);

    if (!factorsInfoAtX.has_value() || !factorsInfoAtY.has_value()) {
        return None;
    }

    const auto factorsInfoAtXVal = factorsInfoAtX.value();
    const auto factorsInfoAtYVal = factorsInfoAtY.value();

    // firstKernel, secondKernel, padBegin, padEnd
    return IE::KernelsInfo(Shape{factorsInfoAtYVal.factors.first, factorsInfoAtXVal.factors.first},
                           Shape{factorsInfoAtYVal.factors.second, factorsInfoAtXVal.factors.second}, Shape{0, 0},
                           Shape{factorsInfoAtYVal.padValue, factorsInfoAtXVal.padValue});
}

// Those last 2 checks have the main scope of finding the best suited factors:
// if one of the last 2 checks fails it means that the gap between product of
// those 2 factors and original kernel size is too big, which generates larger overlapping area
bool vpux::IE::checkFactors(const Factors& factors, int64_t kernelSize) {
    const auto hasZeroFactors = factors.first == 0 || factors.second == 0;
    const auto factorLessThanKernelSize = factors.first * factors.second < kernelSize;
    const auto isIllegalFirstFactor = factors.first > VPU::NCEInvariant::MAX_KERNEL_SIZE;
    const auto hasBadFactors = factors.first * factors.second > (kernelSize + factors.first / 2);
    return !(hasZeroFactors || factorLessThanKernelSize || isIllegalFirstFactor || hasBadFactors);
}

Optional<Factors> vpux::IE::getFactorsWithLimitation(int64_t val, int64_t limit) {
    if (val <= limit) {
        return Factors(val, 1);
    }

    for (int64_t factor = limit; factor > 1; factor--) {
        if (val % factor == 0) {
            return Factors(factor, val / factor);
        }
    }
    return None;
}

Optional<Factors> vpux::IE::getFactorsAroundWithLimitation(int64_t val, int64_t aroundVal, int64_t limit) {
    return getFactorsWithLimitation(val + aroundVal, limit);
}

Optional<IE::FactorsInfo> vpux::IE::getFactors(const int64_t kernelSize) {
    // Set limit value with the smaller one of MAX_KERNEL_SIZE and MAX_STRIDE
    const auto limit = std::min(VPU::NCEInvariant::MAX_KERNEL_SIZE, VPU::NCEInvariant::MAX_STRIDE);

    if (kernelSize <= VPU::NCEInvariant::MAX_KERNEL_SIZE) {
        return FactorsInfo(kernelSize, 1, 0);
    }

    const auto factors = getFactorsWithLimitation(kernelSize, limit);
    if (factors.has_value() && checkFactors(factors.value(), kernelSize)) {
        return FactorsInfo(factors.value(), 0);
    }

    // If can not get Factors with original size
    // Pad can be consider to fit it
    int64_t padValue = 1;
    while (padValue < kernelSize) {
        const auto factors = getFactorsAroundWithLimitation(kernelSize, padValue, limit);
        if (factors.has_value() && checkFactors(factors.value(), kernelSize)) {
            return FactorsInfo(factors.value(), padValue);
        }
        padValue++;
    }

    return None;
}

bool vpux::IE::hasSupportedKernels(ShapeRef kernelSize) {
    const auto KY = kernelSize[Dims4D::Kernel::Y];
    const auto KX = kernelSize[Dims4D::Kernel::X];

    return KY <= VPU::NCEInvariant::MAX_KERNEL_SIZE && KX <= VPU::NCEInvariant::MAX_KERNEL_SIZE;
}

bool vpux::IE::isPoolingKernelSizeValid(int64_t kernelSize) {
    return getFactors(kernelSize).has_value();
}
