// SPDX-FileCopyrightText: © 2023 Tenstorrent Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include <stdint.h>
#include "ttsim.h"
#include "../bmm_op.hpp"

void reader_bmm_tile_layout_in0_receiver_in1_sender(
    uint32_t in0_tensor_start_tile_id,
    uint32_t in0_tensor_stride_w,
    uint32_t in0_tensor_stride_h,
    uint32_t in0_tensor_next_block_stride,
    uint32_t in0_block_w,
    uint32_t in0_block_h,
    uint32_t in0_block_num_tiles,
    uint32_t in1_tensor_start_tile_id,
    uint32_t in1_tensor_stride_w,
    uint32_t in1_tensor_stride_h,
    uint32_t in1_tensor_next_block_stride,
    uint32_t in1_block_w,
    uint32_t in1_block_h,
    uint32_t in1_block_num_tiles,
    uint32_t num_blocks,

    // noc
    uint32_t in0_mcast_dest_noc_start_x,
    uint32_t in0_mcast_dest_noc_start_y,
    uint32_t in0_mcast_dest_noc_end_x,
    uint32_t in0_mcast_dest_noc_end_y,
    uint32_t in0_mcast_num_dests,
    uint32_t in0_mcast_sender_noc_x,
    uint32_t in0_mcast_sender_noc_y,
    uint32_t in1_mcast_dest_noc_start_x,
    uint32_t in1_mcast_dest_noc_start_y,
    uint32_t in1_mcast_dest_noc_end_x,
    uint32_t in1_mcast_dest_noc_end_y,
    uint32_t in1_mcast_num_dests,
    uint32_t in1_mcast_sender_noc_x,
    uint32_t in1_mcast_sender_noc_y,

    // bmm
    uint32_t MtKt,
    uint32_t KtNt,
    uint32_t batch,
    uint32_t bcast_B,

    CoreArgs *core_args
) {
    int core_id = core_args->core_id;
    #define PRINT PRINT_CORE(READER, core_id)
    #define ENDL "\n"


    // Commented out device-specific code for simulation
    // volatile tt_l1_ptr uint32_t* in0_mcast_receiver_semaphore_addr_ptr = reinterpret_cast<volatile tt_l1_ptr uint32_t*>(in0_mcast_receiver_semaphore_addr);
    // volatile tt_l1_ptr uint32_t* in1_mcast_receiver_semaphore_addr_ptr = reinterpret_cast<volatile tt_l1_ptr uint32_t*>(in1_mcast_receiver_semaphore_addr);
    // *(in1_mcast_receiver_semaphore_addr_ptr) = VALID;
    // volatile tt_l1_ptr uint32_t* in1_mcast_sender_semaphore_addr_ptr = reinterpret_cast<volatile tt_l1_ptr uint32_t*>(in1_mcast_sender_semaphore_addr);

    // const InterleavedAddrGenFast<in1_is_dram> s1 = {
    //     .bank_base_address = in1_tensor_addr,
    //     .page_size = single_tile_size_bytes,
    //     .data_format = data_format
    // };

    for (uint32_t b = 0; b < batch; b++) {
        uint32_t in1_tensor_current_block_start_tile_id = in1_tensor_start_tile_id;
        for(uint32_t block = 0; block < num_blocks; block++) {
            PRINT << "Processing block " << block << ENDL;

            // Operand 0
            // Commented out device-specific code for simulation
            // cb_reserve_back(cb_id_in0, in0_block_num_tiles);
            // noc_semaphore_set(in0_mcast_receiver_semaphore_addr_ptr, INVALID);
            // uint64_t in0_mcast_sender_semaphore_noc_addr = get_noc_addr(in0_mcast_sender_noc_x, in0_mcast_sender_noc_y, in0_mcast_sender_semaphore_addr);
            // noc_semaphore_inc(in0_mcast_sender_semaphore_noc_addr, 1);
            // noc_semaphore_wait(in0_mcast_receiver_semaphore_addr_ptr, VALID);
            // cb_push_back(cb_id_in0, in0_block_num_tiles);
            core_args->neighboard_read(1);

            // Operand 1
            // Commented out device-specific code for simulation
            // cb_reserve_back(cb_id_in1, in1_block_num_tiles);
            // l1_write_addr_in1 = get_write_ptr(cb_id_in1);

            // Simulated Copy in1 block into CB
            uint32_t in1_tensor_row_start_tile_id = in1_tensor_current_block_start_tile_id;
            for(uint32_t h = 0; h < in1_block_h; h++) {
                uint32_t in1_tensor_tile_id = in1_tensor_row_start_tile_id;
                for(uint32_t w = 0; w < in1_block_w; w++) {
                    PRINT_LEVEL(2); PRINT << "Processing in1 block height " << h + 1 << "/" << in1_block_h << " and width " << w + 1 << "/" << in1_block_w << ENDL;

                    // Commented out device-specific code for simulation
                    // noc_async_read_tile(in1_tensor_tile_id, s1, l1_write_addr_in1);
                    core_args->dram_read(1);
                    // l1_write_addr_in1 += single_tile_size_bytes;
                    in1_tensor_tile_id += in1_tensor_stride_w;
                    // in1_block_size_bytes += single_tile_size_bytes;
                }
                in1_tensor_row_start_tile_id += in1_tensor_stride_h;
            }
            in1_tensor_current_block_start_tile_id += in1_tensor_next_block_stride;

            // Barrier! make sure the reads are done
            // Commented out device-specific code for simulation
            // noc_async_read_barrier();

            // Wait until all in1 mcast destinations have incremented the semaphore
            // Commented out device-specific code for simulation
            // noc_semaphore_wait(in1_mcast_sender_semaphore_addr_ptr, in1_mcast_num_dests);
            // noc_semaphore_set(in1_mcast_sender_semaphore_addr_ptr, 0);

            // Mcast to destinations
            // Commented out device-specific code for simulation
            // uint64_t in1_multicast_data_addr = get_noc_multicast_addr(
            //     in1_mcast_dest_noc_start_x,
            //     in1_mcast_dest_noc_start_y,
            //     in1_mcast_dest_noc_end_x,
            //     in1_mcast_dest_noc_end_y,
            //     in1_start_address);
            // noc_async_write_multicast(in1_start_address, in1_multicast_data_addr, in1_block_size_bytes, in1_mcast_num_dests);

            // Multicast the flag to destinations
            // Commented out device-specific code for simulation
            // uint64_t in1_mcast_receiver_semaphore_noc_addr = get_noc_multicast_addr(
            //     in1_mcast_dest_noc_start_x,
            //     in1_mcast_dest_noc_start_y,
            //     in1_mcast_dest_noc_end_x,
            //     in1_mcast_dest_noc_end_y,
            //     in1_mcast_receiver_semaphore_addr);
            // noc_semaphore_set_multicast(in1_mcast_receiver_semaphore_addr, in1_mcast_receiver_semaphore_noc_addr, in1_mcast_num_dests);

            // Commented out device-specific code for simulation
            // cb_push_back(cb_id_in1, in1_block_num_tiles);
        }
        if (bcast_B == 0) {
            in1_tensor_start_tile_id += KtNt;
        }
    }
}
