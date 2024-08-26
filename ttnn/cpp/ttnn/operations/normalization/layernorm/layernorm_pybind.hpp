// SPDX-FileCopyrightText: © 2024 Tenstorrent Inc.
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "ttnn/cpp/pybind11/decorators.hpp"
#include "layernorm.hpp"

namespace py = pybind11;

namespace ttnn::operations::normalization::detail {

void bind_normalization_layernorm_program_config_operation(py::module& module) {
    py::class_<LayerNormProgramConfig>(module, "LayerNormProgramConfig").def(py::init<>());

    py::class_<LayerNormDefaultProgramConfig>(module, "LayerNormDefaultProgramConfig")
        .def(py::init<>());

    py::class_<LayerNormShardedMultiCoreProgramConfig>(module, "LayerNormShardedMultiCoreProgramConfig")
        .def(
            py::init<CoreCoord, std::size_t, std::size_t, std::size_t, bool>(),
            py::kw_only(),
            py::arg("compute_with_storage_grid_size"),
            py::arg("subblock_w").noconvert(),
            py::arg("block_h").noconvert(),
            py::arg("block_w").noconvert(),
            py::arg("inplace").noconvert())
        .def(
            "__repr__", [](const LayerNormShardedMultiCoreProgramConfig& config) { return fmt::format("{}", config); });
}

void bind_normalization_layer_norm_operation(py::module& module) {

    ttnn::bind_registered_operation(
        module,
        ttnn::layer_norm,
        R"doc(rms_norm(input_tensor: ttnn.Tensor, epsilon: float = 1e-12, weight: Optional[ttnn.Tensor] = None, bias: Optional[ttnn.Tensor] = None, residual_input_tensor: Optional[ttnn.Tensor] = None, memory_config: Optional[ttnn.MemoryConfig] = None, program_config: Optional[ttnn.ProgramConfig] = None) -> ttnn.Tensor
            Compute layer_norm over :attr:`input_tensor`.
        )doc",
        ttnn::pybind_arguments_t{
            py::arg("input_tensor"),
            py::kw_only(),
            py::arg("epsilon") = 1e-12,
            py::arg("weight") = std::nullopt,
            py::arg("bias") = std::nullopt,
            py::arg("residual_input_tensor") = std::nullopt,
            py::arg("memory_config") = std::nullopt,
            py::arg("program_config") = std::nullopt,
            py::arg("compute_kernel_config") = std::nullopt});
}

}  // namespace ttnn::operations::normalization::detail
