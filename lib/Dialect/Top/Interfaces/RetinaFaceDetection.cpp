//===----------------------------------------------------------------------===//
//
// Copyright (C) 2022 Sophgo Technologies Inc.  All rights reserved.
//
// TPU-MLIR is licensed under the 2-Clause BSD License except for the
// third-party components.
//
//===----------------------------------------------------------------------===//

#include "tpu_mlir/Support/GenericCpuFunc.h"



int64_t top::RetinaFaceDetectionOp::getFLOPs() {
  return module::getNumElements(getOutput());
}

LogicalResult top::RetinaFaceDetectionOp::init(InferenceParameter &p) {
  return success();
}
void top::RetinaFaceDetectionOp::deinit(InferenceParameter &p) {}

LogicalResult top::RetinaFaceDetectionOp::inference(InferenceParameter &p) {
  RetinaFaceDetectionParam param;
  param.confidence_threshold = getConfidenceThreshold().convertToDouble();
  param.keep_topk = getKeepTopk();
  param.nms_threshold = getNmsThreshold().convertToDouble();
  RetinaFaceDetectionFunc func;
  std::vector<tensor_list_t> inputs;
  for (size_t i = 0; i < getInputs().size(); i++) {
    tensor_list_t tensor;
    tensor.ptr = p.inputs[i];
    tensor.size = module::getNumElements(getInputs()[i]);
    tensor.shape = module::getShape(getInputs()[i]);
    inputs.emplace_back(std::move(tensor));
  }
  tensor_list_t output;
  output.ptr = p.outputs[0];
  output.size = module::getNumElements(getOutput());
  output.shape = module::getShape(getOutput());
  func.setup(inputs, output, param);
  func.invoke();
  return success();
}

void top::RetinaFaceDetectionOp::shape_inference() {}
