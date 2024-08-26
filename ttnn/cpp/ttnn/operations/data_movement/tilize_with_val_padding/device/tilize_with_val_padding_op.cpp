// SPDX-FileCopyrightText: © 2024 Tenstorrent Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include "tilize_with_val_padding_op.hpp"

#include "ttnn/tensor/tensor_utils.hpp"
#include "tilize_with_val_padding_program_factory.hpp"
#include "ttnn/run_operation.hpp"

namespace ttnn::operations::data_movement {

void TilizeWithValPadding::validate(const std::vector<Tensor>& input_tensors) const {
    const auto& input_tensor_a = input_tensors.at(0);
    TT_FATAL(input_tensor_a.storage_type() == StorageType::DEVICE, "Operands need to be on device!");
    TT_FATAL(input_tensor_a.buffer() != nullptr, "Operands need to be allocated in buffers on device!");
    TT_FATAL(input_tensor_a.get_layout() == Layout::ROW_MAJOR, "Can only tilize row major data");
    TT_FATAL(input_tensor_a.get_dtype() == DataType::BFLOAT16);

    TT_FATAL(input_tensor_a.get_legacy_shape()[0] <= this->output_tensor_shape[0]);
    TT_FATAL(input_tensor_a.get_legacy_shape()[1] <= this->output_tensor_shape[1]);
    TT_FATAL(input_tensor_a.get_legacy_shape()[2] <= this->output_tensor_shape[2]);
    TT_FATAL(input_tensor_a.get_legacy_shape()[3] <= this->output_tensor_shape[3]);

    uint32_t num_rows = this->output_tensor_shape[2];
    uint32_t inner_dim = this->output_tensor_shape[3];
    TT_FATAL(num_rows % TILE_HEIGHT == 0, "Output shape must be tilizable");
    TT_FATAL(inner_dim % TILE_WIDTH == 0, "Output shape must be tilizable");

    if (input_tensor_a.memory_config().is_sharded()) {
        TT_FATAL(input_tensor_a.memory_config().memory_layout == TensorMemoryLayout::WIDTH_SHARDED);
        TT_FATAL(this->output_mem_config.memory_layout == input_tensor_a.memory_config().memory_layout);
        for (uint32_t i = 0; i < input_tensor_a.get_legacy_shape().rank(); i++) {
            if (i != input_tensor_a.get_legacy_shape().rank() - 2) {
                TT_FATAL(input_tensor_a.get_legacy_shape()[i] == this->output_tensor_shape[i]);
            }
        }
    }
}

std::vector<tt::tt_metal::Shape> TilizeWithValPadding::compute_output_shapes(
    const std::vector<Tensor>& input_tensors) const {
    auto input_shape = input_tensors.at(0).get_legacy_shape();
    auto dimensions_pads = std::vector<Padding::PadDimension>();
    for (auto index = 0; index < input_shape.rank(); index++) {
        auto back = this->output_tensor_shape[index] - input_shape[index];
        dimensions_pads.push_back(Padding::PadDimension{.front = 0, .back = back});
    }
    const auto padding = Padding(dimensions_pads, Padding::PadValue::Any);
    return {tt::tt_metal::Shape(this->output_tensor_shape, padding)};
}

std::vector<Tensor> TilizeWithValPadding::create_output_tensors(
    const std::vector<Tensor>& input_tensors, const std::vector<std::optional<Tensor>>& output_tensors) const {
    const auto& input_tensor_a = input_tensors.at(0);
    if (input_tensor_a.memory_config().is_sharded()) {
        auto output_shape = this->compute_output_shapes(input_tensors).at(0);
        auto shard_spec = input_tensor_a.shard_spec().value();
        shard_spec.shape[0] = tt::tt_metal::compute_volume(output_shape) / output_shape[-1];
        auto mem_config = this->output_mem_config;
        mem_config.shard_spec = shard_spec;
        return {
            create_device_tensor(output_shape, this->output_dtype, Layout::TILE, input_tensor_a.device(), mem_config)};
    } else {
        return operation::generic_create_output_tensors(
            *this, input_tensors, this->output_dtype, Layout::TILE, this->output_mem_config);
    }
}

// TODO: If pad is called on a tile and output is not tile, we could untilize then pad, and output is RM
// Currently calling pad on a tile requires the output pad shape to be tile
operation::ProgramWithCallbacks TilizeWithValPadding::create_program(
    const std::vector<Tensor>& input_tensors, std::vector<Tensor>& output_tensors) const {
    const auto& input_tensor_a = input_tensors.at(0);
    auto& output_tensor = output_tensors.at(0);
    if (input_tensor_a.memory_config().is_sharded() || this->use_multicore) {
        return detail::tilize_with_val_padding_multi_core(input_tensor_a, output_tensor, this->pad_value);
    }
    return detail::tilize_with_val_padding_single_core(input_tensor_a, output_tensor, this->pad_value);
}

}  // namespace ttnn::operations::data_movement
