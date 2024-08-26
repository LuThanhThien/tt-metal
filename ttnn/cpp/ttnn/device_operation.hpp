// SPDX-FileCopyrightText: © 2023 Tenstorrent Inc.
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <concepts>
#include <optional>
#include "ttnn/tensor/tensor.hpp"

#include "third_party/magic_enum/magic_enum.hpp"
#include "ttnn/operation_history.hpp"
#include "tt_metal/third_party/tracy/public/tracy/Tracy.hpp"
#include "tt_metal/tools/profiler/op_profiler.hpp"
#include "tt_stl/concepts.hpp"
#include "tt_stl/reflection.hpp"
#include "tt_stl/unique_any.hpp"

namespace ttnn {

namespace device_operation {

template <typename shared_variables_t>
struct CachedProgram {
    tt::tt_metal::Program program;
    // Cached program needs to share shared_variables between create and override_runtime_arguments functions
    shared_variables_t shared_variables;

    CachedProgram(tt::tt_metal::Program&& program, shared_variables_t&& shared_variables) :
        program{std::move(program)}, shared_variables{shared_variables} {}
};

struct CachedProgramFactory {
    static constexpr auto MAX_SIZE = 896;
    static constexpr auto ALIGNMENT = 32;

    tt::stl::unique_any<MAX_SIZE, ALIGNMENT> cached_program;
    // program_factory_index is used to map a runtime value to a program factory type that is being used
    std::size_t program_factory_index;

