// SPDX-FileCopyrightText: © 2023 Tenstorrent Inc.
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "ttnn/cpp/pybind11/decorators.hpp"
#include "ttnn/operations/eltwise/binary_backward/binary_backward.hpp"
#include "ttnn/operations/eltwise/ternary_backward/ternary_backward.hpp"
#include "ttnn/types.hpp"

namespace py = pybind11;

namespace ttnn {
namespace operations {
namespace binary_backward {

namespace detail {

template <typename binary_backward_operation_t>
void bind_binary_backward_ops(py::module& module, const binary_backward_operation_t& operation, std::string_view description, std::string_view supported_dtype) {
    auto doc = fmt::format(
        R"doc({0}(grad_tensor: ttnn.Tensor, input_tensor_a: ttnn.Tensor, input_tensor_b: ttnn.Tensor, *, memory_config: ttnn.MemoryConfig) -> std::vector<Tensor>

{2}

Args:
    * :attr:`grad_tensor`
    * :attr:`input_tensor_a`
    * :attr:`input_tensor_b`

Keyword args:
    * :attr:`memory_config` (Optional[ttnn.MemoryConfig]): memory config for the output tensor

Supported dtypes, layouts, and ranks:

{3}

Note : bfloat8_b/bfloat4_b is only supported on TILE_LAYOUT

Example:

    >>> grad_tensor = ttnn.to_device(ttnn.from_torch(torch.tensor((1, 2), dtype=torch.bfloat16)), device)
    >>> tensor1 = ttnn.to_device(ttnn.from_torch(torch.tensor((1, 2), dtype=torch.bfloat16)), device)
    >>> tensor2 = ttnn.to_device(ttnn.from_torch(torch.tensor((0, 1), dtype=torch.bfloat16)), device)
    >>> output = {1}(grad_tensor, tensor1, tensor2)
)doc",
        operation.base_name(),
        operation.python_fully_qualified_name(),
        description,
        supported_dtype);

    bind_registered_operation(
        module,
        operation,
        doc,
        ttnn::pybind_overload_t{
            [](const binary_backward_operation_t& self,
               const ttnn::Tensor& grad_tensor,
               const ttnn::Tensor& input_tensor_a,
               const ttnn::Tensor& input_tensor_b,
               const std::optional<ttnn::MemoryConfig>& memory_config) -> std::vector<ttnn::Tensor> {
                auto output_memory_config = memory_config.value_or(input_tensor_a.memory_config());
                return self(grad_tensor, input_tensor_a, input_tensor_b, output_memory_config);
            },
            py::arg("grad_tensor"),
            py::arg("input_tensor_a"),
            py::arg("input_tensor_b"),
            py::kw_only(),
            py::arg("memory_config") = std::nullopt});
}

template <typename binary_backward_operation_t>
void bind_binary_backward_int_default(py::module& module, const binary_backward_operation_t& operation, const std::string& parameter_name, const std::string& parameter_doc, int parameter_value, std::string_view description, std::string_view supported_dtype) {
    auto doc = fmt::format(
        R"doc({0}(grad_tensor: ttnn.Tensor, input_tensor_a: ttnn.Tensor, input_tensor_b: ttnn.Tensor, {2}: int, *, memory_config: ttnn.MemoryConfig) -> std::vector<Tensor>

        {5}

        Args:
            * :attr:`grad_tensor`
            * :attr:`input_tensor_a`
            * :attr:`input_tensor_b`
            * :attr:`{2}` (int):`{3}`, Default value = {4}

        Keyword args:
            * :attr:`memory_config` (Optional[ttnn.MemoryConfig]): memory config for the output tensor

        Supported dtypes, layouts, and ranks:

        {6}

        Note : bfloat8_b/bfloat4_b is only supported on TILE_LAYOUT

        Example:

            >>> grad_tensor = ttnn.to_device(ttnn.from_torch(torch.tensor((1, 2), dtype=torch.bfloat16)), device)
            >>> tensor1 = ttnn.to_device(ttnn.from_torch(torch.tensor((1, 2), dtype=torch.bfloat16)), device)
            >>> tensor2 = ttnn.to_device(ttnn.from_torch(torch.tensor((0, 1), dtype=torch.bfloat16)), device)
            >>> output = {1}(grad_tensor, tensor1, tensor2, int)
        )doc",
        operation.base_name(),
        operation.python_fully_qualified_name(),
        parameter_name,
        parameter_doc,
        parameter_value,
        description,
        supported_dtype);


