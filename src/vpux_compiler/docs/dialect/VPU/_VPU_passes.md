<!-- Autogenerated by mlir-tblgen; don't manually edit -->
### `-add-sparsity-map-to-sparse-activations`: Update type of result for operations which produce SparseTensor type.
Pass updates output type of operations which produce sparsified output. It adds sparsity_map to output tensor type.
Then it propagates type to all users until sparse data consumer is reached.
### `-adjust-memory-space`: Adjusts the tensor location for VPU-driven operations
The pass adjusts the location of tensors that are used by hardware-driven operations

Currently, it surrounds VPU-driven nodes with Copy operations to specify that all the data
that they consume/produce must reside in CMX
### `-adjust-tiling-for-permute-quantize`: Adjust Slice and Concat position after tiling permute quantize
This pass rewrites the operation sequence for `VPU.NCE.PermuteQuantize`.
After `ApplyTilingPass` `VPU.Slice` and `VPU.Concat` operations will be inserted next to `VPU.NCE.PermuteQuantize`,
but we want to slice and concatenate all the sequence (Reshape -> LayoutCast -> PermuteQuantize -> LayoutCast -> [optional Reshape]).
### `-adjust-vf-tiling-strategy`: Adjust tiling strategy in order to make sure that all subgraphs fit in CMX
Take into account conditions in order to avoid additional spills
In case some of them breaks, increase number of tiles
Following buffers should fit in CMX for each VF tile
1. Input tensors of VF tile
2. Output of VF tile
3. Largest operation in the block
### `-apply-tiling`: Apply tiling on layers with assigned tiling strategy
The pass applies tiling strategy on layers with previously assigned strategy attribute.
### `-cmx-concat`: Move Concat operations from DDR to NNCMX
This pass will try to check if a Concat operation can fit in NNCMX
with few restrictions and if so move the concat from DDR to NNCMX.
### `-correct-NCE-workloads`: Correct NCE workloads if they do not fit requirements
The pass adjusts workload size for NCEDepthConvolution, NCEMaxPool, NCEAveragePool and NCEPermuteQuantize,
as well as for NCE operations that produce sparse activations.

NCEDepthConvolutionOp, NCEMaxPoolOp and NCEAveragePoolOp require the number of channels to be 16, 32 or 64.
If the number of channels does not match, workload is split.

NCEPermuteQuantizeOp rotates output dimensions and padding may be used to indicate the expansion over height.
It is necessary to subtract pads from respective workload dimensions and then set zero padding.

NCE operations with sparse outputs must have all variants with the same number of channels and the number
of channels has to be a power of two. Additionally, if the NCE op shares a consumer with another NCE op
(directly or indirectly), the number of channels of their variants must be aligned.
### `-detect-in-place-eltwise`: Convert Eltwise operation to read and write to the same buffer in memory
This pass will check if Eltwise operation has input and output buffers of the same size
in memory and mark such Eltwise eligible for inplace execution.
It will write the result into one of the inputs in memory.
### `-detection-output-decomposition`: Replace DetectionOutput operation with a subgraph of smaller operations
Replace DetectionOutput operation
┌─────────┐   ┌────────────────┐  ┌──────────┐
│BoxLogits│   │ClassPredictions│  │PriorBoxes│
└────┬────┘   └───────┬────────┘  └─────┬────┘
     │                │                 │
     │         ┌──────┴────────┐        │
     └─────────┤DetectionOutput├────────┘
               └───────────────┘

with a subgraph (Reshapes and MemPermutes are ommited)
┌─────────┐  ┌──────────┐        ┌────────────────┐
│BoxLogits│  │PriorBoxes│        │ClassPredictions│
└───────┬─┘  └─┬────────┘        └───────┬────────┘
        │      │                         │