    template <typename shared_variables_t>
    CachedProgramFactory(CachedProgram<shared_variables_t>&& cached_program, std::size_t program_factory_index) :
        cached_program{std::move(cached_program)}, program_factory_index{program_factory_index} {}
};

template <typename program_factory_t>
concept ProgramFactoryConcept = requires {
    [](const auto& operation_attributes, const auto& tensor_args, auto& tensor_return_value) {
        auto cached_program = program_factory_t::create(operation_attributes, tensor_args, tensor_return_value);
        program_factory_t::override_runtime_arguments(
            cached_program, operation_attributes, tensor_args, tensor_return_value);
    };
};

template <typename device_operation_t>
concept DeviceOperationConcept = requires {
    [](const typename device_operation_t::operation_attributes_t& operation_attributes,
       const typename device_operation_t::tensor_args_t& tensor_args) {
        device_operation_t::validate_on_program_cache_hit(operation_attributes, tensor_args);
        device_operation_t::validate_on_program_cache_miss(operation_attributes, tensor_args);

        using shape_return_value_t = typename device_operation_t::shape_return_value_t;
        static_assert(std::same_as<
                      decltype(device_operation_t::compute_output_shapes(operation_attributes, tensor_args)),
                      shape_return_value_t>);

        using tensor_return_value_t = typename device_operation_t::tensor_return_value_t;
        static_assert(std::same_as<
                      decltype(device_operation_t::create_output_tensors(operation_attributes, tensor_args)),
                      tensor_return_value_t>);

        const auto program_factory = device_operation_t::select_program_factory(operation_attributes, tensor_args);
        std::visit(
            [](auto&& program_factory) {
                using program_factory_t = std::decay_t<decltype(program_factory)>;
                static_assert(ProgramFactoryConcept<program_factory_t>);
            },
            program_factory);
    };
};

template <typename device_operation_t>
concept DeviceOperationWithCustomProgramCacheConcept =
    DeviceOperationConcept<device_operation_t> &&
    requires(
        const typename device_operation_t::operation_attributes_t& operation_attributes,
        const typename device_operation_t::tensor_args_t& tensor_args) {
        { device_operation_t::compute_program_hash(operation_attributes, tensor_args)} -> std::convertible_to<tt::stl::hash::hash_t>;
    };

namespace detail {
template <typename... Ts>
[[nodiscard]] std::variant<Ts...> map_index_to_variant(std::size_t i, std::variant<Ts...>) {
    assert(i < sizeof...(Ts));
    static constexpr std::variant<Ts...> table[] = {Ts{}...};
    return table[i];
}

inline const auto USE_FAST_DISPATCH = std::getenv("TT_METAL_SLOW_DISPATCH_MODE") == nullptr;

template <typename device_operation_t>
inline auto compute_program_hash(
    const typename device_operation_t::operation_attributes_t& operation_attributes,
    const typename device_operation_t::tensor_args_t& tensor_args) {
    if constexpr (DeviceOperationWithCustomProgramCacheConcept<device_operation_t>) {
        ZoneScopedN("Compute custom program hash");
        return device_operation_t::compute_program_hash(operation_attributes, tensor_args);
    } else {
        ZoneScopedN("Compute default program hash");
        return tt::stl::hash::hash_objects_with_default_seed(
            tt::stl::hash::type_hash<device_operation_t>, operation_attributes, tensor_args);
    }
}

template <typename device_operation_t>
inline auto& create_or_get_program_from_cache(
    auto& program_cache,
    auto program_cache_hit,
    auto program_hash,
    const typename device_operation_t::operation_attributes_t& operation_attributes,
    const typename device_operation_t::tensor_args_t& tensor_args,
    typename device_operation_t::tensor_return_value_t& tensor_return_value) {
    if (not program_cache_hit) {
        ZoneScopedN("Program Cache Miss");
        auto program_factory = device_operation_t::select_program_factory(operation_attributes, tensor_args);

        auto& program = std::visit(
            [&program_cache,
             &program_hash,
             &operation_attributes,
             &tensor_args,
             &tensor_return_value,
             program_factory_index = program_factory.index()](auto&& program_factory) -> auto& {
                using program_factory_t = std::decay_t<decltype(program_factory)>;
                using cached_program_t =
                    decltype(program_factory_t::create(operation_attributes, tensor_args, tensor_return_value));
                program_cache.insert(
                    program_hash,
                    CachedProgramFactory{
                        program_factory_t::create(operation_attributes, tensor_args, tensor_return_value),
                        program_factory_index});
                auto& cached_program_factory = program_cache.template get<CachedProgramFactory>(program_hash);
                auto& cached_program = cached_program_factory.cached_program.template get<cached_program_t>();
                return cached_program.program;
            },
            program_factory);
        return program;
    } else {
        ZoneScopedN("Program Cache Hit");
        auto& cached_program_factory = program_cache.template get<CachedProgramFactory>(program_hash);
        auto program_factory_index = cached_program_factory.program_factory_index;

        using program_factory_variant_t =
            decltype(device_operation_t::select_program_factory(operation_attributes, tensor_args));
        auto program_factory = map_index_to_variant(program_factory_index, program_factory_variant_t{});

        auto& program = std::visit(
            [&cached_program_factory, &operation_attributes, &tensor_args, &tensor_return_value](
                auto&& program_factory) -> auto& {
                using program_factory_t = std::decay_t<decltype(program_factory)>;

                using cached_program_t =
                    decltype(program_factory_t::create(operation_attributes, tensor_args, tensor_return_value));
                auto& cached_program = cached_program_factory.cached_program.template get<cached_program_t>();

                program_factory_t::override_runtime_arguments(
                    cached_program, operation_attributes, tensor_args, tensor_return_value);

                return cached_program.program;
            },
            program_factory);
        return program;
    }
}

struct CheckDeviceBufferIsAllocated {
    std::size_t index = 0;