    bind_registered_operation(
        module,
        operation,
        doc,
        ttnn::pybind_overload_t{
            [](const binary_backward_operation_t& self,
               const ttnn::Tensor& grad_tensor,
               const ttnn::Tensor& input_tensor_a,
               const ttnn::Tensor& input_tensor_b,
               int parameter,
               const std::optional<ttnn::MemoryConfig>& memory_config) -> std::vector<ttnn::Tensor> {
                return self(grad_tensor, input_tensor_a, input_tensor_b, parameter, memory_config);
            },
            py::arg("grad_tensor"),
            py::arg("input_tensor_a"),
            py::arg("input_tensor_b"),
            py::arg(parameter_name.c_str()) = parameter_value,
            py::kw_only(),
            py::arg("memory_config") = std::nullopt}
    );
}

template <typename binary_backward_operation_t>
void bind_binary_backward_opt_float_default(py::module& module, const binary_backward_operation_t& operation, const std::string& parameter_name, const std::string& parameter_doc, float parameter_value, std::string_view description, std::string_view supported_dtype) {
    auto doc = fmt::format(
        R"doc({0}(grad_tensor: ttnn.Tensor, input_tensor_a: ttnn.Tensor, input_tensor_b: ttnn.Tensor, {2}: float, *, memory_config: ttnn.MemoryConfig) -> std::vector<Tensor>

        {5}

        Args:
            * :attr:`grad_tensor`
            * :attr:`input_tensor_a`
            * :attr:`input_tensor_b`
            * :attr:`{2}` (float):`{3}`,Default value = {4}

        Keyword args:
            * :attr:`memory_config` (Optional[ttnn.MemoryConfig]): memory config for the output tensor

        Supported dtypes, layouts, and ranks:

        {6}

        Note : bfloat8_b/bfloat4_b is only supported on TILE_LAYOUT

        Example:

            >>> grad_tensor = ttnn.to_device(ttnn.from_torch(torch.tensor((1, 2), dtype=torch.bfloat16)), device)
            >>> tensor1 = ttnn.to_device(ttnn.from_torch(torch.tensor((1, 2), dtype=torch.bfloat16)), device)
            >>> tensor2 = ttnn.to_device(ttnn.from_torch(torch.tensor((0, 1), dtype=torch.bfloat16)), device)
            >>> output = {1}(grad_tensor, tensor1, tensor2, float)
        )doc",
        operation.base_name(),
        operation.python_fully_qualified_name(),
        parameter_name,
        parameter_doc,
        parameter_value,
        description,
        supported_dtype);


    bind_registered_operation(
        module,
        operation,
        doc,
        ttnn::pybind_overload_t{
            [](const binary_backward_operation_t& self,
               const ttnn::Tensor& grad_tensor,
               const ttnn::Tensor& input_tensor_a,
               const ttnn::Tensor& input_tensor_b,
               float parameter,
               const std::optional<ttnn::MemoryConfig>& memory_config,
               const std::vector<bool>& are_required_outputs,
               const std::optional<ttnn::Tensor>& input_a_grad,
               const std::optional<ttnn::Tensor>& input_b_grad,
               const uint8_t& queue_id) -> std::vector<std::optional<ttnn::Tensor>> {
                return self(queue_id, grad_tensor, input_tensor_a, input_tensor_b, parameter, memory_config, are_required_outputs, input_a_grad, input_b_grad);
            },
            py::arg("grad_tensor"),
            py::arg("input_tensor_a"),
            py::arg("input_tensor_b"),
            py::arg(parameter_name.c_str()) = parameter_value,
            py::kw_only(),
            py::arg("memory_config") = std::nullopt,
            py::arg("are_required_outputs") = std::vector<bool>{true, true},
            py::arg("input_a_grad") = std::nullopt,
            py::arg("input_b_grad") = std::nullopt,
            py::arg("queue_id") = 0}
    );
}

template <typename binary_backward_operation_t>
void bind_binary_backward_float_string_default(
    py::module& module,
    const binary_backward_operation_t& operation,
    const std::string& parameter_name_a,
    const std::string& parameter_a_doc,
    const std::string& parameter_name_b,
    const std::string& parameter_b_doc,
    string parameter_b_value,
    const std::string& description,
    const std::string& supported_dtype) {
    auto doc = fmt::format(
        R"doc({0}(grad_tensor: ttnn.Tensor, ( input_tensor: ttnn.Tensor, {2}: float ) or  ( input_tensor_a: ttnn.Tensor, input_tensor_b: ttnn.Tensor ), {4}: string, *, memory_config: ttnn.MemoryConfig) -> std::vector<Tensor>

        {7}

        Args:
            * :attr:`grad_tensor`
            * :attr:`input_tensor_a` or :attr:`input_tensor`
            * :attr:`input_tensor_b` or :attr:`{2}` (float): {3}

        Keyword args:
            * :attr:`{4}` (string): {5} , Default value = {6}
            * :attr:`memory_config` [ttnn.MemoryConfig]: memory config for the output tensor

        Supported dtypes, layouts, and ranks:

        {8}

        Note : bfloat8_b/bfloat4_b is only supported on TILE_LAYOUT

        Example:

            >>> grad_tensor = ttnn.to_device(ttnn.from_torch(torch.tensor((1, 2), dtype=torch.bfloat16)), device)
            >>> input = ttnn.to_device(ttnn.from_torch(torch.tensor((1, 2), dtype=torch.bfloat16)), device)
            >>> output = {1}(grad_tensor, input, {2}, {4} = {6})
        )doc",
        operation.base_name(),
        operation.python_fully_qualified_name(),
        parameter_name_a,
        parameter_a_doc,
        parameter_name_b,
        parameter_b_doc,
        parameter_b_value,
        description,
        supported_dtype);

    bind_registered_operation(
        module,
        operation,
        doc,
        ttnn::pybind_overload_t{
            [](const binary_backward_operation_t& self,
               const ttnn::Tensor& grad_tensor,
               const ttnn::Tensor& input_tensor_a,
               const ttnn::Tensor& input_tensor_b,
               string parameter_b,
               const std::optional<MemoryConfig>& memory_config) {
                return self(grad_tensor, input_tensor_a, input_tensor_b, parameter_b, memory_config);
            },
            py::arg("grad_tensor"),
            py::arg("input_tensor_a"),
            py::arg("input_tensor_b"),
            py::kw_only(),
            py::arg(parameter_name_b.c_str()) = parameter_b_value,
            py::arg("memory_config") = std::nullopt},

        ttnn::pybind_overload_t{
            [](const binary_backward_operation_t& self,
               const ttnn::Tensor& grad_tensor,
               const ttnn::Tensor& input_tensor,
               float parameter_a,
               string parameter_b,
               const std::optional<MemoryConfig>& memory_config) {
                return self(grad_tensor, input_tensor, parameter_a, parameter_b, memory_config);
            },
            py::arg("grad_tensor"),
            py::arg("input_tensor"),
            py::arg(parameter_name_a.c_str()),
            py::kw_only(),
            py::arg(parameter_name_b.c_str()) = parameter_b_value,
            py::arg("memory_config") = std::nullopt});
}

