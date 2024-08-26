// SPDX-FileCopyrightText: © 2023 Tenstorrent Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include "ttnn/deprecated/tt_dnn/op_library/move/move_op.hpp"

namespace move_op_utils {
using namespace tt::tt_metal;

// When a single device tensor is created from a multi-device tensor,
// there will be 2 references instead of just 1
bool can_deallocate(const Tensor &input_tensor, bool from_multi_device) {
    return std::visit(
        [&input_tensor, &from_multi_device](auto &&storage) {
            using T = std::decay_t<decltype(storage)>;
            if constexpr (std::is_same_v<T, DeviceStorage>) {
                return storage.buffer.use_count() == (from_multi_device ? 2 : 1);
            } else if constexpr (std::is_same_v<T, MultiDeviceStorage>) {
                bool can_dealloc = true;
                auto input_tensors = get_tensors_from_multi_device_storage(input_tensor);
                for (const auto& device_tensor : input_tensors) {
                    can_dealloc &= can_deallocate(device_tensor, true);
                }
                return can_dealloc;
            } else {
                return false;
            }
        },
        input_tensor.get_storage());
}

} // namespace move_op_utils

namespace tt {

namespace tt_metal {

void Move::validate(const std::vector<Tensor> &input_tensors) const {
    const auto& input_tensor_a = input_tensors.at(0);
}

std::vector<Shape> Move::compute_output_shapes(const std::vector<Tensor> &input_tensors) const {
    const auto& input_tensor = input_tensors.at(0);
    return {input_tensor.get_legacy_shape()};
}

std::vector<Tensor> Move::create_output_tensors(const std::vector<Tensor> &input_tensors) const {
    return {input_tensors.at(1)};
}

operation::ProgramWithCallbacks Move::create_program(const std::vector<Tensor>& input_tensors, std::vector<Tensor> &output_tensors) const {
    const auto& input_tensor = input_tensors.at(0);
    auto& output_tensor = output_tensors.at(0);

    auto parallelization_strategy = this->move_op_parallelization_strategy;
    switch (parallelization_strategy){
        case MoveOpParallelizationStrategy::MULTI_CORE_OVERLAP:
            return move_multi_core_with_overlap(input_tensor, output_tensor);
        case MoveOpParallelizationStrategy::MULTI_CORE_SHARDED:
            return move_multi_core_sharded(input_tensor, output_tensor);
        case MoveOpParallelizationStrategy::MULTI_CORE:
        default:
            return move_multi_core(input_tensor, output_tensor);
    }
}

MoveOpParallelizationStrategy Move::get_parallelization_strategy(const std::vector<Tensor> &input_tensors) const {
    return this->move_op_parallelization_strategy;
}

}  // namespace tt_metal

}  // namespace tt
