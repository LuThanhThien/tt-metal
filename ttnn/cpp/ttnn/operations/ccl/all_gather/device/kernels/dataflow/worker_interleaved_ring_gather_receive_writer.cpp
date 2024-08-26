// SPDX-FileCopyrightText: © 2023 Tenstorrent Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include <cstdint>
#include "dataflow_api.h"
#include "ttnn/cpp/ttnn/operations/ccl/all_gather/device/kernels/dataflow/worker_ring_gather_utils.hpp"
#include "ttnn/cpp/ttnn/operations/ccl/shared_with_host/sharded_tensor_addr_gen.hpp"


void kernel_main() {
    uint32_t arg_idx = 0;
    const uint32_t dst_addr = get_arg_val<uint32_t>(arg_idx++);
    // Different per worker receiver writer
    const uint32_t worker_sender_reader_noc_x = get_arg_val<uint32_t>(arg_idx++);
    const uint32_t worker_sender_reader_noc_y = get_arg_val<uint32_t>(arg_idx++);

    #ifdef SHARDED_MEM_LAYOUT
    uint32_t output_shard_grid_nrows = get_arg_val<uint32_t>(arg_idx++);
    const uint32_t * const output_shard_grid_row_map = reinterpret_cast<const uint32_t * const>(get_arg_addr(arg_idx));
    arg_idx += output_shard_grid_nrows;
    uint32_t output_shard_grid_ncols = get_arg_val<uint32_t>(arg_idx++);
    const uint32_t * const output_shard_grid_col_map = reinterpret_cast<const uint32_t * const>(get_arg_addr(arg_idx));
    arg_idx += output_shard_grid_ncols;
    #endif

    constexpr bool dst_is_dram = get_compile_time_arg_val(0) == 1;
    constexpr uint32_t num_transfers = get_compile_time_arg_val(1);
    constexpr uint32_t num_full_chunks = get_compile_time_arg_val(2);
    constexpr uint32_t page_size = get_compile_time_arg_val(3);
    constexpr uint32_t output_page_size = get_compile_time_arg_val(4);
    constexpr uint32_t num_pages = get_compile_time_arg_val(5);
    constexpr uint32_t rem_num_pages = get_compile_time_arg_val(6);
    constexpr uint32_t output_start_page_idx = get_compile_time_arg_val(7);
    constexpr uint32_t output_start_addr_offset = get_compile_time_arg_val(8);
    constexpr uint32_t row_start_idx = get_compile_time_arg_val(9);
    constexpr uint32_t col_start_idx = get_compile_time_arg_val(10);
    constexpr uint32_t row_offset = get_compile_time_arg_val(11);
    constexpr uint32_t col_offset = get_compile_time_arg_val(12);
    constexpr uint32_t num_rows = get_compile_time_arg_val(13);
    constexpr uint32_t num_cols = get_compile_time_arg_val(14);
    constexpr uint32_t last_output_page_offset = get_compile_time_arg_val(15);
    constexpr uint32_t output_page_offset = get_compile_time_arg_val(16);
    constexpr uint32_t last_output_addr_offset = get_compile_time_arg_val(17);
    constexpr uint32_t output_addr_offset = get_compile_time_arg_val(18);
    constexpr uint32_t input_start_ring_idx = get_compile_time_arg_val(19);
    // Same per worker receiver writer
    uint32_t sem_addr = get_semaphore(get_compile_time_arg_val(20));
    constexpr bool is_clockwise_direction = get_compile_time_arg_val(21) == 1;
    constexpr uint32_t half_cb_n_pages = get_compile_time_arg_val(22);
    constexpr uint32_t ring_size = get_compile_time_arg_val(23);
    static_assert(half_cb_n_pages > rem_num_pages, "half_cb_n_pages must be greater than or equal to rem_num_pages");

    #ifdef SHARDED_MEM_LAYOUT
    constexpr tt::tt_metal::TensorMemoryLayout output_tensor_memory_layout = static_cast<tt::tt_metal::TensorMemoryLayout>(get_compile_time_arg_val(24));
    constexpr uint32_t output_tensor_shard_grid_height = get_compile_time_arg_val(25);
    constexpr uint32_t output_tensor_shard_grid_width = get_compile_time_arg_val(26);
    constexpr uint32_t output_tensor_shard_grid_start_y_logical = get_compile_time_arg_val(27);
    constexpr uint32_t output_tensor_shard_grid_start_x_logical = get_compile_time_arg_val(28);
    constexpr uint32_t output_tensor_shard_pages_per_shard_y = get_compile_time_arg_val(29);
    constexpr uint32_t output_tensor_shard_pages_per_shard_x = get_compile_time_arg_val(30);
    constexpr bool output_tensor_shard_grid_transposed = get_compile_time_arg_val(31) != 0;
    #endif

    constexpr uint32_t cb_id_in0 = tt::CB::c_in0;
    #ifdef ROW_MAJOR_LAYOUT
        #ifdef INTERLEAVED_MEM_LAYOUT
        InterleavedAddrGen<dst_is_dram> d = {
            .bank_base_address = dst_addr + output_start_addr_offset, .page_size = output_page_size};
        #elif defined SHARDED_MEM_LAYOUT
            auto d =
                tt::tt_metal::address_generators::build_sharded_addr_gen<output_tensor_memory_layout>(
                tt::tt_metal::address_generators::HarvestedWormholeWorkerToNocLookup(output_shard_grid_nrows, output_shard_grid_row_map, output_shard_grid_ncols, output_shard_grid_col_map),
                tt::tt_metal::address_generators::DeviceShardSpecTypeGetter<output_tensor_memory_layout>::type(
                    output_tensor_shard_pages_per_shard_y,
                    output_tensor_shard_pages_per_shard_x,
                    output_tensor_shard_grid_height,
                    output_tensor_shard_grid_width,
                    output_tensor_shard_grid_start_y_logical,
                    output_tensor_shard_grid_start_x_logical,
                    output_tensor_shard_grid_transposed
                ),
                output_page_size,
                dst_addr
            );
            ASSERT(false);
        #endif
    #elif defined TILED_LAYOUT
        #ifdef INTERLEAVED_MEM_LAYOUT
        const DataFormat in0_df = get_dataformat(cb_id_in0);

        InterleavedAddrGenFast<dst_is_dram> d = {
            .bank_base_address = dst_addr,
            .page_size = output_page_size,
            .data_format = in0_df
        };
        #elif defined SHARDED_MEM_LAYOUT
            auto d =
                tt::tt_metal::address_generators::build_sharded_addr_gen<output_tensor_memory_layout>(
                tt::tt_metal::address_generators::HarvestedWormholeWorkerToNocLookup(output_shard_grid_nrows, output_shard_grid_row_map, output_shard_grid_ncols, output_shard_grid_col_map),
                tt::tt_metal::address_generators::DeviceShardSpecTypeGetter<output_tensor_memory_layout>::type(
                    output_tensor_shard_pages_per_shard_y,
                    output_tensor_shard_pages_per_shard_x,
                    output_tensor_shard_grid_height,
                    output_tensor_shard_grid_width,
                    output_tensor_shard_grid_start_y_logical,
                    output_tensor_shard_grid_start_x_logical,
                    output_tensor_shard_grid_transposed
                ),
                output_page_size,
                dst_addr
            );
        #endif
    #endif

    // Each worker receiver writer matches with a specific worker sender reader
    // Used to signal that data has been committed to memory and can be read
    const uint64_t worker_send_reader_semaphore_noc_addr = get_noc_addr(worker_sender_reader_noc_x, worker_sender_reader_noc_y, sem_addr);

    uint32_t input_ring_idx = input_start_ring_idx;
    uint32_t output_base_page_idx = output_start_page_idx;
    uint32_t output_page_idx = output_base_page_idx;
    uint32_t col_idx = col_start_idx;
    uint32_t row_idx = row_start_idx;

    for (uint32_t i = 0; i < num_transfers; ++i) {
        if constexpr (num_full_chunks > 0) {
            for (uint32_t c = 0; c < num_full_chunks; ++c) {

                #ifdef SHARDED_MEM_LAYOUT
                ASSERT(output_page_idx < output_tensor_shard_pages_per_shard_y * output_tensor_shard_pages_per_shard_x * output_tensor_shard_grid_height * output_tensor_shard_grid_width);
                #endif
                write_chunk(output_page_idx, col_idx, row_idx, cb_id_in0, d, num_cols, num_rows, col_offset, row_offset, num_pages, page_size);
                noc_semaphore_inc(worker_send_reader_semaphore_noc_addr, 1);
            }
        }
        if constexpr (rem_num_pages > 0) {
            #ifdef SHARDED_MEM_LAYOUT
            ASSERT(output_page_idx < output_tensor_shard_pages_per_shard_y * output_tensor_shard_pages_per_shard_x * output_tensor_shard_grid_height * output_tensor_shard_grid_width);
            #endif
            write_chunk(output_page_idx, col_idx, row_idx, cb_id_in0, d, num_cols, num_rows, col_offset, row_offset, rem_num_pages, page_size);
            noc_semaphore_inc(worker_send_reader_semaphore_noc_addr, 1);
            ASSERT(num_pages == 0 || num_pages > rem_num_pages);
            ASSERT(half_cb_n_pages > rem_num_pages);
            pop_filler_pages_from_cb(cb_id_in0, half_cb_n_pages - rem_num_pages);
        }

        if (is_clockwise_direction) {
            if (input_ring_idx == 0) {
                input_ring_idx = ring_size - 1;
                if constexpr(output_addr_offset != 0) {
                    d.bank_base_address += last_output_addr_offset;
                }
                if constexpr(output_page_offset != 0) {
                    output_base_page_idx += last_output_page_offset;
                }
            } else {
                input_ring_idx--;
                if constexpr(output_addr_offset != 0) {
                    d.bank_base_address -= output_addr_offset;
                }
                if constexpr(output_page_offset != 0) {
                    output_base_page_idx -= output_page_offset;
                }
            }
        } else {
            if (input_ring_idx == ring_size - 1) {
                input_ring_idx = 0;
                if constexpr(output_addr_offset != 0) {
                    d.bank_base_address -= last_output_addr_offset;
                }
                if constexpr(output_page_offset != 0) {
                    output_base_page_idx -= last_output_page_offset;
                }
            } else {
                input_ring_idx++;
                if constexpr(output_addr_offset != 0) {
                    d.bank_base_address += output_addr_offset;
                }
                if constexpr(output_page_offset != 0) {
                    output_base_page_idx += output_page_offset;
                }
            }

        }
        output_page_idx = output_base_page_idx;
        col_idx = col_start_idx;
        row_idx = row_start_idx;
    }
}
