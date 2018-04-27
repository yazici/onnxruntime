#pragma once

#include "core/common/common.h"
#include "core/framework/op_kernel.h"
#include "core/util/math_cpuonly.h"
#include "ml_common.h"

namespace Lotus {
namespace ML {

template <typename T>
class LinearRegressor final : public OpKernel {
 public:
  LinearRegressor(const OpKernelInfo& info);
  Status Compute(OpKernelContext* context) const override;

 private:
  int64_t targets_;
  std::vector<float> coefficients_;
  std::vector<float> intercepts_;
  POST_EVAL_TRANSFORM post_transform_;
};

}  // namespace ML
}  // namespace Lotus