template <typename binary_backward_operation_t>
void bind_binary_backward_float_default(py::module& module, const binary_backward_operation_t& operation, const std::string& parameter_name, const std::string& parameter_doc, float parameter_value, std::string_view description, std::string_view supported_dtype) {
    auto doc = fmt::format(
        R"doc({0}(grad_tensor: ttnn.Tensor, input_tensor_a: ttnn.Tensor, input_tensor_b: ttnn.Tensor, {2}: float, *, memory_config: ttnn.MemoryConfig) -> std::vector<Tensor>

        {5}

        Args:
            * :attr:`grad_tensor`
            * :attr:`input_tensor_a`
            * :attr:`input_tensor_b`
            * :attr:`{2}` (float):`{3}`,Default value = {4}

        Keyword args:
            * :attr:`memory_config` (Optional[ttnn.MemoryConfig]): memory config for the output tensor

        Supported dtypes, layouts, and ranks:

        {6}

        Note : bfloat8_b/bfloat4_b is only supported on TILE_LAYOUT

        Example:

            >>> grad_tensor = ttnn.to_device(ttnn.from_torch(torch.tensor((1, 2), dtype=torch.bfloat16)), device)
            >>> tensor1 = ttnn.to_device(ttnn.from_torch(torch.tensor((1, 2), dtype=torch.bfloat16)), device)
            >>> tensor2 = ttnn.to_device(ttnn.from_torch(torch.tensor((0, 1), dtype=torch.bfloat16)), device)
            >>> output = {1}(grad_tensor, tensor1, tensor2, float)
        )doc",
        operation.base_name(),
        operation.python_fully_qualified_name(),
        parameter_name,
        parameter_doc,
        parameter_value,
        description,
        supported_dtype);


    bind_registered_operation(
        module,
        operation,
        doc,
        ttnn::pybind_overload_t{
            [](const binary_backward_operation_t& self,
               const ttnn::Tensor& grad_tensor,
               const ttnn::Tensor& input_tensor_a,
               const ttnn::Tensor& input_tensor_b,
               float parameter,
               const std::optional<ttnn::MemoryConfig>& memory_config) -> std::vector<ttnn::Tensor> {
                return self(grad_tensor, input_tensor_a, input_tensor_b, parameter, memory_config);
            },
            py::arg("grad_tensor"),
            py::arg("input_tensor_a"),
            py::arg("input_tensor_b"),
            py::arg(parameter_name.c_str()) = parameter_value,
            py::kw_only(),
            py::arg("memory_config") = std::nullopt}
    );
}

template <typename binary_backward_operation_t>
void bind_binary_bw_mul(py::module& module, const binary_backward_operation_t& operation, std::string_view description, std::string_view supported_dtype) {
    auto doc = fmt::format(
        R"doc({0}(input_tensor_a: Union[ttnn.Tensor, ComplexTensor] , input_tensor_b: Union[ComplexTensor, ttnn.Tensor, int, float], *, memory_config: Optional[ttnn.MemoryConfig] = None, dtype: Optional[ttnn.DataType] = None, activations: Optional[List[str]] = None) -> ttnn.Tensor or ComplexTensor

        {2}
        Supports broadcasting.

        Args:
            * :attr:`input_tensor_a` (ComplexTensor or ttnn.Tensor)
            * :attr:`input_tensor_b` (ComplexTensor or ttnn.Tensor or Number): the tensor or number to add to :attr:`input_tensor_a`.

        Keyword args:
            * :attr:`memory_config` (Optional[ttnn.MemoryConfig]): memory config for the output tensor
            * :attr:`dtype` (Optional[ttnn.DataType]): data type for the output tensor
            * :attr:`output_tensor` (Optional[ttnn.Tensor]): preallocated output tensor
            * :attr:`activations` (Optional[List[str]]): list of activation functions to apply to the output tensor
            * :attr:`queue_id` (Optional[uint8]): command queue id

        Supported dtypes, layouts, and ranks:

        {3}

        Note : bfloat8_b/bfloat4_b is only supported on TILE_LAYOUT

        Example:

            >>> tensor1 = ttnn.to_device(ttnn.from_torch(torch.tensor((1, 2), dtype=torch.bfloat16)), device)
            >>> tensor2 = ttnn.to_device(ttnn.from_torch(torch.tensor((0, 1), dtype=torch.bfloat16)), device)
            >>> output = {1}(tensor1, tensor2)

        )doc",
        operation.base_name(),
        operation.python_fully_qualified_name(),
        description,
        supported_dtype);

    bind_registered_operation(
        module,
        operation,
        doc,
        // tensor and scalar
        ttnn::pybind_overload_t{
            [](const binary_backward_operation_t& self,
               const Tensor& grad_tensor,
               const Tensor& input_tensor_a,
               const float scalar,
               const std::optional<MemoryConfig>& memory_config){
                return self(grad_tensor, input_tensor_a, scalar, memory_config);
            },
            py::arg("grad_tensor"),
            py::arg("input_tensor_a"),
            py::arg("scalar"),
            py::kw_only(),
            py::arg("memory_config") = std::nullopt},

        // tensor and tensor
        ttnn::pybind_overload_t{
            [](const binary_backward_operation_t& self,
               const ttnn::Tensor& grad_tensor,
               const ttnn::Tensor& input_tensor,
               const ttnn::Tensor& other_tensor,
               const std::optional<ttnn::MemoryConfig>& memory_config,
               const std::vector<bool>& are_required_outputs,
               const std::optional<ttnn::Tensor>& input_grad,
               const std::optional<ttnn::Tensor>& other_grad,
               const uint8_t& queue_id) -> std::vector<std::optional<ttnn::Tensor>> {
                return self(queue_id, grad_tensor, input_tensor, other_tensor, memory_config, are_required_outputs, input_grad, other_grad);
            },
            py::arg("grad_tensor"),
            py::arg("input_tensor"),
            py::arg("other_tensor"),
            py::kw_only(),
            py::arg("memory_config") = std::nullopt,
            py::arg("are_required_outputs") = std::vector<bool>{true, true},
            py::arg("input_grad") = std::nullopt,
            py::arg("other_grad") = std::nullopt,
            py::arg("queue_id") = 0},

        // complex tensor
        ttnn::pybind_overload_t{
            [](const binary_backward_operation_t& self,
               const ComplexTensor& grad_tensor,
               const ComplexTensor& input_tensor_a,
               const ComplexTensor& input_tensor_b,
               const MemoryConfig& memory_config) {
                return self(grad_tensor, input_tensor_a, input_tensor_b, memory_config);
            },
            py::arg("grad_tensor"),
            py::arg("input_tensor_a"),
            py::arg("input_tensor_b"),
            py::kw_only(),
            py::arg("memory_config") = std::nullopt});
}

