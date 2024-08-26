// SPDX-FileCopyrightText: © 2023 Tenstorrent Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include <stdint.h>
#include "ttsim.h"
#include "../bmm_op.hpp"

void reader_bmm_tile_layout_in0_receiver_in1_receiver(
    // in0 in1
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
    
    // in0/in1 common args
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

    constexpr uint32_t cb_id_in0 = 0;
    constexpr uint32_t cb_id_in1 = 1;

    // Commented out device-specific code for simulation
    // volatile tt_l1_ptr uint32_t* in0_mcast_receiver_semaphore_addr_ptr = reinterpret_cast<volatile tt_l1_ptr uint32_t*>(in0_mcast_receiver_semaphore_addr);
    // volatile tt_l1_ptr uint32_t* in1_mcast_receiver_semaphore_addr_ptr = reinterpret_cast<volatile tt_l1_ptr uint32_t*>(in1_mcast_receiver_semaphore_addr);

    for (uint32_t b = 0; b < batch; b++) {
        for(uint32_t block = 0; block < num_blocks; block++) {
            // Operand 0
            // Commented out device-specific code for simulation
            // cb_reserve_back(cb_id_in0, in0_block_num_tiles);

            // Commented out device-specific code for simulation
            // noc_semaphore_set(in0_mcast_receiver_semaphore_addr_ptr, INVALID);

            // Commented out device-specific code for simulation
            // uint64_t in0_mcast_sender_semaphore_noc_addr = get_noc_addr(in0_mcast_sender_noc_x, in0_mcast_sender_noc_y, in0_mcast_sender_semaphore_addr);
            // noc_semaphore_inc(in0_mcast_sender_semaphore_noc_addr, 1);

            // Commented out device-specific code for simulation
            // noc_semaphore_wait(in0_mcast_receiver_semaphore_addr_ptr, VALID);

            // Commented out device-specific code for simulation
            // cb_push_back(cb_id_in0, in0_block_num_tiles);

            // Operand 1
            // Commented out device-specific code for simulation
            // cb_reserve_back(cb_id_in1, in1_block_num_tiles);

            // Commented out device-specific code for simulation
            // noc_semaphore_set(in1_mcast_receiver_semaphore_addr_ptr, INVALID);

            // Commented out device-specific code for simulation
            // uint64_t in1_mcast_sender_semaphore_noc_addr = get_noc_addr(in1_mcast_sender_noc_x, in1_mcast_sender_noc_y, in1_mcast_sender_semaphore_addr);
            // noc_semaphore_inc(in1_mcast_sender_semaphore_noc_addr, 1);

            // Commented out device-specific code for simulation
            // noc_semaphore_wait(in1_mcast_receiver_semaphore_addr_ptr, VALID);

            // Commented out device-specific code for simulation
            // cb_push_back(cb_id_in1, in1_block_num_tiles);
            core_args->neighboard_read(2);
        }
    }
}