    void operator()(const Tensor& tensor) {
        if (not tensor.is_allocated()) {
            tt::log_warning(tt::LogOp, "Tensor at index {} is not allocated", index);
        }
        index++;
    }
};


#ifdef DEBUG

constexpr auto OPERATION_TYPE = "DeviceOperation";

template <typename device_operation_t>
static void append_operation_to_operation_history(
    const std::size_t ttnn_operation_id,
    const typename device_operation_t::operation_attributes_t& operation_attributes,
    const typename device_operation_t::tensor_args_t& input_tensors,
    tt::stl::hash::hash_t program_hash,
    bool program_cache_hit) {
    std::vector<operation_history::TensorRecord> input_tensor_records;
    tt::stl::reflection::visit_object_of_type<Tensor>(
        [&input_tensor_records](auto&& tensor) {
            input_tensor_records.push_back(operation_history::create_tensor_record(tensor));
        },
        input_tensors);

    operation_history::append(operation_history::OperationRecord{
        ttnn_operation_id,
        std::string(OPERATION_TYPE),
        std::string{tt::stl::get_type_name<device_operation_t>()},
        tt::stl::reflection::get_attributes(operation_attributes),
        input_tensor_records,
        program_cache_hit,
        program_hash,
    });
}

template <typename device_operation_t>
inline void log_operation(
    const typename device_operation_t::operation_attributes_t& operation_attributes,
    const typename device_operation_t::tensor_args_t& tensor_args,
    tt::stl::hash::hash_t program_hash,
    bool program_cache_hit) {
    tt::log_debug(
        tt::LogOp, "Launching Operation: \"{}\" ({})", tt::stl::get_type_name<device_operation_t>(), OPERATION_TYPE);
    tt::log_debug(tt::LogOp, "Program Hash: {}", program_hash);
    tt::log_debug(tt::LogOp, "Program Cache Hit: {}", program_cache_hit);

    tt::log_debug(tt::LogOp, "Attributes:");
    for (const auto& [key, value] : tt::stl::reflection::get_attributes(operation_attributes)) {
        tt::log_debug(tt::LogOp, "\t{} = {}", key, value);
    }

    tt::log_debug(tt::LogOp, "Tensors Args:");
    auto index = 0;
    tt::stl::reflection::visit_object_of_type<Tensor>(
        [&index](auto&& tensor) {
            tt::log_debug(tt::LogOp, "\t{}: {}", index, tensor);
            index++;
        },
        tensor_args);

    tt::log_debug(tt::LogOp, "");

    if (tt::tt_metal::operation_history::enabled()) {
        append_operation_to_operation_history<device_operation_t>(
            ttnn::OPERATION_ID, operation_attributes, tensor_args, program_hash, program_cache_hit);
    }
}
#else

template <typename device_operation_t>
inline void log_operation(
    const typename device_operation_t::operation_attributes_t& operation_attributes,
    const typename device_operation_t::tensor_args_t& tensor_args,
    tt::stl::hash::hash_t program_hash,
    bool program_cache_hit) {}
#endif



template <DeviceOperationConcept device_operation_t>
void launch_on_worker_thread(auto cq_id, auto operation_id, const auto& operation_attributes, const auto& tensor_args, auto &tensor_return_value, auto& device) {


    ZoneScopedN("TT_DNN_DEVICE_OP");

    auto& program_cache = device->program_cache;

    if (program_cache.is_enabled()) {

        auto program_hash = compute_program_hash<device_operation_t>(operation_attributes, tensor_args);
        auto program_cache_hit = program_cache.contains(program_hash);

        log_operation<device_operation_t>(operation_attributes, tensor_args, program_hash, program_cache_hit);

        tt::stl::reflection::visit_object_of_type<Tensor>(CheckDeviceBufferIsAllocated{}, tensor_args);

        if (program_cache_hit) {
            ZoneScopedN("Validate on Program Cache Hit");
            device_operation_t::validate_on_program_cache_hit(operation_attributes, tensor_args);
        } else {
            ZoneScopedN("Validate on Program Cache Miss");
            device_operation_t::validate_on_program_cache_miss(operation_attributes, tensor_args);
        }

        auto& program = create_or_get_program_from_cache<device_operation_t>(
            program_cache, program_cache_hit, program_hash, operation_attributes, tensor_args, tensor_return_value);

        if (USE_FAST_DISPATCH) {
            ZoneScopedN("EnqueueProgram");
            auto& queue = device->command_queue(cq_id);
            tt::tt_metal::EnqueueProgram(queue, program, false);
        } else {
            ZoneScopedN("LaunchProgram");
            ::detail::LaunchProgram(device, program);
        }

        TracyOpTTNNDevice(
            device_operation_t{},
            operation_id,
            device->id(),
            program,
            program_hash,
            operation_attributes,
            tensor_args,
            tensor_return_value);
    } else {
        auto program_hash = 0;
        bool program_cache_hit = false;
        log_operation<device_operation_t>(operation_attributes, tensor_args, program_hash, program_cache_hit);

        tt::stl::reflection::visit_object_of_type<Tensor>(CheckDeviceBufferIsAllocated{}, tensor_args);

        auto program = device_operation_t::select_program_factory(operation_attributes, tensor_args);

        auto program_ptr = std::visit(
            [&operation_attributes, &tensor_args, &tensor_return_value](auto&& program_factory) {
                using program_factory_t = std::decay_t<decltype(program_factory)>;
                return std::make_shared<tt::tt_metal::Program>(
                    program_factory_t::create(operation_attributes, tensor_args, tensor_return_value).program);
            },
            program);

        if (USE_FAST_DISPATCH) {
            ZoneScopedN("EnqueueProgram");
            auto& queue = device->command_queue(cq_id);
            tt::tt_metal::EnqueueProgram(queue, program_ptr, false);
        } else {
            ZoneScopedN("LaunchProgram");
            ::detail::LaunchProgram(device, program_ptr);
        }

        TracyOpTTNNDevice(
            device_operation_t{},
            operation_id,
            device->id(),
            *program_ptr,
            program_hash,
            operation_attributes,
            tensor_args,
            tensor_return_value);
    }
}

template <DeviceOperationConcept device_operation_t>
typename device_operation_t::tensor_return_value_t launch_on_single_device(
    uint8_t cq_id,
    const typename device_operation_t::operation_attributes_t& operation_attributes,
    const typename device_operation_t::tensor_args_t& tensor_args) {
    ZoneScopedN("Launch Device Operation");
    auto operation_id = assign_operation_id();

    // Create output tensor first
    auto tensor_return_value = device_operation_t::create_output_tensors(operation_attributes, tensor_args);
    // TODO: support the case when tensor args are empty? Or pass in the device as an argument in that case
    auto device = tt::stl::reflection::get_first_object_of_type<Tensor>(tensor_args).device();
    launch_on_worker_thread<device_operation_t>(cq_id, operation_id, operation_attributes, tensor_args, tensor_return_value, device);
    return tensor_return_value;
}

template <DeviceOperationConcept device_operation_t>
typename device_operation_t::tensor_args_t get_shard_tensor_args(std::size_t index, auto device, const typename device_operation_t::tensor_args_t& tensor_args) {
    auto get_shard = [device](const auto& tensor) {
        auto& storage = std::get<tt::tt_metal::MultiDeviceStorage>(tensor.get_storage());
        return Tensor{
            DeviceStorage{storage.get_buffer_for_device(device)},
            storage.get_tensor_shape_for_device(device),
            tensor.get_dtype(),
            tensor.get_layout()};
    };
    return tt::stl::reflection::transform_object_of_type<Tensor>(get_shard, tensor_args);
}

static Tensor make_tensor_return_value_from_shards(auto& old_storage, std::vector<Tensor>& output_shards) {
    return create_multi_device_tensor(output_shards, StorageType::MULTI_DEVICE, old_storage.strategy);
}

static std::vector<Tensor> make_tensor_return_value_from_shards(auto& old_storage,  std::vector<std::vector<Tensor>>& output_shards) {
    auto& first_shard = output_shards[0];

    std::vector<Tensor> output;
    output.reserve(first_shard.size());

    for (auto index = 0; index < first_shard.size(); index++) {
        std::vector<Tensor> tensors;
        for (auto shard_index = 0; shard_index < output_shards.size(); shard_index++) {
            tensors.push_back(output_shards[shard_index][index]);
        }
        output.push_back(make_tensor_return_value_from_shards(old_storage, tensors));
    }
    return output;
}

static std::vector<std::optional<Tensor>> make_tensor_return_value_from_shards(auto& old_storage, std::vector<std::vector<std::optional<Tensor>>>& output_shards) {
    auto& first_shard = output_shards[0];

    std::vector<std::optional<Tensor>> output;
    output.reserve(first_shard.size());

    for (auto index = 0; index < first_shard.size(); index++) {
        if (not first_shard[index].has_value()) {
            output.push_back(std::nullopt);
            continue;
        }
        std::vector<Tensor> tensors;
        for (auto shard_index = 0; shard_index < output_shards.size(); shard_index++) {
            tensors.push_back(output_shards[shard_index][index].value());
        }
        output.push_back(make_tensor_return_value_from_shards(old_storage, tensors));
    }
    return output;
}

template<typename T>
static T make_tensor_return_value_from_shards(auto& old_storage, std::vector<T>& output_shards) {
    // TODO: add logic to handle all types we want to support generically
    TT_THROW("make_tensor_return_value_from_shards is not implemented for this type. Please add an overload");
}

template <DeviceOperationConcept device_operation_t>
typename device_operation_t::tensor_return_value_t launch_on_multi_device(
    uint8_t cq_id,
    const typename device_operation_t::operation_attributes_t& operation_attributes,
    const typename device_operation_t::tensor_args_t& tensor_args) {
    ZoneScopedN("Launch Multi Device Operation");

    using tensor_return_value_t = typename device_operation_t::tensor_return_value_t;

    // TODO: support the case when tensor args are empty? Or pass in the device as an argument in that case
    auto first_tensor = tt::stl::reflection::get_first_object_of_type<Tensor>(tensor_args);
    const auto& storage = std::get<tt::tt_metal::MultiDeviceStorage>(first_tensor.get_storage());
    using storage_t = std::remove_cvref_t<decltype(storage)>;

    auto num_shards = storage.num_buffers();

    std::vector<tensor_return_value_t> outputs;
    outputs.reserve(num_shards);

    bool launch_shards_in_parallel = false;
    if (launch_shards_in_parallel) {
        std::vector<std::future<tensor_return_value_t>> shard_futures;
        shard_futures.reserve(num_shards);

        // Launch each shard
        for (auto shard_index = 0; shard_index < num_shards; shard_index++) {
            shard_futures.emplace_back(
                std::async(
                    std::launch::async,
                    [cq_id, operation_attributes, tensor_args, shard_index, storage]() mutable {
                        auto device = storage.get_buffer_for_device_id(shard_index)->device();
                        auto shard_tensor_args = get_shard_tensor_args<device_operation_t>(shard_index, device, tensor_args);
                        return launch_on_single_device<device_operation_t>(cq_id, operation_attributes, shard_tensor_args);
                    }));
        }

        // Combine shards into a multi-device storage
        for (auto& shard_future : shard_futures) {
            outputs.push_back(shard_future.get());
        }
    } else {
        for (auto shard_index = 0; shard_index < num_shards; shard_index++) {
            auto device = storage.get_buffer_for_device_id(shard_index)->device();
            auto shard_tensor_args = get_shard_tensor_args<device_operation_t>(shard_index, device, tensor_args);
            outputs.push_back(launch_on_single_device<device_operation_t>(cq_id, operation_attributes, shard_tensor_args));
        }
    }

    return make_tensor_return_value_from_shards(storage, outputs);
}

}  // namespace detail

template <DeviceOperationConcept device_operation_t>
typename device_operation_t::tensor_return_value_t run(
    uint8_t cq_id,
    const typename device_operation_t::operation_attributes_t& operation_attributes,
    const typename device_operation_t::tensor_args_t& tensor_args) {
    ZoneScopedN("Run Device Operation");

    using tensor_return_value_t = typename device_operation_t::tensor_return_value_t;
    static_assert(not std::same_as<tensor_return_value_t, void>, "Operation return type cannot be \"void\"");

    // TODO: support the case when tensor args are empty? Or pass in the device as an argument in that case
    auto first_tensor = tt::stl::reflection::get_first_object_of_type<Tensor>(tensor_args);
    const auto& storage = first_tensor.get_storage();

    return std::visit(
        [&cq_id, &operation_attributes, &tensor_args](auto&& storage) -> tensor_return_value_t {
            using storage_t = std::remove_cvref_t<decltype(storage)>;
            if constexpr (std::is_same_v<storage_t, tt::tt_metal::DeviceStorage>) {
                return detail::launch_on_single_device<device_operation_t>(cq_id, operation_attributes, tensor_args);
            } else if constexpr (std::is_same_v<storage_t, tt::tt_metal::MultiDeviceStorage>) {
                return detail::launch_on_multi_device<device_operation_t>(cq_id, operation_attributes, tensor_args);
            }
            else {
                TT_THROW("Unsupported storage type");
            }
        },
        storage);
}

}  // namespace device_operation

}  // namespace ttnn
