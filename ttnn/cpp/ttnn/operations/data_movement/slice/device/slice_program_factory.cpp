// SPDX-FileCopyrightText: © 2023 Tenstorrent Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include "optional"
#include "ttnn/deprecated/tt_dnn/op_library/math.hpp"
#include "ttnn/deprecated/tt_dnn/op_library/work_split.hpp"
#include "tt_metal/common/constants.hpp"
#include "tt_metal/detail/util.hpp"
#include "tt_metal/host_api.hpp"

#include "slice_op.hpp"
using namespace tt::constants;


namespace ttnn::operations::data_movement::detail {

inline std::vector<std::pair<std::vector<uint32_t>, std::vector<uint32_t>>> get_slice_runtime_args_rm(
    const Tensor& input_tensor,
    Tensor& output_tensor,
    const tt::tt_metal::Shape& output_tensor_start,
    uint32_t num_cores_total,
    uint32_t num_cores,
    uint32_t num_cores_y,
    CoreRangeSet core_group_1,
    CoreRangeSet core_group_2,
    uint32_t num_sticks_per_core_group_1,
    uint32_t num_sticks_per_core_group_2,
    uint32_t max_read_size) {
    tt::tt_metal::Device* device = input_tensor.device();

    auto input_buffer = input_tensor.buffer();
    auto output_buffer = output_tensor.buffer();
    auto input_shape = input_tensor.get_legacy_shape();
    auto output_shape = output_tensor.get_legacy_shape();

    uint32_t padded_row_size_bytes = input_shape[-1] * input_tensor.element_size();
    uint32_t unpadded_row_size_bytes = output_shape[-1] * input_tensor.element_size();

    std::uint32_t num_dims = static_cast<std::uint32_t>(input_shape.rank());
    std::vector<uint32_t> num_unpadded_sticks_per_dim(num_dims);
    std::vector<uint32_t> num_padded_sticks_per_dim(num_dims);
    std::vector<uint32_t> id_per_dim(num_dims);

    std::vector<uint32_t> accumulated_total_per_dim(num_dims);

    // TODO: Remove first element of these arrays and update kernel accordingly
    // This currently just matches tile version where we iterate over the row as well
    num_unpadded_sticks_per_dim[0] = 1;
    num_padded_sticks_per_dim[0] = 0;
    accumulated_total_per_dim[0] = 1;

    for (int32_t i = 1; i < num_dims; i++) {
        uint32_t num_unpadded_dim = output_shape[-(i + 1)];
        uint32_t num_total_dim = input_shape[-(i + 1)];
        uint32_t num_padded_dim = (num_total_dim - num_unpadded_dim) * accumulated_total_per_dim[i - 1];
        num_unpadded_sticks_per_dim[i] = num_unpadded_dim;
        num_padded_sticks_per_dim[i] = num_padded_dim;
        accumulated_total_per_dim[i] = num_total_dim * accumulated_total_per_dim[i - 1];
    }

    uint32_t unpadded_row_size_bytes_offset = tt::round_up(unpadded_row_size_bytes, TILE_WIDTH / 2);

    vector<uint32_t> common_reader_kernel_args = {
        input_tensor.buffer()->address() + output_tensor_start[-1] * output_tensor.element_size(),
        padded_row_size_bytes,
        unpadded_row_size_bytes,
        unpadded_row_size_bytes_offset,
        num_dims,
        0,
        0,
        0,
        0};
    common_reader_kernel_args.insert(
        common_reader_kernel_args.end(), num_unpadded_sticks_per_dim.begin(), num_unpadded_sticks_per_dim.end());
    common_reader_kernel_args.insert(
        common_reader_kernel_args.end(), num_padded_sticks_per_dim.begin(), num_padded_sticks_per_dim.end());

    std::vector<std::pair<std::vector<uint32_t>, std::vector<uint32_t>>> ret_val(num_cores_total);

    uint32_t start_offset = ttnn::operations::data_movement::get_rm_start_offset(input_tensor, ttnn::Shape(output_tensor_start));
    for (uint32_t i = 0, num_sticks_written = 0; i < num_cores_total; i++) {
        CoreCoord core = {i / num_cores_y, i % num_cores_y};
        uint32_t num_sticks_per_core;
        if (core_group_1.core_coord_in_core_ranges(core)) {
            num_sticks_per_core = num_sticks_per_core_group_1;
        } else if (core_group_2.core_coord_in_core_ranges(core)) {
            num_sticks_per_core = num_sticks_per_core_group_2;
        } else {
            // no-op
            num_sticks_per_core = 0;
        }

        // issue more reads before calling barrier
        uint32_t num_sticks_per_core_read = 0, num_read_per_barrier = 0;
        if (num_sticks_per_core != 0) {
            auto num_sticks_per_core_pad32 = num_sticks_per_core + (32 - num_sticks_per_core % 32) % 32;
            num_sticks_per_core_read = merge_num_sticks_to_read(num_sticks_per_core_pad32, unpadded_row_size_bytes_offset, max_read_size);
            num_read_per_barrier = num_sticks_per_core_pad32 / num_sticks_per_core_read;
        }

        id_per_dim[0] = num_sticks_written % num_unpadded_sticks_per_dim[0];
        uint32_t unpadded_written = num_sticks_written / num_unpadded_sticks_per_dim[0];
        uint32_t start_id = id_per_dim[0] + start_offset;

        for (uint32_t j = 1; j < num_dims; j++) {
            id_per_dim[j] = unpadded_written % num_unpadded_sticks_per_dim[j];
            unpadded_written = unpadded_written / num_unpadded_sticks_per_dim[j];
            start_id += id_per_dim[j] * accumulated_total_per_dim[j - 1];
        }
        vector<uint32_t> reader_kernel_args = common_reader_kernel_args;
        //
        uint32_t addr_offset = 5;  // input buffer addr, padded_row_size_bytes, unpadded_row_size_bytes, num_dims
        reader_kernel_args[addr_offset++] = start_id;
        reader_kernel_args[addr_offset++] = num_sticks_per_core;
        reader_kernel_args[addr_offset++] = num_sticks_per_core_read;
        reader_kernel_args[addr_offset] = num_read_per_barrier;
        reader_kernel_args.insert(reader_kernel_args.end(), id_per_dim.begin(), id_per_dim.end());

        vector<uint32_t> writer_kernel_args = {
            output_buffer->address(), unpadded_row_size_bytes, unpadded_row_size_bytes_offset, num_sticks_per_core, num_sticks_per_core_read, num_read_per_barrier, num_sticks_written, 0};
        num_sticks_written += num_sticks_per_core;
        ret_val[i] = {reader_kernel_args, writer_kernel_args};
    }

    return ret_val;
}

operation::ProgramWithCallbacks slice_rm_multi_core(
    const Tensor& a, Tensor& output, const tt::tt_metal::Shape& output_tensor_start, const tt::tt_metal::Shape& output_tensor_end) {
    const tt::tt_metal::Shape output_shape = output.get_legacy_shape();

    tt::tt_metal::Program program = tt::tt_metal::CreateProgram();

    // This should allocate a DRAM buffer on the device
    tt::tt_metal::Device* device = a.device();

    uint32_t num_unpadded_sticks = output.volume() / output.get_legacy_shape()[-1];

    auto compute_with_storage_grid_size = device->compute_with_storage_grid_size();
    uint32_t num_cores_x = compute_with_storage_grid_size.x;
    uint32_t num_cores_y = compute_with_storage_grid_size.y;

    CoreRange total_cores({0, 0}, {num_cores_x - 1, num_cores_y - 1});
    uint32_t num_cores_total = num_cores_x * num_cores_y;
    auto [num_cores, all_cores, core_group_1, core_group_2, num_sticks_per_core_group_1, num_sticks_per_core_group_2] =
        split_work_to_cores(compute_with_storage_grid_size, num_unpadded_sticks);

    tt::tt_metal::Buffer* src0_buffer = a.buffer();

    tt::DataFormat cb_data_format = tt::tt_metal::datatype_to_dataformat_converter(a.get_dtype());

    uint32_t padded_row_size_bytes = a.get_legacy_shape()[-1] * a.element_size();
    uint32_t unpadded_row_size_bytes = output_shape[-1] * a.element_size();

    tt::tt_metal::Buffer* dst_buffer = output.buffer();
    TT_ASSERT(dst_buffer != nullptr, "Output buffer should be allocated on device!");

    bool src0_is_dram = src0_buffer->buffer_type() == tt::tt_metal::BufferType::DRAM ? 1 : 0;
    std::vector<uint32_t> reader_compile_time_args_vec = {(std::uint32_t)src0_is_dram};
    bool dst_is_dram = dst_buffer->buffer_type() == tt::tt_metal::BufferType::DRAM ? 1 : 0;

    uint32_t src_stick_size = padded_row_size_bytes;
    uint32_t dst_stick_size = unpadded_row_size_bytes;

    uint32_t src0_cb_index = 0;
    uint32_t max_read_size = 4096;
    uint32_t cb_page_size = dst_is_dram ? tt::round_up(unpadded_row_size_bytes, TILE_WIDTH) : tt::round_up(unpadded_row_size_bytes, TILE_WIDTH / 2);
    uint32_t num_input_pages = num_sticks_per_core_group_1 > num_sticks_per_core_group_2 ? num_sticks_per_core_group_1 : num_sticks_per_core_group_2;
    uint32_t num_sticks_per_core_read = 0, num_read_per_barrier = 0;
    if (num_input_pages != 0) {
        auto num_sticks_per_core_pad32 = num_input_pages + (32 - num_input_pages % 32) % 32;
        num_sticks_per_core_read = merge_num_sticks_to_read(num_sticks_per_core_pad32, cb_page_size, max_read_size);
        num_read_per_barrier = num_sticks_per_core_pad32 / num_sticks_per_core_read;
    }
    tt::tt_metal::CircularBufferConfig cb_src0_config =
        tt::tt_metal::CircularBufferConfig(num_read_per_barrier * 2 * cb_page_size, {{src0_cb_index, cb_data_format}})
            .set_page_size(src0_cb_index, cb_page_size);
    auto cb_src0 = tt::tt_metal::CreateCircularBuffer(program, total_cores, cb_src0_config);


    std::vector<uint32_t> writer_compile_time_args_vec = {(std::uint32_t)src0_cb_index, (std::uint32_t)dst_is_dram};

    tt::tt_metal::KernelHandle unary_reader_kernel_id = tt::tt_metal::CreateKernel(
        program,
        "ttnn/cpp/ttnn/operations/data_movement/slice/device/kernels/dataflow/slice_reader_unary_unpad_dims_rm_interleaved_start_id.cpp",
        total_cores,
        tt::tt_metal::ReaderDataMovementConfig(reader_compile_time_args_vec));

    tt::tt_metal::KernelHandle unary_writer_kernel_id = tt::tt_metal::CreateKernel(
        program,
        "ttnn/cpp/ttnn/operations/data_movement/slice/device/kernels/dataflow/slice_writer_unary_stick_layout_interleaved_start_id.cpp",
        total_cores,
        tt::tt_metal::WriterDataMovementConfig(writer_compile_time_args_vec));

    auto all_runtime_args = get_slice_runtime_args_rm(
        a,
        output,
        output_tensor_start,
        num_cores_total,
        num_cores,
        num_cores_y,
        core_group_1,
        core_group_2,
        num_sticks_per_core_group_1,
        num_sticks_per_core_group_2,
        max_read_size);

    for (uint32_t i = 0, num_sticks_written = 0; i < num_cores_total; i++) {
        CoreCoord core = {i / num_cores_y, i % num_cores_y};
        tt::tt_metal::SetRuntimeArgs(program, unary_reader_kernel_id, core, all_runtime_args[i].first);

        tt::tt_metal::SetRuntimeArgs(program, unary_writer_kernel_id, core, all_runtime_args[i].second);
    }

    auto override_runtime_args_callback =
        [unary_reader_kernel_id, unary_writer_kernel_id, compute_with_storage_grid_size, max_read_size](
            const void* operation,
            const Program& program,
            const std::vector<Tensor>& input_tensors,
            const std::vector<std::optional<const Tensor>>&,
            const std::vector<Tensor>& output_tensors) {
            auto src_tensor = input_tensors.at(0);
            auto dst_tensor = output_tensors.at(0);
            uint32_t num_cores_x = compute_with_storage_grid_size.x;
            uint32_t num_cores_y = compute_with_storage_grid_size.y;
            uint32_t num_cores_total = num_cores_x * num_cores_y;
            uint32_t num_unpadded_sticks = dst_tensor.volume() / dst_tensor.get_legacy_shape()[-1];
            auto
                [num_cores,
                 all_cores,
                 core_group_1,
                 core_group_2,
                 num_sticks_per_core_group_1,
                 num_sticks_per_core_group_2] =
                    split_work_to_cores(compute_with_storage_grid_size, num_unpadded_sticks);

            const auto tensor_start = static_cast<const ttnn::operations::data_movement::SliceDeviceOperation *>(operation)->slice_start;
            auto all_runtime_args = get_slice_runtime_args_rm(
                src_tensor,
                dst_tensor,
                tensor_start,
                num_cores_total,
                num_cores,
                num_cores_y,
                core_group_1,
                core_group_2,
                num_sticks_per_core_group_1,
                num_sticks_per_core_group_2,
                max_read_size);

            for (uint32_t i = 0, num_tiles_written = 0; i < num_cores_total; i++) {
                CoreCoord core = {i / num_cores_y, i % num_cores_y};

                { SetRuntimeArgs(program, unary_reader_kernel_id, core, all_runtime_args[i].first); }

                { SetRuntimeArgs(program, unary_writer_kernel_id, core, all_runtime_args[i].second); }
            }
        };

    return {.program = std::move(program), .override_runtime_arguments_callback = override_runtime_args_callback};
}

template <bool initialize_args>
inline __attribute__((always_inline)) void set_slice_runtime_args_tile(
    const Tensor& input_tensor,
    const Tensor& output_tensor,
    const tt::tt_metal::Shape& output_tensor_start,
    const uint32_t& num_cores_total,
    const uint32_t& num_cores,
    const std::vector<CoreCoord>& cores,
    const uint32_t& num_cores_group_1,
    const uint32_t& num_cores_group_2,
    const uint32_t& num_tiles_per_core_group_1,
    const uint32_t& num_tiles_per_core_group_2,
    const Program& program,
    const tt::tt_metal::KernelHandle& unary_reader_kernel_id,
    const tt::tt_metal::KernelHandle& unary_writer_kernel_id,
    std::vector<uint32_t>& accumulated_total_per_dim) {
    const auto input_buffer = input_tensor.buffer();
    const auto output_buffer = output_tensor.buffer();
    const auto& input_shape = input_tensor.get_legacy_shape();
    const auto& output_shape = output_tensor.get_legacy_shape();

    std::uint32_t num_dims = static_cast<std::uint32_t>(input_shape.rank());

    uint32_t num_unpadded_Xt = output_shape[-1] / TILE_WIDTH;
    uint32_t num_total_Xt = input_shape[-1] / TILE_WIDTH;
    uint32_t num_padded_Xt = num_total_Xt - num_unpadded_Xt;
    uint32_t num_unpadded_Yt = output_shape[-2] / TILE_HEIGHT;
    uint32_t num_total_Yt = input_shape[-2] / TILE_HEIGHT;
    uint32_t num_padded_Yt = (num_total_Yt - num_unpadded_Yt) * num_total_Xt;

    const auto set_common_reader_args = [&](
        uint32_t* reader_common_args,
        uint32_t* num_unpadded_tiles_per_dim,
        uint32_t* num_padded_tiles_per_dim) __attribute__((always_inline)) {
        reader_common_args[0] = input_buffer->address();
        num_unpadded_tiles_per_dim[0] = num_unpadded_Xt;
        num_unpadded_tiles_per_dim[1] = num_unpadded_Yt;
        num_padded_tiles_per_dim[0] = num_padded_Xt;
        num_padded_tiles_per_dim[1] = num_padded_Yt;
        accumulated_total_per_dim[0] = num_total_Xt;
        accumulated_total_per_dim[1] = num_total_Yt * num_total_Xt;
        for (int32_t i = 2; i < num_dims; ++i) {
            uint32_t num_unpadded_dim = output_shape[-(i + 1)];
            uint32_t num_total_dim = input_shape[-(i + 1)];
            uint32_t num_padded_dim = (num_total_dim - num_unpadded_dim) * accumulated_total_per_dim[i - 1];
            num_unpadded_tiles_per_dim[i] = num_unpadded_dim;
            num_padded_tiles_per_dim[i] = num_padded_dim;
            accumulated_total_per_dim[i] = num_total_dim * accumulated_total_per_dim[i - 1];
        }
    };

    const auto set_reader_rt_args = [&](
        uint32_t* reader_rt_args,
        const uint32_t* num_unpadded_tiles_per_dim,
        const uint32_t* num_padded_tiles_per_dim,
        const uint32_t& num_tiles_per_core,
        const uint32_t& start_offset,
        const uint32_t& num_tiles_written) __attribute__((always_inline)) {
        reader_rt_args[2] = num_tiles_written % num_unpadded_tiles_per_dim[0];
        uint32_t unpadded_written = num_tiles_written / num_unpadded_tiles_per_dim[0];
        uint32_t start_id = reader_rt_args[2] + start_offset;
        for (uint32_t j = 1; j < num_dims; ++j) {
            reader_rt_args[2 + j] = unpadded_written % num_unpadded_tiles_per_dim[j];
            unpadded_written = unpadded_written / num_unpadded_tiles_per_dim[j];
            start_id += reader_rt_args[2 + j] * accumulated_total_per_dim[j - 1];
        }
        reader_rt_args[0] = start_id;
        reader_rt_args[1] = num_tiles_per_core;
    };

    if constexpr (initialize_args) {
        std::vector<uint32_t> reader_common_args(1 + num_dims * 2);
        uint32_t* num_unpadded_tiles_per_dim = reader_common_args.data() + 1;
        uint32_t* num_padded_tiles_per_dim = num_unpadded_tiles_per_dim + num_dims;
        set_common_reader_args(reader_common_args.data(), num_unpadded_tiles_per_dim, num_padded_tiles_per_dim);
        SetCommonRuntimeArgs(program, unary_reader_kernel_id, reader_common_args);
    }
    auto& reader_common_args = GetCommonRuntimeArgs(program, unary_reader_kernel_id);
    uint32_t* num_unpadded_tiles_per_dim = reader_common_args.data() + 1;
    uint32_t* num_padded_tiles_per_dim = num_unpadded_tiles_per_dim + num_dims;
    if constexpr (!initialize_args) {
        set_common_reader_args(reader_common_args.data(), num_unpadded_tiles_per_dim, num_padded_tiles_per_dim);
    }

    uint32_t start_offset = ttnn::operations::data_movement::get_tiled_start_offset(input_tensor, ttnn::Shape(output_tensor_start));

    auto& reader_kernel_args_by_core = GetRuntimeArgs(program, unary_reader_kernel_id);
    auto& writer_kernel_args_by_core = GetRuntimeArgs(program, unary_writer_kernel_id);
    const uint32_t num_used_cores = num_cores_group_1 + num_cores_group_2;
    for (uint32_t i = 0, num_tiles_written = 0; i < num_cores_total; ++i) {
        const CoreCoord& core = cores[i];
        uint32_t num_tiles_per_core;
        if (i < num_cores_group_1) {
            num_tiles_per_core = num_tiles_per_core_group_1;
        } else if (i < num_used_cores) {
            num_tiles_per_core = num_tiles_per_core_group_2;
        } else {
            // no-op
            if constexpr (initialize_args) {
                std::vector<uint32_t> reader_kernel_args(2 + num_dims, 0);
                std::vector<uint32_t> writer_kernel_args(3, 0);
                tt::tt_metal::SetRuntimeArgs(program, unary_reader_kernel_id, core, reader_kernel_args);
                tt::tt_metal::SetRuntimeArgs(program, unary_writer_kernel_id, core, writer_kernel_args);
            } else {
                auto& reader_kernel_args = reader_kernel_args_by_core[core.x][core.y];
                reader_kernel_args[1] = 0;
                auto& writer_kernel_args = writer_kernel_args_by_core[core.x][core.y];
                writer_kernel_args[1] = 0;
            }
            continue;
        }

        if constexpr (initialize_args) {
            std::vector<uint32_t> reader_kernel_args(2 + num_dims);
            set_reader_rt_args(
                reader_kernel_args.data(),
                num_unpadded_tiles_per_dim,
                num_padded_tiles_per_dim,
                num_tiles_per_core,
                start_offset,
                num_tiles_written);
            SetRuntimeArgs(program, unary_reader_kernel_id, core, reader_kernel_args);
        } else {
            auto& reader_kernel_args = reader_kernel_args_by_core[core.x][core.y];
            set_reader_rt_args(
                reader_kernel_args.data(),
                num_unpadded_tiles_per_dim,
                num_padded_tiles_per_dim,
                num_tiles_per_core,
                start_offset,
                num_tiles_written);
        }

        if constexpr (initialize_args) {
            vector<uint32_t> writer_kernel_args = {output_buffer->address(), num_tiles_per_core, num_tiles_written};
            tt::tt_metal::SetRuntimeArgs(program, unary_writer_kernel_id, core, writer_kernel_args);
        } else {
            auto& writer_kernel_args = writer_kernel_args_by_core[core.x][core.y];
            writer_kernel_args[0] = output_buffer->address();
            writer_kernel_args[1] = num_tiles_per_core;
            writer_kernel_args[2] = num_tiles_written;
        }
        num_tiles_written += num_tiles_per_core;
    }
}

operation::ProgramWithCallbacks slice_tile_multi_core(
    const Tensor& a, Tensor& output, const tt::tt_metal::Shape& output_tensor_start, const tt::tt_metal::Shape& output_tensor_end) {
    const tt::tt_metal::Shape output_shape = output.get_legacy_shape();

    tt::tt_metal::Program program = tt::tt_metal::CreateProgram();

    // This should allocate a DRAM buffer on the device
    tt::tt_metal::Device* device = a.device();

    uint32_t num_unpadded_tiles = output.volume() / TILE_HW;

    auto compute_with_storage_grid_size = device->compute_with_storage_grid_size();
    uint32_t num_cores_x = compute_with_storage_grid_size.x;
    uint32_t num_cores_y = compute_with_storage_grid_size.y;
    auto num_cores_total = num_cores_x * num_cores_y;
    CoreRange total_cores({0, 0}, {num_cores_x - 1, num_cores_y - 1});

    auto [num_cores, all_cores, core_group_1, core_group_2, num_tiles_per_core_group_1, num_tiles_per_core_group_2] =
        split_work_to_cores(compute_with_storage_grid_size, num_unpadded_tiles);

    tt::tt_metal::Buffer* src0_buffer = a.buffer();

    tt::tt_metal::Buffer* dst_buffer = output.buffer();
    TT_ASSERT(dst_buffer != nullptr, "Output buffer should be allocated on device!");

    tt::DataFormat cb_data_format = tt::tt_metal::datatype_to_dataformat_converter(a.get_dtype());
    uint32_t single_tile_size = tt::tt_metal::detail::TileSize(cb_data_format);

    uint32_t src0_cb_index = 0;
    uint32_t num_input_tiles = 2;
    tt::tt_metal::CircularBufferConfig cb_src0_config =
        tt::tt_metal::CircularBufferConfig(num_input_tiles * single_tile_size, {{src0_cb_index, cb_data_format}})
            .set_page_size(src0_cb_index, single_tile_size);
    auto cb_src0 = tt::tt_metal::CreateCircularBuffer(program, total_cores, cb_src0_config);

    std::uint32_t num_dims = static_cast<std::uint32_t>(a.get_legacy_shape().rank());

    // Reader compile-time args
    // Data is 32 byte aligned
    bool src0_is_dram = src0_buffer->buffer_type() == tt::tt_metal::BufferType::DRAM ? 1 : 0;
    bool dst_is_dram = dst_buffer->buffer_type() == tt::tt_metal::BufferType::DRAM ? 1 : 0;
    std::vector<uint32_t> reader_compile_time_args = {
        static_cast<uint32_t>(src0_cb_index),
        static_cast<uint32_t>(num_dims),
        static_cast<uint32_t>(src0_is_dram),
    };
    std::vector<uint32_t> writer_compile_time_args = {
        static_cast<uint32_t>(src0_cb_index), static_cast<uint32_t>(dst_is_dram)};

    // Tilized reader
    tt::tt_metal::KernelHandle unary_reader_kernel_id = tt::tt_metal::CreateKernel(
        program,
        "ttnn/cpp/ttnn/operations/data_movement/slice/device/kernels/dataflow/reader_unary_unpad_dims_interleaved_start_id.cpp",
        total_cores,
        tt::tt_metal::ReaderDataMovementConfig(reader_compile_time_args));

    tt::tt_metal::KernelHandle unary_writer_kernel_id = tt::tt_metal::CreateKernel(
        program,
        "ttnn/cpp/ttnn/operations/eltwise/unary/device/kernels/dataflow/writer_unary_interleaved_start_id.cpp",
        total_cores,
        tt::tt_metal::WriterDataMovementConfig(writer_compile_time_args));

    const auto cores = grid_to_cores(num_cores_total, num_cores_x, num_cores_y, false);

    std::vector<uint32_t> accumulated_total_per_dim(num_dims);
    set_slice_runtime_args_tile<true>(
        a,
        output,
        output_tensor_start,
        num_cores_total,
        num_cores,
        cores,
        core_group_1.num_cores(),
        core_group_2.num_cores(),
        num_tiles_per_core_group_1,
        num_tiles_per_core_group_2,
        program,
        unary_reader_kernel_id,
        unary_writer_kernel_id,
        accumulated_total_per_dim);

    auto override_runtime_args_callback = [unary_reader_kernel_id,
                                           unary_writer_kernel_id,
                                           compute_with_storage_grid_size,
                                           cores,
                                           accumulated_total_per_dim](
                                              const void* operation,
                                              const Program& program,
                                              const std::vector<Tensor>& input_tensors,
                                              const std::vector<std::optional<const Tensor>>&,
                                              const std::vector<Tensor>& output_tensors) mutable {
        const Tensor& src_tensor = input_tensors[0];
        const Tensor& dst_tensor = output_tensors[0];
        uint32_t num_unpadded_tiles = dst_tensor.volume() / TILE_HW;

        uint32_t num_cores_x = compute_with_storage_grid_size.x;
        uint32_t num_cores_y = compute_with_storage_grid_size.y;
        uint32_t num_cores_total = cores.size();

        auto
            [num_cores, all_cores, core_group_1, core_group_2, num_tiles_per_core_group_1, num_tiles_per_core_group_2] =
                split_work_to_cores(compute_with_storage_grid_size, num_unpadded_tiles);

        const auto& tensor_start = static_cast<const ttnn::operations::data_movement::SliceDeviceOperation *>(operation)->slice_start;
        set_slice_runtime_args_tile<false>(
            src_tensor,
            dst_tensor,
            tensor_start,
            num_cores_total,
            num_cores,
            cores,
            core_group_1.num_cores(),
            core_group_2.num_cores(),
            num_tiles_per_core_group_1,
            num_tiles_per_core_group_2,
            program,
            unary_reader_kernel_id,
            unary_writer_kernel_id,
            accumulated_total_per_dim);
    };

    return {.program = std::move(program), .override_runtime_arguments_callback = override_runtime_args_callback};
}

operation::ProgramWithCallbacks slice_multi_core(
    const Tensor& a, Tensor& output, const tt::tt_metal::Shape& output_tensor_start, const tt::tt_metal::Shape& output_tensor_end) {
    switch (a.get_layout()) {
        case Layout::ROW_MAJOR: return slice_rm_multi_core(a, output, output_tensor_start, output_tensor_end);
        case Layout::TILE: return slice_tile_multi_core(a, output, output_tensor_start, output_tensor_end);
        default: TT_ASSERT(false, "Unsupported Layout");
    }
    return {};
}

}  // namespace ttnn::operations::data_movement::detail 