template <typename binary_backward_operation_t>
void bind_binary_bw_div(py::module& module, const binary_backward_operation_t& operation, std::string_view description, std::string_view supported_dtype) {
    auto doc = fmt::format(
        R"doc({0}(input_tensor_a: Union[ttnn.Tensor, ComplexTensor] , input_tensor_b: Union[ComplexTensor, ttnn.Tensor, int, float], round_mode: string *, memory_config: Optional[ttnn.MemoryConfig] = None, dtype: Optional[ttnn.DataType] = None, activations: Optional[List[str]] = None) -> ttnn.Tensor or ComplexTensor

        {2}
        Supports broadcasting.

        Args:
            * :attr:`input_tensor_a` (ComplexTensor or ttnn.Tensor)
            * :attr:`input_tensor_b` (ComplexTensor or ttnn.Tensor or Number): the tensor or number to add to :attr:`input_tensor_a`.

        Keyword args:
            * :attr:`memory_config` (Optional[ttnn.MemoryConfig]): memory config for the output tensor
            * :attr:`dtype` (Optional[ttnn.DataType]): data type for the output tensor

        Supported dtypes, layouts, and ranks:

        {3}

        Note : bfloat8_b/bfloat4_b is only supported on TILE_LAYOUT

        Example:

            >>> tensor1 = ttnn.to_device(ttnn.from_torch(torch.tensor((1, 2), dtype=torch.bfloat16)), device)
            >>> tensor2 = ttnn.to_device(ttnn.from_torch(torch.tensor((0, 1), dtype=torch.bfloat16)), device)
            >>> output = {1}(tensor1, tensor2)

        )doc",
        operation.base_name(),
        operation.python_fully_qualified_name(),
        description,
        supported_dtype);

    bind_registered_operation(
        module,
        operation,
        doc,
        // tensor and scalar
        ttnn::pybind_overload_t{
            [](const binary_backward_operation_t& self,
               const Tensor& grad_tensor,
               const Tensor& input_tensor_a,
               const float scalar,
               std::string round_mode,
               const std::optional<MemoryConfig>& memory_config){
                return self(grad_tensor, input_tensor_a, scalar, round_mode, memory_config);
            },
            py::arg("grad_tensor"),
            py::arg("input_tensor_a"),
            py::arg("scalar"),
            py::kw_only(),
            py::arg("round_mode") = "None",
            py::arg("memory_config") = std::nullopt},

        // tensor and tensor
        ttnn::pybind_overload_t{
            [](const binary_backward_operation_t& self,
               const ttnn::Tensor& grad_tensor,
               const ttnn::Tensor& input_tensor,
               const ttnn::Tensor& other_tensor,
               std::string round_mode,
               const std::optional<ttnn::MemoryConfig>& memory_config) {
                return self(grad_tensor, input_tensor, other_tensor, round_mode, memory_config);
            },
            py::arg("grad_tensor"),
            py::arg("input_tensor"),
            py::arg("other_tensor"),
            py::kw_only(),
            py::arg("round_mode") = "None",
            py::arg("memory_config") = std::nullopt},

        // complex tensor
        ttnn::pybind_overload_t{
            [](const binary_backward_operation_t& self,
               const ComplexTensor& grad_tensor,
               const ComplexTensor& input_tensor_a,
               const ComplexTensor& input_tensor_b,
               const MemoryConfig& memory_config) {
                return self(grad_tensor, input_tensor_a, input_tensor_b, memory_config);
            },
            py::arg("grad_tensor"),
            py::arg("input_tensor_a"),
            py::arg("input_tensor_b"),
            py::kw_only(),
            py::arg("memory_config") = std::nullopt});
}

