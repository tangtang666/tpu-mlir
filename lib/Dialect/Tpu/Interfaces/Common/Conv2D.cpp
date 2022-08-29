//===----------------------------------------------------------------------===//
//
// Copyright (c) 2020-2030 by Sophgo Technologies Inc. All rights reserved.
//
// Licensed under the Apache License v2.0.
// See http://www.apache.org/licenses/LICENSE-2.0 for license information.
// SPDX-License-Identifier: Apache-2.0
//
//===----------------------------------------------------------------------===//

#include "tpu_mlir/Dialect/Tpu/IR/TpuOps.h"
#include "tpu_mlir/Support/Dnnl/Dnnl.h"
#include "tpu_mlir/Support/Float16.h"
#include "tpu_mlir/Support/Helper/Module.h"
#include "tpu_mlir/Support/Helper/Quant.h"
#include "tpu_mlir/Support/MathUtils.h"

using namespace tpu_mlir;
using namespace tpu_mlir::helper;
using namespace mlir;

void tpu::Conv2DOp::parseParam(void *param) {
  conv_attr_t *p = (conv_attr_t *)param;
  memset(p, 0, sizeof(conv_attr_t));
  p->id = p->od = p->kd = p->sd = p->dd = 1;
  auto i_s = input().getType().cast<RankedTensorType>().getShape();
  auto o_s = output().getType().cast<RankedTensorType>().getShape();
  p->do_relu = this->do_relu();
  p->relu_limit = relu_limit().convertToDouble();
  p->has_bias = with_bias();
  p->n = i_s[0];
  p->ic = i_s[1];
  p->ih = i_s[2];
  p->iw = i_s[3];
  p->oc = o_s[1];
  p->oh = o_s[2];
  p->ow = o_s[3];
  auto kernel = Module::getI64Array(kernel_shape());
  p->kh = kernel->at(0);
  p->kw = kernel->at(1);
  auto pads_v = Module::getI64Array(pads());
  p->pht = pads_v->at(0);
  p->pwl = pads_v->at(1);
  p->phb = pads_v->at(2);
  p->pwr = pads_v->at(3);
  if (Quant::isUniformQuantized(input())) {
    p->pad_value = Quant::getUniformQuantizedType(input()).getZeroPoint();
  }
  auto strides_v = Module::getI64Array(strides());
  p->sh = strides_v->at(0);
  p->sw = strides_v->at(1);
  auto dhdw = Module::getI64Array(dilations(), 2, 1);
  p->dh = dhdw->at(0);
  p->dw = dhdw->at(1);
  auto ins = Module::getI64Array(inserts(), 2, 0);
  p->ins_h = ins->at(0);
  p->ins_w = ins->at(1);
  assert(p->ins_h == 0 && p->ins_w == 0);
  p->groups = group();
  p->is_dw = (p->oc == p->ic && p->oc == p->groups && p->groups > 1);
}
LogicalResult tpu::Conv2DOp::init(InferenceParameter &p) {
  auto conv = new Conv();
  conv_attr_t attr = {0};
  parseParam(&attr);

  conv->setup(p.inputs[0], p.inputs[1], p.inputs[2], p.outputs[0], attr);
  p.handle = (void *)conv;
  return success();
}

void tpu::Conv2DOp::deinit(InferenceParameter &p) {
  if (p.handle != nullptr) {
    auto conv = (Conv *)p.handle;
    delete conv;
    p.handle = nullptr;
  }
}

LogicalResult tpu::Conv2DOp::inference(InferenceParameter &p) {
  if (p.handle == nullptr) {
    return failure();
  }
  auto conv = (Conv *)p.handle;
  conv->run();
  // requant
  auto out_type = Module::getStorageType(output());
  auto num_elem = Module::getNumElements(output());
  if (out_type.isa<FloatType>()) {
    if (out_type.isBF16()) {
      f32_to_bf16(p.outputs[0], p.outputs[0], num_elem);
    } else if (out_type.isF16()) {
      f32_to_f16(p.outputs[0], p.outputs[0], num_elem);
    }
  } else if (Quant::isUniformQuantized(output())) {
    int64_t n, c, h, w;
    auto sType = Module::getStorageType(output());
    Module::getNCHW(output(), n, c, h, w);
    auto o_qtype = Quant::getUniformQuantizedType(output());
    auto rshift_v = Module::getI64Array(rshift().getValue());
    auto multiplier_v = Module::getI64Array(multiplier(), rshift_v->size(), 1);
    bool per_axis = rshift_v->size() == c;
    auto mode = quant_mode().hasValue() ? quant_mode().getValue() : 2;
#pragma omp parallel for schedule(static, omp_schedule(c))
    for (int ic = 0; ic < c; ic++) {
      int64_t shift = per_axis ? rshift_v->at(ic) : rshift_v->at(0);
      int64_t multi = per_axis ? multiplier_v->at(ic) : multiplier_v->at(0);
      for (int in = 0; in < n; in++) {
        for (int hw = 0; hw < h * w; hw++) {
          int offset = (in * c + ic) * h * w + hw;
          int64_t v = 0;
          if (mode == 0) {
            v = MultiplyByQuantizedMultiplier((int32_t)p.outputs[0][offset],
                                              (int32_t)multi, (int32_t)shift) +
                o_qtype.getZeroPoint();
          } else {
            v = applyMultiplierAndRShift(p.outputs[0][offset], multi, shift) +
                o_qtype.getZeroPoint();
          }
          p.outputs[0][offset] = sType.isUnsignedInteger(8) ? Quant::to_uint8(v)
                                                            : Quant::to_int8(v);
        }
      }
    }
  }

  return success();
}

LogicalResult tpu::Conv2DOp::BackwardH(int64_t &in_idx, int64_t &in_slice,
                                       int64_t out_idx, int64_t out_slice) {
  conv_attr_t attr = {0};
  parseParam(&attr);
  int kh_with_dh = (attr.kh - 1) * attr.dh + 1;
  in_slice = (out_slice - 1) * attr.sh +
             (kh_with_dh >= attr.sh ? kh_with_dh : attr.sh);
  in_idx = out_idx * attr.sh - attr.pht;
  LocalGenInterface::fixSlice(in_idx, in_slice, attr.ih);
  return success();
}