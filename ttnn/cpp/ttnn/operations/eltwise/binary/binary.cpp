
// SPDX-FileCopyrightText: © 2023 Tenstorrent Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include "binary.hpp"
#include "device/binary_device_operation.hpp"
#include "ttnn/device_operation.hpp"
#include "ttnn/operations/data_movement/repeat/repeat.hpp"
#include "ttnn/operations/eltwise/unary/unary.hpp"

namespace ttnn::operations::binary {

namespace detail {

constexpr bool is_associative(BinaryOpType op) {
    return op == BinaryOpType::ADD || op == BinaryOpType::MUL || op == BinaryOpType::EQ || op == BinaryOpType::NE ||
           op == BinaryOpType::LOGICAL_AND || op == BinaryOpType::LOGICAL_OR || op == BinaryOpType::LOGADDEXP ||
           op == BinaryOpType::LOGADDEXP2;
}

// Tensor - Scalar
inline Tensor binary_impl(
    uint8_t queue_id,
    BinaryOpType binary_op_type,
    const ttnn::Tensor &input_tensor,
    const float scalar,
    const std::optional<ttnn::MemoryConfig> &memory_config = std::nullopt,
    const std::optional<Tensor> &optional_output_tensor = std::nullopt) {
    auto output_memory_config = optional_output_tensor.has_value()
                                    ? optional_output_tensor.value().memory_config()
                                    : memory_config.value_or(input_tensor.memory_config());
    auto output_tensor = input_tensor;
    if (binary_op_type == BinaryOpType::GT) {
        output_tensor = ttnn::gt_unary(queue_id, input_tensor, scalar, output_memory_config, optional_output_tensor);
    } else if (binary_op_type == BinaryOpType::LT) {
        output_tensor = ttnn::lt_unary(queue_id, input_tensor, scalar, output_memory_config, optional_output_tensor);
    } else if (binary_op_type == BinaryOpType::NE) {
        output_tensor = ttnn::ne_unary(queue_id, input_tensor, scalar, output_memory_config, optional_output_tensor);
    } else if (binary_op_type == BinaryOpType::GTE) {
        output_tensor = ttnn::gez(
            queue_id,
            ttnn::sub_sfpu(queue_id, input_tensor, scalar, output_memory_config),
            output_memory_config,
            optional_output_tensor);
    } else if (binary_op_type == BinaryOpType::LTE) {
        output_tensor = ttnn::lez(
            queue_id,
            ttnn::sub_sfpu(queue_id, input_tensor, scalar, output_memory_config),
            output_memory_config,
            optional_output_tensor);
    } else if (binary_op_type == BinaryOpType::EQ) {
        output_tensor = ttnn::eqz(
            queue_id,
            ttnn::sub_sfpu(queue_id, input_tensor, scalar, output_memory_config),
            output_memory_config,
            optional_output_tensor);
    } else {
        TT_THROW("Unsupported operation");
    }
    return output_tensor;
}

// Scalar - Tensor
inline Tensor binary_impl(
    uint8_t queue_id,
    BinaryOpType binary_op_type,
    const float scalar,
    const ttnn::Tensor &input_tensor,
    const std::optional<ttnn::MemoryConfig> &memory_config = std::nullopt,
    const std::optional<Tensor> &optional_output_tensor = std::nullopt) {
    auto output_memory_config = optional_output_tensor.has_value()
                                    ? optional_output_tensor.value().memory_config()
                                    : memory_config.value_or(input_tensor.memory_config());
    auto output_tensor = input_tensor;
    if (binary_op_type == BinaryOpType::GTE) {
        output_tensor = ttnn::gez(
            queue_id,
            ttnn::sub_sfpu(queue_id, scalar, input_tensor, output_memory_config),
            output_memory_config,
            optional_output_tensor);
    } else if (binary_op_type == BinaryOpType::LTE) {
        output_tensor = ttnn::lez(
            queue_id,
            ttnn::sub_sfpu(queue_id, scalar, input_tensor, output_memory_config),
            output_memory_config,
            optional_output_tensor);
    } else if (binary_op_type == BinaryOpType::EQ) {
        output_tensor = ttnn::eqz(
            queue_id,
            ttnn::sub_sfpu(queue_id, scalar, input_tensor, output_memory_config),
            output_memory_config,
            optional_output_tensor);
    } else {
        TT_THROW("Unsupported operation");
    }
    return output_tensor;
}
}  // namespace detail

template <BinaryOpType binary_op_type, bool in_place>
Tensor BinaryOperation<binary_op_type, in_place>::operator()(
    uint8_t queue_id,
    const Tensor &input_tensor_a_arg,
    const Tensor &input_tensor_b_arg,
    const std::optional<const DataType> &output_dtype,
    const std::optional<MemoryConfig> &memory_config,
    std::optional<Tensor> optional_output_tensor,
    std::optional<unary::FusedActivations> activations,
    std::optional<unary::UnaryWithParam> input_tensor_a_activation) {
    if(output_dtype.has_value() && optional_output_tensor.has_value()){
        TT_FATAL(output_dtype.value() == optional_output_tensor.value().get_dtype(), "If both output dtype and output tensor provided dtype should match");
    }

    auto &&[input_tensor_a, input_tensor_b] = [](const auto &input_tensor_a_arg, const auto &input_tensor_b_arg) {
        if (detail::is_associative(binary_op_type)) {
            const auto input_shape_a = input_tensor_a_arg.get_shape();
            const auto input_shape_b = input_tensor_b_arg.get_shape();
            // Swap tensors if input_tensor_a needs to be broadcasted to input_tensor_b
            if (tt::tt_metal::compute_volume(input_shape_a) < tt::tt_metal::compute_volume(input_shape_b)) {
                return std::make_tuple(input_tensor_b_arg, input_tensor_a_arg);
            }
        }
        return std::make_tuple(input_tensor_a_arg, input_tensor_b_arg);
    }(input_tensor_a_arg, input_tensor_b_arg);

    auto output_memory_config = memory_config.value_or(input_tensor_a.memory_config());

    // TODO(arakhmati): #7731 - remove this!
    auto repeat_smaller = [&output_memory_config](const auto& first, auto& second){
        const auto first_shape = first.get_shape();
        const auto second_shape = second.get_shape();

        // repeats second if it is smaller
        if (first_shape.rank() == 4 and second_shape.rank() == 4 and
            first_shape[0] > second_shape[0] and
            first_shape[-1] == second_shape[-1] and
            first_shape[-2] == second_shape[-2] and
            first_shape[-3] == second_shape[-3]) {

            tt::log_warning(tt::LogOp, "Using repeat op to broadcast batch dim");
            Shape repeats(std::array<uint32_t, 4>{first_shape[0], 1, 1, 1});
            second = ttnn::repeat(second, repeats, output_memory_config);
        }
    };
    repeat_smaller(input_tensor_a, input_tensor_b);
    repeat_smaller(input_tensor_b, input_tensor_a);

    DataType dtype = output_dtype.value_or(input_tensor_a.get_dtype());
    if(optional_output_tensor.has_value()) {
        dtype = optional_output_tensor.value().get_dtype();
    }

    return ttnn::device_operation::run<BinaryDeviceOperation>(
        queue_id,
        BinaryDeviceOperation::operation_attributes_t{
            binary_op_type, in_place, activations, input_tensor_a_activation, output_memory_config, dtype, std::nullopt},
        BinaryDeviceOperation::tensor_args_t{input_tensor_a, input_tensor_b, optional_output_tensor});
}

template <BinaryOpType binary_op_type, bool in_place>
Tensor BinaryOperation<binary_op_type, in_place>::operator()(
    const Tensor &input_tensor_a_arg,
    const Tensor &input_tensor_b_arg,
    const std::optional<const DataType> &output_dtype,
    const std::optional<MemoryConfig> &memory_config,
    std::optional<Tensor> optional_output_tensor,
    std::optional<unary::FusedActivations> activations,
    std::optional<unary::UnaryWithParam> input_tensor_a_activation) {
    return operator()(
        DefaultQueueId,
        input_tensor_a_arg,
        input_tensor_b_arg,
        output_dtype,
        memory_config,
        optional_output_tensor,
        activations,
        input_tensor_a_activation);
}

// TODO: this case should use BinaryWithScalarProgramConfig and there should be a custom kernel to run this
// Currently, this is exactly how tt::tt_metal::add_unary works
template <BinaryOpType binary_op_type, bool in_place>
Tensor BinaryOperation<binary_op_type, in_place>::operator()(
    const ttnn::Tensor &input_tensor_a,
    const float scalar,
    const std::optional<const DataType> &dtype,
    const std::optional<ttnn::MemoryConfig> &memory_config,
    const std::optional<Tensor> &optional_output_tensor,
    std::optional<unary::FusedActivations> activations,
    std::optional<unary::UnaryWithParam> input_tensor_a_activation) {
    return BinaryOperation::operator()(
        DefaultQueueId, input_tensor_a, scalar, dtype, memory_config, optional_output_tensor, activations, input_tensor_a_activation);
}

template <BinaryOpType binary_op_type, bool in_place>
Tensor BinaryOperation<binary_op_type, in_place>::operator()(
    uint8_t queue_id,
    const ttnn::Tensor &input_tensor_a,
    const float scalar,
    const std::optional<const DataType> &dtype,
    const std::optional<ttnn::MemoryConfig> &memory_config,
    const std::optional<Tensor> &optional_output_tensor,
    std::optional<unary::FusedActivations> activations,
    std::optional<unary::UnaryWithParam> input_tensor_a_activation) {
    // Cast Float Scalar to a device tensor
    auto host_buffer = owned_buffer::create<::bfloat16>(static_cast<std::size_t>(TILE_HEIGHT * TILE_WIDTH));
    host_buffer[0] = scalar;
    Tensor scalar_tensor_host = Tensor(
        OwnedStorage{host_buffer},
        ttnn::Shape(std::array<std::uint32_t, 2>{1, 1}, std::array<std::uint32_t, 2>{TILE_HEIGHT, TILE_WIDTH}),
        DataType::BFLOAT16,
        Layout::TILE);
    Tensor scalar_tensor_device = scalar_tensor_host.to(input_tensor_a.device());
    // TODO(arakhmati): #7637 pass in memory_config instead of operation::DEFAULT_OUTPUT_MEMORY_CONFIG
    return BinaryOperation::operator()(
        input_tensor_a, scalar_tensor_device, dtype, memory_config, optional_output_tensor, activations, input_tensor_a_activation);
}


template <BinaryOpType binary_op_type, bool in_place>
Tensor RelationalBinary<binary_op_type, in_place>::operator()(
    uint8_t queue_id,
    const Tensor &input_tensor_a_arg,
    const Tensor &input_tensor_b_arg,
    const std::optional<const DataType> &output_dtype,
    const std::optional<MemoryConfig> &memory_config,
    std::optional<Tensor> optional_output_tensor,
    std::optional<unary::FusedActivations> activations,
    std::optional<unary::UnaryWithParam> input_tensor_a_activation) {
    if (output_dtype.has_value() && optional_output_tensor.has_value()) {
        TT_FATAL(
            output_dtype.value() == optional_output_tensor.value().get_dtype(),
            "If both output dtype and output tensor provided dtype should match");
    }

    auto &&[input_tensor_a, input_tensor_b] = [](const auto &input_tensor_a_arg, const auto &input_tensor_b_arg) {
        if (detail::is_associative(binary_op_type)) {
            const auto input_shape_a = input_tensor_a_arg.get_shape();
            const auto input_shape_b = input_tensor_b_arg.get_shape();
            // Swap tensors if input_tensor_a needs to be broadcasted to input_tensor_b
            if (tt::tt_metal::compute_volume(input_shape_a) < tt::tt_metal::compute_volume(input_shape_b)) {
                return std::make_tuple(input_tensor_b_arg, input_tensor_a_arg);
            }
        }
        return std::make_tuple(input_tensor_a_arg, input_tensor_b_arg);
    }(input_tensor_a_arg, input_tensor_b_arg);

    auto output_memory_config = memory_config.value_or(input_tensor_a.memory_config());

    // TODO(arakhmati): #7731 - remove this!
    auto repeat_smaller = [&output_memory_config](const auto &first, auto &second) {
        const auto first_shape = first.get_shape();
        const auto second_shape = second.get_shape();

        // repeats second if it is smaller
        if (first_shape.rank() == 4 and second_shape.rank() == 4 and first_shape[0] > second_shape[0] and
            first_shape[-1] == second_shape[-1] and first_shape[-2] == second_shape[-2] and
            first_shape[-3] == second_shape[-3]) {
            tt::log_warning(tt::LogOp, "Using repeat op to broadcast batch dim");
            Shape repeats(std::array<uint32_t, 4>{first_shape[0], 1, 1, 1});
            second = ttnn::repeat(second, repeats, output_memory_config);
        }
    };
    repeat_smaller(input_tensor_a, input_tensor_b);
    repeat_smaller(input_tensor_b, input_tensor_a);

    DataType dtype = output_dtype.value_or(input_tensor_a.get_dtype());
    if (optional_output_tensor.has_value()) {
        dtype = optional_output_tensor.value().get_dtype();
    }

    return ttnn::device_operation::run<BinaryDeviceOperation>(
        queue_id,
        BinaryDeviceOperation::operation_attributes_t{
            binary_op_type, in_place, activations, input_tensor_a_activation, output_memory_config, dtype, std::nullopt},
        BinaryDeviceOperation::tensor_args_t{input_tensor_a, input_tensor_b, optional_output_tensor});
}

template <BinaryOpType binary_op_type, bool in_place>
Tensor RelationalBinary<binary_op_type, in_place>::operator()(
    const Tensor &input_tensor_a_arg,
    const Tensor &input_tensor_b_arg,
    const std::optional<const DataType> &output_dtype,
    const std::optional<MemoryConfig> &memory_config,
    std::optional<Tensor> optional_output_tensor,
    std::optional<unary::FusedActivations> activations,
    std::optional<unary::UnaryWithParam> input_tensor_a_activation) {
    return operator()(
        DefaultQueueId,
        input_tensor_a_arg,
        input_tensor_b_arg,
        output_dtype,
        memory_config,
        optional_output_tensor,
        activations,
        input_tensor_a_activation);
}

template <BinaryOpType binary_op_type, bool in_place>
Tensor RelationalBinary<binary_op_type, in_place>::operator()(
    const ttnn::Tensor &input_tensor_a,
    const float scalar,
    const std::optional<const DataType> &dtype,
    const std::optional<ttnn::MemoryConfig> &memory_config,
    const std::optional<Tensor> &optional_output_tensor,
    std::optional<unary::FusedActivations> activations,
    std::optional<unary::UnaryWithParam> input_tensor_a_activation) {
    return detail::binary_impl(
        DefaultQueueId, binary_op_type, input_tensor_a, scalar, memory_config, optional_output_tensor);
}

template <BinaryOpType binary_op_type, bool in_place>
Tensor RelationalBinary<binary_op_type, in_place>::operator()(
    uint8_t queue_id,
    const ttnn::Tensor &input_tensor_a,
    const float scalar,
    const std::optional<const DataType> &dtype,
    const std::optional<ttnn::MemoryConfig> &memory_config,
    const std::optional<Tensor> &optional_output_tensor,
    std::optional<unary::FusedActivations> activations,
    std::optional<unary::UnaryWithParam> input_tensor_a_activation) {
    return detail::binary_impl(
        DefaultQueueId, binary_op_type, input_tensor_a, scalar, memory_config, optional_output_tensor);
}
// scalar - tensor combination not available on Pytorch for this op
template <BinaryOpType binary_op_type, bool in_place>
Tensor RelationalBinary<binary_op_type, in_place>::operator()(
    uint8_t queue_id,
    const float scalar,
    const ttnn::Tensor &input_tensor_a,
    const std::optional<const DataType> &dtype,
    const std::optional<ttnn::MemoryConfig> &memory_config,
    const std::optional<Tensor> &optional_output_tensor) {
    return detail::binary_impl(
        DefaultQueueId, binary_op_type, scalar, input_tensor_a, memory_config, optional_output_tensor);
}

template struct BinaryOperation<BinaryOpType::ADD, false>;
template struct BinaryOperation<BinaryOpType::ADD, true>;
template struct BinaryOperation<BinaryOpType::SUB, false>;
template struct BinaryOperation<BinaryOpType::SUB, true>;
template struct BinaryOperation<BinaryOpType::MUL, false>;
template struct BinaryOperation<BinaryOpType::MUL, true>;
template struct BinaryOperation<BinaryOpType::LOGICAL_AND, false>;
template struct BinaryOperation<BinaryOpType::LOGICAL_OR, false>;
template struct BinaryOperation<BinaryOpType::LDEXP, false>;
template struct BinaryOperation<BinaryOpType::LOGADDEXP, false>;
template struct BinaryOperation<BinaryOpType::LOGADDEXP2, false>;
template struct BinaryOperation<BinaryOpType::SQUARED_DIFFERENCE, false>;
template struct BinaryOperation<BinaryOpType::DIV_FAST, false>;
template struct BinaryOperation<BinaryOpType::BIAS_GELU, false>;

template struct RelationalBinary<BinaryOpType::EQ, false>;
template struct RelationalBinary<BinaryOpType::NE, false>;
template struct RelationalBinary<BinaryOpType::GTE, false>;
template struct RelationalBinary<BinaryOpType::GT, false>;
template struct RelationalBinary<BinaryOpType::LTE, false>;
template struct RelationalBinary<BinaryOpType::LT, false>;

}  // namespace ttnn::operations::binary
