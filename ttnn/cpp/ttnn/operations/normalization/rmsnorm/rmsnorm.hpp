// SPDX-FileCopyrightText: © 2024 Tenstorrent Inc.
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ttnn/operations/normalization/layernorm/device/layernorm_op.hpp"

namespace ttnn {
namespace operations::normalization {

struct ExecuteRMSNorm {
    static inline ttnn::Tensor operator()(
        const ttnn::Tensor& input_tensor,
        float epsilon = 1e-12,
        const std::optional<const ttnn::Tensor>& weight = std::nullopt,
        const std::optional<const ttnn::Tensor>& bias = std::nullopt,
        const std::optional<const ttnn::Tensor>& residual_input_tensor = std::nullopt,
        const std::optional<MemoryConfig>& memory_config = std::nullopt,
        const std::optional<const LayerNormProgramConfig>& program_config = std::nullopt,
        const std::optional<const DeviceComputeKernelConfig> compute_kernel_config = std::nullopt) {
        auto arch = input_tensor.storage_type() == StorageType::DEVICE ? input_tensor.device()->arch() : AutoFormat::GetDefaultDevice()->arch();
        auto kernel_config_val = init_device_compute_kernel_config(arch, compute_kernel_config, MathFidelity::HiFi4, true, false, false);
        return operation::run(
                    LayerNorm{
                        .norm_type = LayerNormType::RMSNORM,
                        .eps = epsilon,
                        .output_mem_config = memory_config.value_or(input_tensor.memory_config()),
                        .program_config = program_config.value_or(LayerNormDefaultProgramConfig{}),
                        .compute_kernel_config = kernel_config_val},
                    {input_tensor},
                    {residual_input_tensor, weight, bias}).at(0);
    }
};

}  // namespace operations::normalization

constexpr auto rms_norm = ttnn::register_operation_with_auto_launch_op<"ttnn::rms_norm", ttnn::operations::normalization::ExecuteRMSNorm>();

}  // namespace ttnn
