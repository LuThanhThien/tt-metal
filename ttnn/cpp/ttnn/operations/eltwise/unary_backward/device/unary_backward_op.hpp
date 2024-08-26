// SPDX-FileCopyrightText: © 2023 Tenstorrent Inc.
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <functional>
#include <optional>
#include "ttnn/tensor/tensor.hpp"
#include "third_party/magic_enum/magic_enum.hpp"
#include "ttnn/operations/eltwise/binary_backward/device/binary_backward_op.hpp"

namespace ttnn::operations::unary_backward {

enum class UnaryBackwardOpType {
    CLAMP_BW,
    HARDTANH_BW,
    THRESHOLD_BW,
    SOFTPLUS_BW,
    DIV_BW,
    RDIV_BW,
    POW_BW,
    TANH_BW,
    EXP_BW,
    SQRT_BW,
    ASSIGN_BW,
    MULTIGAMMALN_BW,
    ADD_BW,
    EQ_BW,
    GT_BW,
    LT_BW,
    LGAMMA_BW,
    FILL_BW,
    HARDSIGMOID_BW,
    COS_BW,
    ACOSH_BW,
    ACOS_BW,
    ATAN_BW,
    RAD2DEG_BW,
    SUB_BW,
    FRAC_BW,
    TRUNC_BW,
    LOG_SIGMOID_BW,
    FILL_ZERO_BW,
    I0_BW,
    TAN_BW,
    SIGMOID_BW,
    RSQRT_BW,
    NEG_BW,
    RELU_BW,
    LOGIT_BW,
    HARDSHRINK_BW,
    SOFTSHRINK_BW,
    LEAKY_RELU_BW,
    ELU_BW,
    CELU_BW,
    RPOW_BW,
    FLOOR_BW,
    ROUND_BW,
    LOG_BW,
    RELU6_BW,
    ABS_BW,
    SILU_BW,
    SELU_BW,
    SQUARE_BW,
    HARDSWISH_BW,
    TANHSHRINK_BW,
    ATANH_BW,
    ASIN_BW,
    ASINH_BW,
    SIN_BW,
    SINH_BW,
    LOG10_BW,
    LOG1P_BW,
    ERFC_BW,
    CEIL_BW,
    SOFTSIGN_BW,
    COSH_BW,
    LOGITEPS_BW,
    LOG2_BW,
    SIGN_BW,
    FMOD_BW,
    REMAINDER_BW,
    DIV_NO_NAN_BW,
    EXP2_BW,
    EXPM1_BW,
    RECIPROCAL_BW,
    DIGAMMA_BW,
    ERFINV_BW,
    ERF_BW,
    DEG2RAD_BW,
    POLYGAMMA_BW,
    GELU_BW,
    REPEAT_BW,
    PROD_BW,
};

std::vector<Tensor> _threshold_bw( const Tensor& grad, const Tensor& input, float threshold, float value, const std::optional<MemoryConfig>& output_mem_config);

std::vector<Tensor> _acos_bw( const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config);
std::vector<Tensor> _atan_bw( const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config);
std::vector<Tensor> _rad2deg_bw( const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config);
std::vector<Tensor> _frac_bw( const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config);
std::vector<Tensor> _trunc_bw( const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config);
std::vector<Tensor> _log_sigmoid_bw( const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config);
std::vector<Tensor> _fill_zero_bw( const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config);
std::vector<Tensor> _i0_bw( const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config);
std::vector<Tensor> _tan_bw( const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config);
std::vector<Tensor> _sigmoid_bw( const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config);
std::vector<Tensor> _rsqrt_bw( const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config);
std::vector<Tensor> _neg_bw( const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config);
std::vector<Tensor> _reciprocal_bw( const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config);
std::vector<Tensor> _ceil_bw(const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config);
std::vector<Tensor> _softsign_bw(const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config);
std::vector<Tensor> _cosh_bw(const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config);
std::vector<Tensor> _log2_bw(const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config);
std::vector<Tensor> _sign_bw(const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config);
std::vector<Tensor> _exp2_bw(const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config);
std::vector<Tensor> _expm1_bw(const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config);
std::vector<Tensor> _digamma_bw(const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config);
std::vector<Tensor> _erfinv_bw(const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config);
std::vector<Tensor> _erf_bw(const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config);
std::vector<Tensor> _deg2rad_bw(const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config);
std::vector<Tensor> _hardswish_bw(const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config);
std::vector<Tensor> _tanhshrink_bw(const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config);
std::vector<Tensor> _atanh_bw(const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config);
std::vector<Tensor> _asin_bw(const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config);
std::vector<Tensor> _asinh_bw(const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config);
std::vector<Tensor> _sin_bw(const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config);
std::vector<Tensor> _sinh_bw(const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config);
std::vector<Tensor> _log10_bw(const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config);
std::vector<Tensor> _log1p_bw(const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config);
std::vector<Tensor> _erfc_bw(const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config);
std::vector<Tensor> _relu6_bw(const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config);
std::vector<Tensor> _abs_bw(const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config);
std::vector<Tensor> _silu_bw(const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config);
std::vector<Tensor> _selu_bw(const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config);
std::vector<Tensor> _square_bw(const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config);

std::vector<Tensor> _rpow_bw( const Tensor& grad, const Tensor& input, float exponent, const std::optional<MemoryConfig>& output_mem_config);
std::vector<Tensor> _div_no_nan_bw( const Tensor& grad, const Tensor& input, float scalar, const std::optional<MemoryConfig>& output_mem_config);
std::vector<Tensor> _polygamma_bw( const Tensor& grad, const Tensor& input, int n, const std::optional<MemoryConfig>& output_mem_config);
std::vector<Tensor> _lt_bw( const Tensor& grad, const Tensor& input, float other, const std::optional<MemoryConfig>& output_mem_config);
std::vector<Tensor> _remainder_bw( const Tensor& grad, const Tensor& input, float scalar, const std::optional<MemoryConfig>& output_mem_config);
std::vector<Tensor> _fmod_bw( const Tensor& grad, const Tensor& input, float scalar, const std::optional<MemoryConfig>& output_mem_config);
std::vector<Tensor> _sub_bw( const Tensor& grad, const Tensor& input, float scalar, const std::optional<MemoryConfig>& output_mem_config);
std::vector<Tensor> _gt_bw( const Tensor& grad, const Tensor& input, float other, const std::optional<MemoryConfig>& output_mem_config);

std::vector<Tensor> _assign_bw( const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config);
std::vector<Tensor> _multigammaln_bw( const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config);
std::vector<Tensor> _lgamma_bw( const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config);
std::vector<Tensor> _fill_bw( const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config);
std::vector<Tensor> _hardsigmoid_bw( const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config);
std::vector<Tensor> _cos_bw( const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config);
std::vector<Tensor> _acosh_bw( const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config);
std::vector<Tensor> _relu_bw( const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config);
std::vector<Tensor> _logit_bw( const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config);
std::vector<Tensor> _floor_bw( const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config);
std::vector<Tensor> _round_bw( const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config);
std::vector<Tensor> _log_bw( const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config);

std::vector<Tensor> _softplus_bw( const Tensor& grad, const Tensor& input, float beta = 1.0, float threshold = 20.0, const std::optional<MemoryConfig>& output_mem_config = std::nullopt);
std::vector<Tensor> _hardtanh_bw( const Tensor& grad, const Tensor& input, float min = -1.0, float max = 1.0, const std::optional<MemoryConfig>& output_mem_config = std::nullopt);

std::vector<Tensor> _add_bw( const Tensor& grad, const Tensor& input, float alpha, const std::optional<MemoryConfig>& output_mem_config = std::nullopt);
std::vector<Tensor> _eq_bw( const Tensor& grad, const Tensor& input, float other, const std::optional<MemoryConfig>& output_mem_config = std::nullopt);

std::vector<Tensor> _hardshrink_bw( const Tensor& grad, const Tensor& input, float lambd = 0.5, const std::optional<MemoryConfig>& output_mem_config = std::nullopt);
std::vector<Tensor> _softshrink_bw( const Tensor& grad, const Tensor& input, float lambd = 0.5, const std::optional<MemoryConfig>& output_mem_config = std::nullopt);
std::vector<Tensor> _leaky_relu_bw( const Tensor& grad, const Tensor& input, float negative_slope = 0.01, const std::optional<MemoryConfig>& output_mem_config = std::nullopt);
std::vector<Tensor> _elu_bw( const Tensor& grad, const Tensor& input, float alpha = 1.0, const std::optional<MemoryConfig>& output_mem_config = std::nullopt);
std::vector<Tensor> _celu_bw( const Tensor& grad, const Tensor& input, float aplha = 1.0, const std::optional<MemoryConfig>& output_mem_config = std::nullopt);
std::vector<Tensor> _logiteps_bw( const Tensor& grad, const Tensor& input, float eps = 0.0, const std::optional<MemoryConfig>& output_mem_config = std::nullopt);

std::vector<Tensor> _clamp_bw( const Tensor& grad, const Tensor& input, std::optional<float> min = std::nullopt, std::optional<float> max = std::nullopt, const std::optional<MemoryConfig>& output_mem_config = std::nullopt);

std::vector<Tensor> _rdiv_bw( const Tensor& grad, const Tensor& input, float scalar, string round_mode = "None", const std::optional<MemoryConfig>& output_mem_config = std::nullopt);

std::vector<Tensor> _gelu_bw( const Tensor& grad, const Tensor& input, string approximate = "none", const std::optional<MemoryConfig>& output_mem_config = std::nullopt);

std::vector<Tensor> _repeat_bw(const Tensor& grad, const Tensor& input, const tt::tt_metal::Shape& shape, const std::optional<MemoryConfig>& output_mem_config);

std::vector<std::optional<Tensor>> _pow_bw(uint8_t queue_id, const Tensor& grad, const Tensor& input, float exponent, const MemoryConfig& output_mem_config , const std::vector<bool>& are_required_outputs, std::optional<Tensor> input_grad);

std::vector<std::optional<Tensor>> _exp_bw(uint8_t queue_id, const Tensor& grad, const Tensor& input, const MemoryConfig& output_mem_config, const std::vector<bool>& are_required_outputs, std::optional<Tensor> input_grad);
std::vector<std::optional<Tensor>> _tanh_bw(uint8_t queue_id, const Tensor& grad, const Tensor& input, const MemoryConfig& output_mem_config, const std::vector<bool>& are_required_outputs, std::optional<Tensor> input_grad);
std::vector<std::optional<Tensor>> _sqrt_bw(uint8_t queue_id, const Tensor& grad, const Tensor& input, const MemoryConfig& output_mem_config, const std::vector<bool>& are_required_outputs, std::optional<Tensor> input_grad);

std::vector<Tensor> _prod_bw( const Tensor& grad, const Tensor& input, bool all_dimensions = true, int64_t dim = 0, const std::optional<MemoryConfig>& output_mem_config = std::nullopt);
Tensor change_layout_to_tile(const Tensor& temp, const MemoryConfig& output_mem_config);

// OpHandler struct template
template <UnaryBackwardOpType OpType>
struct OpHandler;

template <>
struct OpHandler<UnaryBackwardOpType::CLAMP_BW> {
    static std::vector<Tensor> handle( const Tensor& grad, const Tensor& input, std::optional<float> min, std::optional<float> max, const std::optional<MemoryConfig>& output_mem_config ) {
        return _clamp_bw(grad, input, min, max, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::HARDTANH_BW> {
    static std::vector<Tensor> handle( const Tensor& grad, const Tensor& input, float min, float max, const std::optional<MemoryConfig>& output_mem_config ) {
        return _hardtanh_bw(grad, input, min, max, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::THRESHOLD_BW> {
    static std::vector<Tensor> handle( const Tensor& grad, const Tensor& input, float threshold, float value, const std::optional<MemoryConfig>& output_mem_config ) {
        return _threshold_bw(grad, input, threshold, value, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::RPOW_BW> {
    static std::vector<Tensor> handle( const Tensor& grad, const Tensor& input, float exponent, const std::optional<MemoryConfig>& output_mem_config ) {
        return _rpow_bw(grad, input, exponent, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::ACOS_BW> {
    static std::vector<Tensor> handle( const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config ) {
        return _acos_bw(grad, input, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::ATAN_BW> {
    static std::vector<Tensor> handle(const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config) {
        return _atan_bw(grad, input, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::RAD2DEG_BW> {
    static std::vector<Tensor> handle(const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config) {
        return _rad2deg_bw(grad, input, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::FRAC_BW> {
    static std::vector<Tensor> handle(const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config) {
        return _frac_bw(grad, input, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::TAN_BW> {
    static std::vector<Tensor> handle(const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config) {
        return _tan_bw(grad, input, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::SIGMOID_BW> {
    static std::vector<Tensor> handle(const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config) {
        return _sigmoid_bw(grad, input, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::RSQRT_BW> {
    static std::vector<Tensor> handle(const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config) {
        return _rsqrt_bw(grad, input, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::NEG_BW> {
    static std::vector<Tensor> handle(const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config) {
        return _neg_bw(grad, input, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::RECIPROCAL_BW> {
    static std::vector<Tensor> handle(const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config) {
        return _reciprocal_bw(grad, input, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::CEIL_BW> {
    static std::vector<Tensor> handle(const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config) {
        return _ceil_bw(grad, input, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::SOFTSIGN_BW> {
    static std::vector<Tensor> handle(const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config) {
        return _softsign_bw(grad, input, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::COSH_BW> {
    static std::vector<Tensor> handle(const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config) {
        return _cosh_bw(grad, input, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::LOG2_BW> {
    static std::vector<Tensor> handle(const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config) {
        return _log2_bw(grad, input, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::SIGN_BW> {
    static std::vector<Tensor> handle(const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config) {
        return _sign_bw(grad, input, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::EXP2_BW> {
    static std::vector<Tensor> handle(const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config) {
        return _exp2_bw(grad, input, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::EXPM1_BW> {
    static std::vector<Tensor> handle(const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config) {
        return _expm1_bw(grad, input, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::DIGAMMA_BW> {
    static std::vector<Tensor> handle(const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config) {
        return _digamma_bw(grad, input, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::ERFINV_BW> {
    static std::vector<Tensor> handle(const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config) {
        return _erfinv_bw(grad, input, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::ERF_BW> {
    static std::vector<Tensor> handle(const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config) {
        return _erf_bw(grad, input, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::DEG2RAD_BW> {
    static std::vector<Tensor> handle(const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config) {
        return _deg2rad_bw(grad, input, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::HARDSWISH_BW> {
    static std::vector<Tensor> handle(const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config) {
        return _hardswish_bw(grad, input, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::TANHSHRINK_BW> {
    static std::vector<Tensor> handle(const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config) {
        return _tanhshrink_bw(grad, input, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::ATANH_BW> {
    static std::vector<Tensor> handle(const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config) {
        return _atanh_bw(grad, input, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::ASIN_BW> {
    static std::vector<Tensor> handle(const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config) {
        return _asin_bw(grad, input, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::ASINH_BW> {
    static std::vector<Tensor> handle(const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config) {
        return _asinh_bw(grad, input, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::SIN_BW> {
    static std::vector<Tensor> handle(const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config) {
        return _sin_bw(grad, input, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::SINH_BW> {
    static std::vector<Tensor> handle(const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config) {
        return _sinh_bw(grad, input, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::LOG10_BW> {
    static std::vector<Tensor> handle(const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config) {
        return _log10_bw(grad, input, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::LOG1P_BW> {
    static std::vector<Tensor> handle(const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config) {
        return _log1p_bw(grad, input, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::ERFC_BW> {
    static std::vector<Tensor> handle(const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config) {
        return _erfc_bw(grad, input, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::TRUNC_BW> {
    static std::vector<Tensor> handle(const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config) {
        return _trunc_bw(grad, input, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::LOG_SIGMOID_BW> {
    static std::vector<Tensor> handle(const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config) {
        return _log_sigmoid_bw(grad, input, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::FILL_ZERO_BW> {
    static std::vector<Tensor> handle(const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config) {
        return _fill_zero_bw(grad, input, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::I0_BW> {
    static std::vector<Tensor> handle(const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config) {
        return _i0_bw(grad, input, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::RELU6_BW> {
    static std::vector<Tensor> handle(const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config) {
        return _relu6_bw(grad, input, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::ABS_BW> {
    static std::vector<Tensor> handle(const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config) {
        return _abs_bw(grad, input, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::SILU_BW> {
    static std::vector<Tensor> handle(const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config) {
        return _silu_bw(grad, input, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::SELU_BW> {
    static std::vector<Tensor> handle(const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config) {
        return _selu_bw(grad, input, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::SQUARE_BW> {
    static std::vector<Tensor> handle(const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config) {
        return _square_bw(grad, input, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::DIV_NO_NAN_BW> {
    static std::vector<Tensor> handle( const Tensor& grad, const Tensor& input, float exponent, const std::optional<MemoryConfig>& output_mem_config ) {
        return _div_no_nan_bw(grad, input, exponent, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::POLYGAMMA_BW> {
    static std::vector<Tensor> handle( const Tensor& grad, const Tensor& input, int n, const std::optional<MemoryConfig>& output_mem_config ) {
        return _polygamma_bw(grad, input, n, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::LT_BW> {
    static std::vector<Tensor> handle( const Tensor& grad, const Tensor& input, float other, const std::optional<MemoryConfig>& output_mem_config ) {
        return _lt_bw(grad, input, other, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::HARDSHRINK_BW> {
    static std::vector<Tensor> handle( const Tensor& grad, const Tensor& input, float lambd, const std::optional<MemoryConfig>& output_mem_config ) {
        return _hardshrink_bw(grad, input, lambd, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::SOFTSHRINK_BW> {
    static std::vector<Tensor> handle( const Tensor& grad, const Tensor& input, float lambd, const std::optional<MemoryConfig>& output_mem_config ) {
        return _softshrink_bw(grad, input, lambd, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::LEAKY_RELU_BW> {
    static std::vector<Tensor> handle( const Tensor& grad, const Tensor& input, float negative_slope, const std::optional<MemoryConfig>& output_mem_config ) {
        return _leaky_relu_bw(grad, input, negative_slope, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::ELU_BW> {
    static std::vector<Tensor> handle( const Tensor& grad, const Tensor& input, float alpha, const std::optional<MemoryConfig>& output_mem_config ) {
        return _elu_bw(grad, input, alpha, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::CELU_BW> {
    static std::vector<Tensor> handle( const Tensor& grad, const Tensor& input, float alpha, const std::optional<MemoryConfig>& output_mem_config ) {
        return _celu_bw(grad, input, alpha, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::LOGITEPS_BW> {
    static std::vector<Tensor> handle( const Tensor& grad, const Tensor& input, float eps, const std::optional<MemoryConfig>& output_mem_config ) {
        return _logiteps_bw(grad, input, eps, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::REMAINDER_BW> {
    static std::vector<Tensor> handle( const Tensor& grad, const Tensor& input, float scalar, const std::optional<MemoryConfig>& output_mem_config ) {
        return _remainder_bw(grad, input, scalar, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::FMOD_BW> {
    static std::vector<Tensor> handle( const Tensor& grad, const Tensor& input, float scalar, const std::optional<MemoryConfig>& output_mem_config ) {
        return _fmod_bw(grad, input, scalar, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::GT_BW> {
    static std::vector<Tensor> handle( const Tensor& grad, const Tensor& input, float other, const std::optional<MemoryConfig>& output_mem_config ) {
        return _gt_bw(grad, input, other, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::ASSIGN_BW> {
    static std::vector<Tensor> handle( const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config ) {
        return _assign_bw(grad, input, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::MULTIGAMMALN_BW> {
    static std::vector<Tensor> handle( const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config ) {
        return _multigammaln_bw(grad, input, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::LGAMMA_BW> {
    static std::vector<Tensor> handle( const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config ) {
        return _lgamma_bw(grad, input, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::FILL_BW> {
    static std::vector<Tensor> handle( const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config ) {
        return _fill_bw(grad, input, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::HARDSIGMOID_BW> {
    static std::vector<Tensor> handle( const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config ) {
        return _hardsigmoid_bw(grad, input, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::COS_BW> {
    static std::vector<Tensor> handle( const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config ) {
        return _cos_bw(grad, input, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::ACOSH_BW> {
    static std::vector<Tensor> handle( const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config ) {
        return _acosh_bw(grad, input, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::RELU_BW> {
    static std::vector<Tensor> handle( const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config ) {
        return _relu_bw(grad, input, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::LOGIT_BW> {
    static std::vector<Tensor> handle( const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config ) {
        return _logit_bw(grad, input, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::FLOOR_BW> {
    static std::vector<Tensor> handle( const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config ) {
        return _floor_bw(grad, input, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::ROUND_BW> {
    static std::vector<Tensor> handle( const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config ) {
        return _round_bw(grad, input, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::LOG_BW> {
    static std::vector<Tensor> handle( const Tensor& grad, const Tensor& input, const std::optional<MemoryConfig>& output_mem_config ) {
        return _log_bw(grad, input, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::SUB_BW> {
    static std::vector<Tensor> handle( const Tensor& grad, const Tensor& input, float scalar, const std::optional<MemoryConfig>& output_mem_config ) {
        return _sub_bw(grad, input, scalar, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::SOFTPLUS_BW> {
    static std::vector<Tensor> handle( const Tensor& grad, const Tensor& input, float beta, float threshold, const std::optional<MemoryConfig>& output_mem_config ) {
        return _softplus_bw(grad, input, beta, threshold, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::RDIV_BW> {
    static std::vector<Tensor> handle( const Tensor& grad, const Tensor& input, float scalar, string round_mode, const std::optional<MemoryConfig>& output_mem_config ) {
        return _rdiv_bw(grad, input, scalar, round_mode, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::POW_BW> {
    static std::vector<std::optional<Tensor>> handle( uint8_t queue_id, const Tensor& grad, const Tensor& input, float exponent, const MemoryConfig& output_mem_config, const std::vector<bool>& are_required_outputs, std::optional<Tensor> input_grad ) {
        return _pow_bw(queue_id, grad, input, exponent, output_mem_config, are_required_outputs, input_grad);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::EXP_BW> {
    static std::vector<std::optional<Tensor>> handle( uint8_t queue_id, const Tensor& grad, const Tensor& input, const MemoryConfig& output_mem_config, const std::vector<bool>& are_required_outputs, std::optional<Tensor> input_grad ) {
        return _exp_bw(queue_id, grad, input, output_mem_config, are_required_outputs, input_grad);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::TANH_BW> {
    static std::vector<std::optional<Tensor>> handle( uint8_t queue_id, const Tensor& grad, const Tensor& input, const MemoryConfig& output_mem_config, const std::vector<bool>& are_required_outputs, std::optional<Tensor> input_grad ) {
        return _tanh_bw(queue_id, grad, input, output_mem_config, are_required_outputs, input_grad);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::SQRT_BW> {
    static std::vector<std::optional<Tensor>> handle( uint8_t queue_id, const Tensor& grad, const Tensor& input, const MemoryConfig& output_mem_config, const std::vector<bool>& are_required_outputs, std::optional<Tensor> input_grad ) {
        return _sqrt_bw(queue_id, grad, input, output_mem_config, are_required_outputs, input_grad);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::GELU_BW> {
    static std::vector<Tensor> handle( const Tensor& grad, const Tensor& input, string approximate, const std::optional<MemoryConfig>& output_mem_config ) {
        return _gelu_bw(grad, input, approximate, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::REPEAT_BW> {
    static std::vector<Tensor> handle( const Tensor& grad, const Tensor& input, const tt::tt_metal::Shape& shape, const std::optional<MemoryConfig>& output_mem_config ) {
        return _repeat_bw(grad, input, shape, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::PROD_BW> {
    static std::vector<Tensor> handle( const Tensor& grad, const Tensor& input, bool all_dimensions, int64_t dim, const std::optional<MemoryConfig>& output_mem_config ) {
        return _prod_bw(grad, input, all_dimensions, dim, output_mem_config);
    }
};

template <>
struct OpHandler<UnaryBackwardOpType::EQ_BW> {
    static std::vector<Tensor> handle( const Tensor& grad, const Tensor& input, float other, const std::optional<MemoryConfig>& output_mem_config ) {
        return _eq_bw(grad, input, other, output_mem_config);
    }
};

}  // namespace ttnn::operations::unary_backward
