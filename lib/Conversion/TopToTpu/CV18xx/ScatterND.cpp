//===----------------------------------------------------------------------===//
//
// ScatterNDright (C) 2022 Sophgo Technologies Inc.  All rights reserved.
//
// TPU-MLIR is licensed under the 2-Clause BSD License except for the
// third-party components.
//
//===----------------------------------------------------------------------===//

#include "tpu_mlir/Conversion/TopToTpu/LoweringCV18xx.h"
#include "llvm/Support/Debug.h"

#define DEBUG_TYPE "lowering-ScatterND"
namespace tpu_mlir {
namespace cv18xx {
void ScatterNDLowering::LoweringINT8(PatternRewriter &rewriter, top::ScatterNDOp op,
                                   bool asymmetric) const {
  if (!isa<top::WeightOp>(op.getIndices().getDefiningOp())) {
    llvm_unreachable("Not support activation indices.");
  }
  auto newType = getQuantInt8Type(op.getOutput(), asymmetric);
  rewriter.replaceOpWithNewOp<tpu::ScatterNDOp>(op, newType, op->getOperands(), op->getAttrs());
}

void ScatterNDLowering::LoweringBF16(PatternRewriter &rewriter,
                                   top::ScatterNDOp op) const {
  if (!isa<top::WeightOp>(op.getIndices().getDefiningOp())) {
    llvm_unreachable("Not support activation indices.");
  }
  auto newType = getQuantBF16Type(op.getOutput());
  rewriter.replaceOpWithNewOp<tpu::ScatterNDOp>(op, newType, op->getOperands(), op->getAttrs());
}

} // namespace cv18xx
} // namespace tpu_mlir