┌───────┴──────┴───────────┐  ┌──────────┴────────────┐
│DetectionOutputDecodeBoxes│  │DetectionOutputSortTopK│
└───────────────────┬──────┘  └───┬──┬─────┬─┬────────┘
                    │             │  │     │ │
              ┌─────┴─────────────┴──┴───┐ │ │
              │DetectionOutputSelectBoxes│ │ │
              └─────────────┬────────────┘ │ │
                            │              │ │
                          ┌─┴──────────────┴─┴────┐
                          │DetectionOutputNmsCaffe│
                          └────┬─┬─┬──────────────┘
                               │ │ │
                  ┌────────────┴─┴─┴────────────┐
                  │DetectionOutputCollectResults│
                  └─────────────────────────────┘
### `-ensure-nce-ops-size-requirements`: Ensure hw operations meet size requirements
This pass ensures that hardware operations meet hardware size requirements:
each operation need to have less than 8192 values per dimension. This is done
by tiling such operations into smaller ones.
### `-fuse-clamp`: Fuses VPU.Clamp parameters into previous NCE operation
This pass follows `SetupPPEPass` and fuses VPU.Clamp with already existing PPE task.
1. Search for VPU.NCE -> VPU.Clamp pattern
2. Fetch min and max parameters from VPU.Clamp
3. Set clamp_low and clamp_high according to min, max and existing activation
4. Remove VPU.Clamp from the graph
### `-fuse-nce-interpolate-consumers`: Fuses NCE.Interpolate into consumer NCE.Convolution
Fuses NCE.Interpolate with consumer NCE.Convolution.

NCE.Interpolate with mode "nearest" is lowered to a dummy NCE.Convolution with
SE table that upsamples tensor.

We can simply pass the SE table to this consumer and avoid the dummy convolution.
### `-fuse-sparsity-ops`: Fuse subsequent [De]SparsifyOps with SparseOpInterface ops

#### Options
```
-fuse-sparsify : Flag to choose inputs or output will be handled
```
### `-init-compiler`: Initializes compiler for VPU platforms
This pass attaches VPU related compilation parameters to Module attributes and
initializes **IERT Dialect** run-time resources information.

#### Options
```
-vpu-arch            : VPU architecture to compile for
-compilation-mode    : [Optional] Set compilation mode as `ReferenceSW`, `ReferenceHW` or `DefaultHW`
-num-of-dpu-groups   : [Optional] Number of available DPU groups
-num-of-dma-ports    : [Optional] Number of available DMA ports
-allow-custom-values : [Optional] Allows keep predefined values in IR
```
### `-lower-ops-to-se-nce`: Converts compatible operations to SE NCE operations
Finds operations that can be executed on hardware using the Storage Element pointers
feature and lowers them to VPU.NCE.

The list of supported operations:
- Interpolate - mode: NEAREST
                axes: H, W
                coordinate_transformation_mode: all (except ALIGN_CORNERS)
                nearest_mode: all
                scale: integer only
                padding: none

              - mode: LINEAR, LINEAR_ONNX
                axes: H, W
                coordinate_transformation_mode: ASYMMETRIC
                scale: [1-11] (integer only)
                padding: none
### `-lower-sparsity-ops`: Convert Sparsify/Desparsify ops to Eltwise or GroupSparseBufferOp
Converts Sparsify operations to Convolutions and Desparsify operations to Eltwise ops.

In case the `fakeSparsity` flag is set to true, Sparsify operations are instead converted to a
GroupSparseTensor operation whose sparsity map contains only values of 1. This lets the data be
interpreted as a sparse one without actually removing the sparse values.

#### Options
```
-fake-sparsify : Flag to choose method of VPU.Sparsify lowering
```
### `-manual-strategy-utils`: Utils for reading or writing a json strategy
Utility allowing to store and write as JSON the current selected strategy from the two strategy passes
createMultiClusterStrategyAssignmentPass() and createPrefetchTilingPass(). And also to manually
overwrite the strategy.