template <typename binary_backward_operation_t>
void bind_binary_bw_comparison(py::module& module, const binary_backward_operation_t& operation, std::string_view description, std::string_view supported_dtype) {
    auto doc = fmt::format(
        R"doc({0}(input_tensor_a: ttnn.Tensor , input_tensor_b: Union[ttnn.Tensor, int, float], *, memory_config: Optional[ttnn.MemoryConfig] = None) -> ttnn.Tensor

        {2}
        Supports broadcasting.

        Args:
            * :attr:`input_tensor_a` ttnn.Tensor
            * :attr:`input_tensor_b` (ttnn.Tensor or Number)

        Keyword args:
            * :attr:`memory_config` (Optional[ttnn.MemoryConfig]): memory config for the output tensor

        Supported dtypes, layouts, and ranks:

        {3}

        Note : bfloat8_b/bfloat4_b is only supported on TILE_LAYOUT

        Example:

            >>> tensor1 = ttnn.to_device(ttnn.from_torch(torch.tensor((1, 2), dtype=torch.bfloat16)), device)
            >>> tensor2 = ttnn.to_device(ttnn.from_torch(torch.tensor((0, 1), dtype=torch.bfloat16)), device)
            >>> output = {1}(tensor1, tensor2)
        )doc",
        operation.base_name(),
        operation.python_fully_qualified_name(),
        description,
        supported_dtype);

    bind_registered_operation(
        module,
        operation,
        doc,
        // tensor and scalar
        ttnn::pybind_overload_t{
            [](const binary_backward_operation_t& self,
               const Tensor& grad_tensor,
               const Tensor& input_tensor,
               const float scalar,
               const std::optional<MemoryConfig>& memory_config){
                return self(grad_tensor, input_tensor, scalar, memory_config);
            },
            py::arg("grad_tensor"),
            py::arg("input_tensor"),
            py::arg("scalar"),
            py::kw_only(),
            py::arg("memory_config") = std::nullopt},

        // tensor and tensor
        ttnn::pybind_overload_t{
            [](const binary_backward_operation_t& self,
               const Tensor& grad_tensor,
               const Tensor& input_tensor_a,
               const Tensor& input_tensor_b,
               const std::optional<MemoryConfig>& memory_config) {
                return self(grad_tensor, input_tensor_a, input_tensor_b, memory_config);
            },
            py::arg("grad_tensor"),
            py::arg("input_tensor_a"),
            py::arg("input_tensor_b"),
            py::kw_only(),
            py::arg("memory_config") = std::nullopt});
}

template <typename binary_backward_operation_t>
void bind_binary_bw_operation(py::module& module, const binary_backward_operation_t& operation, std::string_view description, std::string_view supported_dtype) {
    auto doc = fmt::format(
        R"doc({0}(input_tensor_a: Union[ttnn.Tensor, ComplexTensor] , input_tensor_b: Union[ComplexTensor, ttnn.Tensor, int, float], *, memory_config: Optional[ttnn.MemoryConfig] = None) -> ttnn.Tensor or ComplexTensor

        {2}

        Supports broadcasting.

        Args:
            * :attr:`input_tensor_a` (ComplexTensor or ttnn.Tensor)
            * :attr:`input_tensor_b` (ComplexTensor or ttnn.Tensor or Number): the tensor or number to add to :attr:`input_tensor_a`.

        Keyword args:
            * :attr:`memory_config` (Optional[ttnn.MemoryConfig]): memory config for the output tensor

        Supported dtypes, layouts, and ranks:

        {3}

        Note : bfloat8_b/bfloat4_b is only supported on TILE_LAYOUT

        Example:

            >>> tensor1 = ttnn.to_device(ttnn.from_torch(torch.tensor((1, 2), dtype=torch.bfloat16)), device)
            >>> tensor2 = ttnn.to_device(ttnn.from_torch(torch.tensor((0, 1), dtype=torch.bfloat16)), device)
            >>> output = {1}(tensor1, tensor2)
        )doc",
        operation.base_name(),
        operation.python_fully_qualified_name(),
        description,
        supported_dtype);

    bind_registered_operation(
        module,
        operation,
        doc,
        // tensor and scalar
        ttnn::pybind_overload_t{
            [](const binary_backward_operation_t& self,
               const Tensor& grad_tensor,
               const Tensor& input_tensor_a,
               const float scalar,
               const std::optional<MemoryConfig>& memory_config){
                return self(grad_tensor, input_tensor_a, scalar, memory_config);
            },
            py::arg("grad_tensor"),
            py::arg("input_tensor_a"),
            py::arg("scalar"),
            py::kw_only(),
            py::arg("memory_config") = std::nullopt},

        // tensor and tensor
        ttnn::pybind_overload_t{
            [](const binary_backward_operation_t& self,
               const Tensor& grad_tensor,
               const Tensor& input_tensor_a,
               const Tensor& input_tensor_b,
               const std::optional<MemoryConfig>& memory_config) {
                return self(grad_tensor, input_tensor_a, input_tensor_b, memory_config);
            },
            py::arg("grad_tensor"),
            py::arg("input_tensor_a"),
            py::arg("input_tensor_b"),
            py::kw_only(),
            py::arg("memory_config") = std::nullopt},

        // complextensor
        ttnn::pybind_overload_t{
            [](const binary_backward_operation_t& self,
               const ComplexTensor& grad_tensor,
               const ComplexTensor& input_tensor_a,
               const ComplexTensor& input_tensor_b,
               float alpha,
               const std::optional<MemoryConfig>& memory_config) {
                return self(grad_tensor, input_tensor_a, input_tensor_b, alpha, memory_config);
            },
            py::arg("grad_tensor"),
            py::arg("input_tensor_a"),
            py::arg("input_tensor_b"),
            py::arg("alpha"),
            py::kw_only(),
            py::arg("memory_config") = std::nullopt});
}

}  // namespace detail


