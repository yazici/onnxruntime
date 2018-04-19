#pragma once

#include "core/common/common.h"
#include "core/framework/op_kernel.h"
#include "core/util/math_cpuonly.h"
#include "ml_common.h"

namespace Lotus {
namespace ML {

// stuffs shared by SVMClassifier and SVMRegressor
template <typename T>
class SVMCommon {
 protected:
  SVMCommon(const OpKernelInfo& info) {
    std::string tmp = "LINEAR";
    info.GetAttr<std::string>("kernel_type", &tmp);
    KERNEL tmpval2 = MakeKernel(tmp);
    kernel_type_ = tmpval2;

    std::vector<float> kernel_params;
    LOTUS_ENFORCE(info.GetAttrs<float>("kernel_params", kernel_params).IsOK());

    if (kernel_params.size() > 0) {
      gamma_ = kernel_params[0];
      coef0_ = kernel_params[1];
      degree_ = kernel_params[2];
    }
  }

  void set_kernel_type(KERNEL new_kernel_type) { kernel_type_ = new_kernel_type; }
  KERNEL get_kernel_type() const { return kernel_type_; }

  float kernel_dot(const T* A, int64_t a, const std::vector<float>& B, int64_t b, int64_t len, KERNEL k) const {
    float sum = 0.f;
    if (k == KERNEL::POLY) {
      for (int64_t i = 0; i < len; i++) {
        sum += B[b + i] * static_cast<float>(A[a + i]);
      }
      sum = gamma_ * sum + coef0_;
      sum = std::pow(sum, degree_);
    } else if (k == KERNEL::SIGMOID) {
      for (int64_t i = 0; i < len; i++) {
        sum += B[b + i] * static_cast<float>(A[a + i]);
      }
      sum = gamma_ * sum + coef0_;
      sum = std::tanh(sum);
    } else if (k == KERNEL::RBF) {
      for (int64_t i = 0; i < len; i++) {
        float val = static_cast<float>(A[a + i]) - B[b + i];
        sum += (val * val);
      }
      sum = std::exp(-gamma_ * sum);
    } else if (k == KERNEL::LINEAR) {
      for (int64_t i = 0; i < len; i++) {
        sum += B[b + i] * static_cast<float>(A[a + i]);
      }
    }
    return sum;
  }

 private:
  KERNEL kernel_type_;
  float gamma_;
  float coef0_;
  float degree_;
};

template <typename T>
class SVMClassifier final : public OpKernel, private SVMCommon<T> {
  using SVMCommon<T>::kernel_dot;
  using SVMCommon<T>::set_kernel_type;
  using SVMCommon<T>::get_kernel_type;

 public:
  SVMClassifier(const OpKernelInfo& info);
  Status Compute(OpKernelContext* context) const override;

 private:
  bool weights_are_all_positive_;
  int64_t feature_count_;
  int64_t class_count_;
  int64_t vector_count_;
  bool using_strings_;
  std::vector<int64_t> vectors_per_class_;
  std::vector<int64_t> starting_vector_;
  std::vector<float> rho_;
  std::vector<float> proba_;
  std::vector<float> probb_;
  std::vector<float> coefficients_;
  std::vector<float> support_vectors_;
  std::vector<int64_t> classlabels_ints_;
  std::vector<std::string> classlabels_strings_;
  POST_EVAL_TRANSFORM post_transform_;
  SVM_TYPE mode_;  //how are we computing SVM? 0=LibSVC, 1=LibLinear
};

}  // namespace ML
}  // namespace Lotus