#### Options
```
-write-strategy-to-json       : Flag to enable writing strategy to file
-write-strategy-file-location : Location/path to write strategy file
-read-strategy-from-json      : Flag to enable reading strategy from file
-read-strategy-file-location  : Location/path to read strategy file
```
### `-merge-vertical-fusion-subgraphs`: Build subgraph from VF single regions
Merge VF blocks and add operations to them recalculating tiling information in following cases:
1. Number of operations which might increase computational cost does not exceed limit
2. All operations have same multicluster strategy or don't have them at all
3. Region which is supposed to be added doesn't have any other users except current region or
all its users point to current region too.
4. All operations in new region after merging fit in CMX when they are tiled for VF. In case they don't, number of tiles
increases.
5. Required CMX memory by constant weights shouldn't exceed the threshold to avoid spilling.
### `-multi-cluster-strategy-assignment`: This pass compute the hardware efficiency of layer that is executed as SOH or SOK and assigns the most optimal strategy
### `-optimize-concat`: Try to eliminate Concat for Concat-Slice pattern
After `ApplyTilingPass` lots of `VPU.Concat`-`VPU.Slice` are introduced, `VPU.Concat` can be eliminated only if all its users are `VPU.Slice` and
input tensor of each `VPU.Slice` is actually sub-tensor of one of input tensors of Concat.
### `-optimize-sparsify-desparsify-pairs`: Optimize common patterns of subsequent sparsify-desparsify ops to remove redundant conversions

#### Options
```
-sparsity-profile : Flag to choose sparsity profile
```
### `-optimize-sparsity-ops`: Optimize additional sparsity patterns
Some optimizations such duplicated Sparsify ops for Eltwise, first Sparsify
or last Desparsify cant be done during WrapOpsInSparsifyDesparsifyPairs pass
until output sparsity wouldnt be fused

#### Options
```
-sparsity-profile : Flag to choose sparsity profile
```
### `-recompute-sparsity-ptrs`: Recomputes sparsity pointers
Recomputes the sparsity pointers inside the weights table for sparse weights.
### `-resolve-eltwise-with-z-tiled-workloads`: Resolves Eltwise operations which have workloads tiled over Z
Hardware Eltwise does not support variants tiled over the Z dimension. If such cases are encountered,
these operations are split into separate Eltwise operations, each containing the workloads that cover
a different subset of channels.

For example, if the original Eltwise contains the following workloads:
    1. offset = [0, 0,  0, 0], sizes = [1, 64, 8, 16], cluster_id = 0
    2. offset = [0, 64, 0, 0], sizes = [1, 64, 8, 16], cluster_id = 0
    3. offset = [0, 0,  8, 0], sizes = [1, 64, 8, 16], cluster_id = 1
    4. offset = [0, 64, 8, 0], sizes = [1, 64, 8, 16], cluster_id = 1
Two Eltwise operations will be created, the first one containing workloads 1 and 3, the other one
workloads 2 and 4, with their channel offsets reset to zero. The correct subset of channels is
sliced individually for each new Eltwise operation.