void py_module(py::module& module) {
    detail::bind_binary_bw_mul(
        module,
        ttnn::mul_bw,
        R"doc(Performs backward operations for multiply on :attr:`input_tensor`, :attr:`alpha` or attr:`input_tensor_a`, attr:`input_tensor_b`, with given :attr:`grad_tensor`.)doc",
        R"doc(
        +----------------------------+---------------------------------+-------------------+
        |     Dtypes                 |         Layouts                 |     Ranks         |
        +----------------------------+---------------------------------+-------------------+
        |    BFLOAT16, BFLOAT8_B     |       ROW_MAJOR, TILE           |      2, 3, 4      |
        +----------------------------+---------------------------------+-------------------+)doc");

    detail::bind_binary_bw_operation(
        module,
        ttnn::add_bw,
        R"doc(Performs backward operations for add of :attr:`input_tensor_a` and :attr:`input_tensor_b` or :attr:`scalar` with given :attr:`grad_tensor`.)doc",
        R"doc(
        +----------------------------+---------------------------------+-------------------+
        |     Dtypes                 |         Layouts                 |     Ranks         |
        +----------------------------+---------------------------------+-------------------+
        |    BFLOAT16, BFLOAT8_B     |       ROW_MAJOR, TILE           |      2, 3, 4      |
        +----------------------------+---------------------------------+-------------------+)doc");

    detail::bind_binary_bw_operation(
        module,
        ttnn::sub_bw,
        R"doc(Performs backward operations for sub of :attr:`input_tensor_a` and :attr:`input_tensor_b` or :attr:`scalar` with given :attr:`grad_tensor`.)doc",
        R"doc(
        +----------------------------+---------------------------------+-------------------+
        |     Dtypes                 |         Layouts                 |     Ranks         |
        +----------------------------+---------------------------------+-------------------+
        |    BFLOAT16, BFLOAT8_B     |       ROW_MAJOR, TILE           |      2, 3, 4      |
        +----------------------------+---------------------------------+-------------------+)doc");

    detail::bind_binary_bw_div(
        module,
        ttnn::div_bw,
        R"doc(Performs backward operations for divide on :attr:`input_tensor`, :attr:`alpha` or attr:`input_tensor_a`, attr:`input_tensor_b`, attr:`round_mode`,  with given :attr:`grad_tensor`.)doc",
        R"doc(
        +----------------------------+---------------------------------+-------------------+
        |     Dtypes                 |         Layouts                 |     Ranks         |
        +----------------------------+---------------------------------+-------------------+
        |    BFLOAT16, BFLOAT8_B     |       ROW_MAJOR, TILE           |      2, 3, 4      |
        +----------------------------+---------------------------------+-------------------+)doc");

    detail::bind_binary_backward_ops(
        module,
        ttnn::atan2_bw,
        R"doc(Performs backward operations for atan2 of :attr:`input_tensor_a` and :attr:`input_tensor_b` with given :attr:`grad_tensor`.)doc",
        R"doc(
        +----------------------------+---------------------------------+-------------------+
        |     Dtypes                 |         Layouts                 |     Ranks         |
        +----------------------------+---------------------------------+-------------------+
        |    BFLOAT16, BFLOAT8_B     |       ROW_MAJOR, TILE           |      2, 3, 4      |
        +----------------------------+---------------------------------+-------------------+)doc");

    detail::bind_binary_backward_ops(
        module,
        ttnn::embedding_bw,
        R"doc(Performs backward operations for embedding_bw function and it returns specific indices of the embedding table specified by the :attr:`grad_tensor`.
        The input tensor( :attr:`input_tensor_a`, :attr:`input_tensor_b`) should be unique.)doc",
        R"doc(
        +----------------------------+---------------------------------+-------------------+
        |     Dtypes                 |         Layouts                 |     Ranks         |
        +----------------------------+---------------------------------+-------------------+
        |    BFLOAT16, BFLOAT8_B     |       ROW_MAJOR, TILE           |      2, 3, 4      |
        +----------------------------+---------------------------------+-------------------+)doc");

    detail::bind_binary_backward_float_default(
        module,
        ttnn::subalpha_bw,
        "alpha", "Alpha value", 1.0f,
        R"doc(Performs backward operations for subalpha of :attr:`input_tensor_a` and :attr:`input_tensor_b` with given :attr:`grad_tensor`.)doc",
        R"doc(
        +----------------------------+---------------------------------+-------------------+
        |     Dtypes                 |         Layouts                 |     Ranks         |
        +----------------------------+---------------------------------+-------------------+
        |    BFLOAT16, BFLOAT8_B     |       ROW_MAJOR, TILE           |      2, 3, 4      |
        +----------------------------+---------------------------------+-------------------+)doc");

    detail::bind_binary_backward_opt_float_default(
        module,
        ttnn::addalpha_bw,
        "alpha", "Alpha value", 1.0f,
        R"doc(Performs backward operations for addalpha on :attr:`input_tensor_b` , attr:`input_tensor_a`, attr:`alpha` with given attr:`grad_tensor`.)doc",
        R"doc(
        +----------------------------+---------------------------------+-------------------+
        |     Dtypes                 |         Layouts                 |     Ranks         |
        +----------------------------+---------------------------------+-------------------+
        |    BFLOAT16, BFLOAT8_B     |       ROW_MAJOR, TILE           |      2, 3, 4      |
        +----------------------------+---------------------------------+-------------------+)doc");

    detail::bind_binary_backward_ops(
        module,
        ttnn::xlogy_bw,
        R"doc(Performs backward operations for xlogy of :attr:`input_tensor_a` and :attr:`input_tensor_b` with given :attr:`grad_tensor`.)doc",
        R"doc(
        +----------------------------+---------------------------------+-------------------+
        |     Dtypes                 |         Layouts                 |     Ranks         |
        +----------------------------+---------------------------------+-------------------+
        |    BFLOAT16, BFLOAT8_B     |       ROW_MAJOR, TILE           |      2, 3, 4      |
        +----------------------------+---------------------------------+-------------------+)doc");

    detail::bind_binary_backward_ops(
        module,
        ttnn::hypot_bw,
        R"doc(Performs backward operations for hypot of :attr:`input_tensor_a` and :attr:`input_tensor_b` with given :attr:`grad_tensor`.)doc",
        R"doc(
        +----------------------------+---------------------------------+-------------------+
        |     Dtypes                 |         Layouts                 |     Ranks         |
        +----------------------------+---------------------------------+-------------------+
        |    BFLOAT16, BFLOAT8_B     |       ROW_MAJOR, TILE           |      2, 3, 4      |
        +----------------------------+---------------------------------+-------------------+)doc");

    detail::bind_binary_backward_ops(
        module,
        ttnn::ldexp_bw,
        R"doc(Performs backward operations for ldexp of :attr:`input_tensor_a` and :attr:`input_tensor_b` with given :attr:`grad_tensor`.)doc",
        R"doc(
        +----------------------------+---------------------------------+-------------------+
        |     Dtypes                 |         Layouts                 |     Ranks         |
        +----------------------------+---------------------------------+-------------------+
        |    BFLOAT16                |       ROW_MAJOR                 |      2, 3, 4      |
        +----------------------------+---------------------------------+-------------------+)doc");

    detail::bind_binary_backward_ops(
        module,
        ttnn::logaddexp_bw,
        R"doc(Performs backward operations for logaddexp of :attr:`input_tensor_a` and :attr:`input_tensor_b` with given :attr:`grad_tensor`.)doc",
        R"doc(
        +----------------------------+---------------------------------+-------------------+
        |     Dtypes                 |         Layouts                 |     Ranks         |
        +----------------------------+---------------------------------+-------------------+
        |    BFLOAT16, BFLOAT8_B     |       ROW_MAJOR, TILE           |      2, 3, 4      |
        +----------------------------+---------------------------------+-------------------+)doc");

    detail::bind_binary_backward_ops(
        module,
        ttnn::logaddexp2_bw,
        R"doc(Performs backward operations for logaddexp2 of :attr:`input_tensor_a` and :attr:`input_tensor_b` with given :attr:`grad_tensor`.)doc",
        R"doc(
        +----------------------------+---------------------------------+-------------------+
        |     Dtypes                 |         Layouts                 |     Ranks         |
        +----------------------------+---------------------------------+-------------------+
        |    BFLOAT16, BFLOAT8_B     |       ROW_MAJOR, TILE           |      2, 3, 4      |
        +----------------------------+---------------------------------+-------------------+)doc");

    detail::bind_binary_backward_ops(
        module,
        ttnn::squared_difference_bw,
        R"doc(Performs backward operations for squared_difference of :attr:`input_tensor_a` and :attr:`input_tensor_b` with given :attr:`grad_tensor`.)doc",
        R"doc(
        +----------------------------+---------------------------------+-------------------+
        |     Dtypes                 |         Layouts                 |     Ranks         |
        +----------------------------+---------------------------------+-------------------+
        |    BFLOAT16, BFLOAT8_B     |       ROW_MAJOR, TILE           |      2, 3, 4      |
        +----------------------------+---------------------------------+-------------------+)doc");

    detail::bind_binary_backward_int_default(
        module,
        ttnn::concat_bw,
        "dim", "Dimension to concatenate", 0,
        R"doc(Performs backward operations for concat on :attr:`input_tensor_a` and :attr:`input_tensor_b` with given attr:`grad_tensor`.)doc",
        R"doc(
        +----------------------------+---------------------------------+-------------------+
        |     Dtypes                 |         Layouts                 |     Ranks         |
        +----------------------------+---------------------------------+-------------------+
        |    BFLOAT16, BFLOAT8_B     |       ROW_MAJOR, TILE           |      2, 3, 4      |
        +----------------------------+---------------------------------+-------------------+)doc");

    detail::bind_binary_backward_ops(
        module,
        ttnn::rsub_bw,
        R"doc(Performs backward operations for subraction of :attr:`input_tensor_a` from :attr:`input_tensor_b` with given attr:`grad_tensor` (reversed order of subtraction operator).)doc",
        R"doc(
        +----------------------------+---------------------------------+-------------------+
        |     Dtypes                 |         Layouts                 |     Ranks         |
        +----------------------------+---------------------------------+-------------------+
        |    BFLOAT16, BFLOAT8_B     |       ROW_MAJOR, TILE           |      2, 3, 4      |
        +----------------------------+---------------------------------+-------------------+)doc");

    detail::bind_binary_backward_ops(
        module,
        ttnn::min_bw,
        R"doc(Performs backward operations for minimum of :attr:`input_tensor_a` and :attr:`input_tensor_b` with given :attr:`grad_tensor`.)doc",
        R"doc(
        +----------------------------+---------------------------------+-------------------+
        |     Dtypes                 |         Layouts                 |     Ranks         |
        +----------------------------+---------------------------------+-------------------+
        |    BFLOAT16, BFLOAT8_B     |       ROW_MAJOR, TILE           |      2, 3, 4      |
        +----------------------------+---------------------------------+-------------------+)doc");

    detail::bind_binary_backward_ops(
        module,
        ttnn::max_bw,
        R"doc(Performs backward operations for maximum of :attr:`input_tensor_a` and :attr:`input_tensor_b` with given :attr:`grad_tensor`.)doc",
        R"doc(
        +----------------------------+---------------------------------+-------------------+
        |     Dtypes                 |         Layouts                 |     Ranks         |
        +----------------------------+---------------------------------+-------------------+
        |    BFLOAT16, BFLOAT8_B     |       ROW_MAJOR, TILE           |      2, 3, 4      |
        +----------------------------+---------------------------------+-------------------+)doc");

    detail::bind_binary_backward_float_string_default(
        module,
        ttnn::bias_gelu_bw,
        "bias",
        "Bias value",
        "approximate",
        "Approximation type",
        "none",
        R"doc(Performs backward operations for bias_gelu on :attr:`input_tensor_a` and :attr:`input_tensor_b` or :attr:`input_tensor` and :attr:`bias`, with given :attr:`grad_tensor` using given :attr:`approximate` mode.
        :attr:`approximate` mode can be 'none', 'tanh'.)doc",
        R"doc(
        +----------------------------+---------------------------------+-------------------+
        |     Dtypes                 |         Layouts                 |     Ranks         |
        +----------------------------+---------------------------------+-------------------+
        |    BFLOAT16, BFLOAT8_B     |       ROW_MAJOR, TILE           |      2, 3, 4      |
        +----------------------------+---------------------------------+-------------------+)doc");

    detail::bind_binary_bw_comparison(
        module,
        ttnn::le_bw,
        R"doc(Performs backward operations for less than or equal comparison on :attr:`input_tensor`, :attr:`alpha` or attr:`input_tensor_a`, attr:`input_tensor_b`, with given :attr:`grad_tensor`.
        Returns an tensor of zeros like input tensors.)doc",
        R"doc(
        +----------------------------+---------------------------------+-------------------+
        |     Dtypes                 |         Layouts                 |     Ranks         |
        +----------------------------+---------------------------------+-------------------+
        |    BFLOAT16, BFLOAT8_B     |       ROW_MAJOR, TILE           |      2, 3, 4      |
        +----------------------------+---------------------------------+-------------------+)doc");

    detail::bind_binary_bw_comparison(
        module,
        ttnn::ne_bw,
        R"doc(Performs backward operations for not equal comparison on :attr:`input_tensor`, :attr:`alpha` or attr:`input_tensor_a`, attr:`input_tensor_b`, with given :attr:`grad_tensor`.
        Returns an tensor of zeros like input tensors.)doc",
        R"doc(
        +----------------------------+---------------------------------+-------------------+
        |     Dtypes                 |         Layouts                 |     Ranks         |
        +----------------------------+---------------------------------+-------------------+
        |    BFLOAT16, BFLOAT8_B     |       ROW_MAJOR, TILE           |      2, 3, 4      |
        +----------------------------+---------------------------------+-------------------+)doc");

    detail::bind_binary_bw_comparison(
        module,
        ttnn::lt_bw,
        R"doc(Performs backward operations for less than comparison on :attr:`input_tensor`, :attr:`alpha` or attr:`input_tensor_a`, attr:`input_tensor_b`, with given :attr:`grad_tensor`.
        Returns an tensor of zeros like input tensors.)doc",
        R"doc(
        +----------------------------+---------------------------------+-------------------+
        |     Dtypes                 |         Layouts                 |     Ranks         |
        +----------------------------+---------------------------------+-------------------+
        |    BFLOAT16, BFLOAT8_B     |       ROW_MAJOR, TILE           |      2, 3, 4      |
        +----------------------------+---------------------------------+-------------------+)doc");

    detail::bind_binary_bw_comparison(
        module,
        ttnn::gt_bw,
        R"doc(Performs backward operations for greater than comparison on :attr:`input_tensor`, :attr:`alpha` or attr:`input_tensor_a`, attr:`input_tensor_b`, with given :attr:`grad_tensor`.
        Returns an tensor of zeros like input tensors.)doc",
        R"doc(
        +----------------------------+---------------------------------+-------------------+
        |     Dtypes                 |         Layouts                 |     Ranks         |
        +----------------------------+---------------------------------+-------------------+
        |    BFLOAT16, BFLOAT8_B     |       ROW_MAJOR, TILE           |      2, 3, 4      |
        +----------------------------+---------------------------------+-------------------+)doc");

    detail::bind_binary_bw_comparison(
        module,
        ttnn::ge_bw,
        R"doc(Performs backward operations for greater than or equal comparison on :attr:`input_tensor`, :attr:`alpha` or attr:`input_tensor_a`, attr:`input_tensor_b`, with given :attr:`grad_tensor`.
        Returns an tensor of zeros like input tensors.)doc",
        R"doc(
        +----------------------------+---------------------------------+-------------------+
        |     Dtypes                 |         Layouts                 |     Ranks         |
        +----------------------------+---------------------------------+-------------------+
        |    BFLOAT16, BFLOAT8_B     |       ROW_MAJOR, TILE           |      2, 3, 4      |
        +----------------------------+---------------------------------+-------------------+)doc");

    detail::bind_binary_bw_comparison(
        module,
        ttnn::eq_bw,
        R"doc(Performs backward operations for equal comparison on :attr:`input_tensor`, :attr:`alpha` or attr:`input_tensor_a`, attr:`input_tensor_b`, with given :attr:`grad_tensor`.
        Returns an tensor of zeros like input tensors.)doc",
        R"doc(
        +----------------------------+---------------------------------+-------------------+
        |     Dtypes                 |         Layouts                 |     Ranks         |
        +----------------------------+---------------------------------+-------------------+
        |    BFLOAT16, BFLOAT8_B     |       ROW_MAJOR, TILE           |      2, 3, 4      |
        +----------------------------+---------------------------------+-------------------+)doc");

}

}  // namespace binary_backward
}  // namespace operations
}  // namespace ttnn