In case the inputs are distributed types in CMX, manual copy operations that spill them to DDR are
introduced, in order to avoid Slice operations that work with these types. These Slice operations
would get lowered to copies where both the input and output are distributed types; such scenarios
are not fully supported (E#78676).

The outputs of the smaller Eltwise operations get copied to DDR in order to avoid accuracy degradation
that takes place when the outputs are concatenated in CMX.
### `-resolve-pwl-post-ops`: Resolve requirements for fused PWL post-ops
Ensures the correct quantization ranges are used for fused PWL activation functions.
### `-roll-back-tiling-strategy`: Roll back the H-prioritized tiling strategy if unused
If the tiling strategy is changed to H dimension to enable vertical fusion but at last not vertical-fused
roll back to the original tiling strategy

#### Options
```
-tiling-mode : [Optional] Set tiling mode as `ISOLATED` or `PREFETCH`
```
### `-setup-ppe`: Sets activation function for VPU37XX PPE based on clamp range
Ensures the correct activation function and clamping is used for PPE.
Namely:
* When ReLU shift value is non-zero, set leaky ReLU.
* Otherwise, set NOOP.
* Deduce clamping via output element type.
### `-sparsify-weights`: Sparsify weights for NCE ops
Convert const parameters for NCE ops to sparse types depending on sparsify strategy.
### `-split-NCE-ops-onto-workloads`: Split VPU NCE operation onto workloads
### `-split-gru-sequence`: Split GRUSequence into GRUSequenceFirstPart and GRUSequenceLastPart
The pass can split GRUSequence into two parts to fit into CMX when tiling strategy can't be generated.
### `-strategy-manager`: Assignment and optimization multi-cluster strategies to operations
Pass consists of two parts:
1. Assignment of multicluster strategies and tiling strategies to each operation based on vpunn cost of each strategy.
2. Optimization/adjustment of strategies based on one of common optimization algorithm.

#### Options
```
-tiling-mode : [Optional] Set tiling mode as `ISOLATED` or `PREFETCH`. `PREFETCH` is set by default
```
### `-tile-over-h-for-vf`: Assign tiling strategy over H for the following vertical fusion
This pass tiles candidate vertical fusion operations over H for the following vertical fusion passes.

#### Options
```
-tiling-mode : [Optional] Set tiling mode as `ISOLATED` or `PREFETCH`
```
### `-tiling-strategy-assignment`: Assign tiling strategy for layers applicable
The pass assigns tiling strategy for layers whose memory requirements exceed the capacity available.
The pass only assigns strategy and do not perform any tiling actions, and if tiling strategy is set by
ManualStrategyUtilsPass, it will not be processed by this pass.

Isolated tiling: split each single layer in isolation, with no smarter heuristics such as
                 "allow running in parallel" or "allow continious computation in tiles" or any else.
Prefetch tiling: tries to run tiles in parallel, and 'prefetch' means that the next tile could be loaded
                 in advance when the current tile is computing.

The pass does not use any cost model to optimize the entire layer's processing time.

#### Options
```
-tiling-mode : [Optional] Set tiling mode as `ISOLATED` or `PREFETCH`
```
### `-unroll-unused-vertical-fusion`: Unroll single VF blocks
Unroll VF block in case it hasn't been assembled with other blocks in subgraph
### `-vertical-fusion-tiling`: Apply tiling on VF subgraph
Apply VF tiling on subgraph wrapped in VF region.
### `-wrap-in-vertical-fusion`: This pass wraps vpu operations that might be tiled in order to implement VF
Wrap operations to VerticalFusion block which match criterias
1. Operation has activation tiling (activation doesn't fit in CMX)
2. NCE operations don't have strides larger than 1
3. Even if NCE operation doesn't have activation tiling, but its kernel is 1x1,
it also might be wrapped because there is no additional computation cost of it
### `-wrap-ops-in-sparsify-pairs`: Wrap operations in pairs of Sparsify-Desparsify
Wraps operations in pairs of Sparsify-Desparify ops. The sparsity profile
will determine which operations will be wrapped:
- profile S0: add SparsifyOp for each input and Sparsify-Desparsify chain for output
- profile S1: add Sparsify-Desparsify chain both for inputs and output

#### Options
```
-enable-activation-sparsity-mode : Activation sparsity enablement mode (auto, true or false)
-sparsity-profile                : Flag to choose sparsity profile
```
### `-wrap-vpu-ops-in-ncecluster-tiling`: This pass wraps vpu operations that should be executed across multiple clusters in NCEClusterTiling operations
This pass builds an IR in order to represent multi-cluster compilation. It performs a number of functions.
1) It creates variations of distributed tensors depending on the multi-cluster strategy of the layer.
2) It creates DMA operations DDR->CMX and wraps the DMAs in NCEClusterTiling.
3) It wraps hardware executable operations in NCEClusterTiling.

#### Options
```
-enable-explicit-distributed-attr : Flag to enable generating explicit DistributedTensorAttr for Distributed data type.
```
