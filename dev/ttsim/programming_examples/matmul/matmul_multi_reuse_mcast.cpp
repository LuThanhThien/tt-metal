// SPDX-FileCopyrightText: © 2023 Tenstorrent Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include <iostream>
#include <thread>
#include "kernel/dataflow/reader_bmm_tile_layout_in0_receiver_in1_receiver.cpp"
#include "kernel/dataflow/reader_bmm_tile_layout_in0_receiver_in1_sender.cpp"
#include "kernel/dataflow/reader_bmm_tile_layout_in0_sender_in1_receiver.cpp"
#include "kernel/dataflow/reader_bmm_tile_layout_in0_sender_in1_sender.cpp"
#include "kernel/dataflow/writer_bmm_tile_layout.cpp"
#include "kernel/compute/bmm_large_block_zm.cpp"
#include "kernel/bmm_op.hpp"
#include "tt_metal/common/constants.hpp"
#include "ttsim.h"

using namespace std;
using namespace tt::constants;

CoreArgs assign_shared_core_args(vector<CoreArgs> &shared_core_args, string name) {
    for (int i = 0; i < shared_core_args.size(); i++) {
        if (shared_core_args[i].is_name(name)) {
            return shared_core_args[i];
        }
    }
    CoreArgs undefined_core_args;
    undefined_core_args.set_name("undefined");
    return undefined_core_args;
}

void thread_func(
    // in0 in1
    uint32_t in0_buffer_start_tile_id,
    uint32_t in0_buffer_stride_w,
    uint32_t in0_buffer_stride_h,
    uint32_t in0_buffer_next_block_stride,
    uint32_t in0_block_w,
    uint32_t in0_block_h,
    uint32_t in0_block_num_tiles,

    uint32_t in0_num_subblocks, // in0_num_subblocks
    uint32_t in0_out_block_num_tiles, // in0_block_num_tiles for compute
    uint32_t in0_subblock_num_tiles, // in0_subblock_num_tiles

    uint32_t in1_buffer_start_tile_id,
    uint32_t in1_buffer_stride_w,
    uint32_t in1_buffer_stride_h,
    uint32_t in1_buffer_next_block_stride,
    uint32_t in1_block_w,
    uint32_t in1_block_h,
    uint32_t in1_block_num_tiles,

    uint32_t in1_num_subblocks, // in1_num_subblocks
    uint32_t in1_out_block_num_tiles, // in1_block_num_tiles for compute
    uint32_t in1_subblock_num_tiles, // in1_subblock_num_tiles
    
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

    // bmm_large_block_zm
    uint32_t MtKt,
    uint32_t KtNt,
    uint32_t MtNt,
    uint32_t batch,
    uint32_t bcast_B,

    // out
    uint32_t out_buffer_start_tile_id,
    uint32_t out_buffer_stride_w,
    uint32_t out_buffer_stride_h,
    uint32_t out_buffer_next_subblock_stride_w,
    uint32_t out_buffer_next_subblock_stride_h,
    uint32_t out_subblock_tile_count,
    uint32_t out_subblock_h,
    uint32_t out_subblock_w,
    uint32_t out_num_subblocks_w,
    uint32_t out_num_subblocks_h,
    uint32_t out_subblock_num_tiles,

    // identity
    bool is_in0_sender,
    bool is_in1_sender,
    CoreArgs *core_args
) {
    
    // reader
    if (is_in0_sender && is_in1_sender) 
    {
        reader_bmm_tile_layout_in0_sender_in1_sender(
            in0_buffer_start_tile_id, in0_buffer_stride_w, in0_buffer_stride_h, in0_buffer_next_block_stride, 
            in0_block_w, in0_block_h, in0_block_num_tiles, 
            in1_buffer_start_tile_id, in1_buffer_stride_w, in1_buffer_stride_h, in1_buffer_next_block_stride, 
            in1_block_w, in1_block_h, in1_block_num_tiles,
            num_blocks, 
            in0_mcast_dest_noc_start_x, in0_mcast_dest_noc_start_y, in0_mcast_dest_noc_end_x, in0_mcast_dest_noc_end_y, 
            in0_mcast_num_dests, in0_mcast_sender_noc_x, in0_mcast_sender_noc_y,
            in1_mcast_dest_noc_start_x, in1_mcast_dest_noc_start_y, in1_mcast_dest_noc_end_x, in1_mcast_dest_noc_end_y, 
            in1_mcast_num_dests, in1_mcast_sender_noc_x, in1_mcast_sender_noc_y,
            MtKt, KtNt, batch, bcast_B, core_args);
    }
    else if (is_in0_sender && !is_in1_sender) 
    {
        reader_bmm_tile_layout_in0_sender_in1_receiver(
            in0_buffer_start_tile_id, in0_buffer_stride_w, in0_buffer_stride_h, in0_buffer_next_block_stride, 
            in0_block_w, in0_block_h, in0_block_num_tiles, 
            in1_buffer_start_tile_id, in1_buffer_stride_w, in1_buffer_stride_h, in1_buffer_next_block_stride, 
            in1_block_w, in1_block_h, in1_block_num_tiles,
            num_blocks, 
            in0_mcast_dest_noc_start_x, in0_mcast_dest_noc_start_y, in0_mcast_dest_noc_end_x, in0_mcast_dest_noc_end_y, 
            in0_mcast_num_dests, in0_mcast_sender_noc_x, in0_mcast_sender_noc_y,
            in1_mcast_dest_noc_start_x, in1_mcast_dest_noc_start_y, in1_mcast_dest_noc_end_x, in1_mcast_dest_noc_end_y, 
            in1_mcast_num_dests, in1_mcast_sender_noc_x, in1_mcast_sender_noc_y,
            MtKt, KtNt, batch, bcast_B, core_args);
    } 
    else if (!is_in0_sender && is_in1_sender)
    {
        reader_bmm_tile_layout_in0_receiver_in1_sender(
            in0_buffer_start_tile_id, in0_buffer_stride_w, in0_buffer_stride_h, in0_buffer_next_block_stride, 
            in0_block_w, in0_block_h, in0_block_num_tiles, 
            in1_buffer_start_tile_id, in1_buffer_stride_w, in1_buffer_stride_h, in1_buffer_next_block_stride, 
            in1_block_w, in1_block_h, in1_block_num_tiles,
            num_blocks, 
            in0_mcast_dest_noc_start_x, in0_mcast_dest_noc_start_y, in0_mcast_dest_noc_end_x, in0_mcast_dest_noc_end_y, 
            in0_mcast_num_dests, in0_mcast_sender_noc_x, in0_mcast_sender_noc_y,
            in1_mcast_dest_noc_start_x, in1_mcast_dest_noc_start_y, in1_mcast_dest_noc_end_x, in1_mcast_dest_noc_end_y, 
            in1_mcast_num_dests, in1_mcast_sender_noc_x, in1_mcast_sender_noc_y,
            MtKt, KtNt, batch, bcast_B, core_args); 
    }
    else 
    {
        reader_bmm_tile_layout_in0_receiver_in1_receiver(
            in0_buffer_start_tile_id, in0_buffer_stride_w, in0_buffer_stride_h, in0_buffer_next_block_stride, 
            in0_block_w, in0_block_h, in0_block_num_tiles, 
            in1_buffer_start_tile_id, in1_buffer_stride_w, in1_buffer_stride_h, in1_buffer_next_block_stride, 
            in1_block_w, in1_block_h, in1_block_num_tiles,
            num_blocks, 
            in0_mcast_dest_noc_start_x, in0_mcast_dest_noc_start_y, in0_mcast_dest_noc_end_x, in0_mcast_dest_noc_end_y, 
            in0_mcast_num_dests, in0_mcast_sender_noc_x, in0_mcast_sender_noc_y,
            in1_mcast_dest_noc_start_x, in1_mcast_dest_noc_start_y, in1_mcast_dest_noc_end_x, in1_mcast_dest_noc_end_y, 
            in1_mcast_num_dests, in1_mcast_sender_noc_x, in1_mcast_sender_noc_y,
            MtKt, KtNt, batch, bcast_B, core_args);
    }
    
    // compute
    bmm_large_block_zm(
        in0_block_w, 
        in0_num_subblocks, in0_out_block_num_tiles, in0_subblock_num_tiles,
        in1_num_subblocks, in1_out_block_num_tiles, in1_subblock_num_tiles,
        num_blocks,
        out_subblock_h, out_subblock_w, out_subblock_num_tiles, batch, core_args);

    // writer
    writer_bmm_tile_layout(
        out_buffer_start_tile_id, 
        out_buffer_stride_w, out_buffer_stride_h, 
        out_buffer_next_subblock_stride_w, out_buffer_next_subblock_stride_h,
        out_subblock_h, out_subblock_w, 
        out_subblock_tile_count,
        out_num_subblocks_w, out_num_subblocks_h,
        MtNt, batch, core_args);
}

void print_all_stats(vector<CoreArgs> &shared_core_args) {
    int num_dram_read_all = 0;
    int num_neighbour_read_all = 0;
    int num_unit_compute_all = 0;

    // print starts
    for (int i = 0; i < shared_core_args.size(); i++) {
        // PRINT_HOST << shared_core_args[i].str() << ENDL;
        // shared_core_args[i].print_stats();
        // PRINT_SEP; PRINT_LINE;

        num_dram_read_all += *shared_core_args[i].num_dram_read;
        num_neighbour_read_all += *shared_core_args[i].num_neighbour_read;
        num_unit_compute_all += *shared_core_args[i].num_unit_compute;
    }

    float total_read = num_dram_read_all * DRAM_READ_COST + num_neighbour_read_all * NEIGHBOUR_READ_COST;
    float algth_ratio = num_unit_compute_all / total_read;

    PRINT_HOST << "Total Stats" << ENDL;
    PRINT_HOST << "num_dram_read_all=" << num_dram_read_all << ENDL;
    PRINT_HOST << "num_neighbour_read_all=" << num_neighbour_read_all << ENDL;
    PRINT_HOST << "num_unit_compute_all=" << num_unit_compute_all << ENDL;
    PRINT_HOST << "total_read=" << total_read << ENDL;
    PRINT_HOST << "total_compute=" << num_unit_compute_all << ENDL;
    PRINT_HOST << "algth_ratio=" << algth_ratio << ENDL;
    PRINT_SEP; PRINT_LINE;

}

int main() {
    constexpr bool src0_is_dram = true;
    constexpr bool src1_is_dram = true;

    constexpr uint32_t M = 32 * 32;        // user-defined
    constexpr uint32_t N = 32 * 32;        // user-defined
    constexpr uint32_t K = 32 * 32;        // user-defined
    constexpr uint32_t B = 1;           // user-defined
    constexpr uint32_t bcast_B = 1;     // user-defined
    constexpr uint32_t num_cores_x = 2; // user-defined
    constexpr uint32_t num_cores_y = 2; // user-defined

    uint32_t Mt = M / TILE_HEIGHT;
    uint32_t Kt = K / TILE_WIDTH;
    uint32_t Nt = N / TILE_WIDTH;

    uint32_t MtKt = Mt * Kt;
    uint32_t KtNt = Kt * Nt;
    uint32_t MtNt = Mt * Nt;
    uint32_t in0_block_w = 2;

    
    auto matmul_params = bmm_op_utils::get_large_matmul_params(Mt, Nt, num_cores_y, num_cores_x, in0_block_w);
    uint32_t per_core_M = std::get<0>(matmul_params);
    uint32_t per_core_N = std::get<1>(matmul_params);
    uint32_t out_subblock_h = std::get<2>(matmul_params);
    uint32_t out_subblock_w = std::get<3>(matmul_params);
    
    // Compute kernel compile time args
    uint32_t in0_num_subblocks = (per_core_M/out_subblock_h);
    uint32_t in0_out_block_num_tiles = out_subblock_h*in0_block_w*in0_num_subblocks;
    uint32_t in0_subblock_num_tiles = out_subblock_h * in0_block_w;

    uint32_t in1_num_subblocks = (per_core_N/out_subblock_w);
    uint32_t in1_out_block_num_tiles = out_subblock_w*in0_block_w*in1_num_subblocks;
    uint32_t in1_per_core_w = out_subblock_w * in1_num_subblocks;

    uint32_t out_buffer_stride_w = 1;
    uint32_t out_buffer_stride_h = Nt;
    uint32_t out_buffer_next_subblock_stride_w = out_subblock_w;
    uint32_t out_buffer_next_subblock_stride_h = out_subblock_h * Nt;
    uint32_t out_subblock_tile_count = out_subblock_w * out_subblock_h;
    uint32_t out_num_subblocks_w = per_core_N / out_subblock_w;
    uint32_t out_num_subblocks_h = per_core_M / out_subblock_h;
    uint32_t out_subblock_num_tiles = out_subblock_h*out_subblock_w;


    /*
    * Multi-Core prep
    */
    uint32_t num_blocks_y = Mt / per_core_M;
    uint32_t num_blocks_x = Nt / per_core_N;
    uint32_t num_blocks_total =  num_blocks_y * num_blocks_x;

    CoreCoord start_core = {0, 0};
    CoreCoord core_range = bmm_op_utils::get_core_range(num_blocks_y, num_blocks_x, num_cores_y, num_cores_x);
    
    uint32_t start_core_x = start_core.x;
    uint32_t start_core_y = start_core.y;
    uint32_t num_cores_c = core_range.x;
    uint32_t num_cores_r = core_range.y;


    // Set values for the parameters used in your functions
    uint32_t in0_buffer_stride_w = 1;
    uint32_t in0_buffer_stride_h = Kt;
    uint32_t in0_buffer_next_block_stride = in0_block_w;
    uint32_t in0_block_h = per_core_M;
    uint32_t in0_block_num_tiles = in0_block_w * per_core_M;
    uint32_t in1_buffer_stride_w = 1;
    uint32_t in1_buffer_stride_h = Nt;
    uint32_t in1_buffer_next_block_stride = in0_block_w * Nt;
    uint32_t in1_block_w = per_core_N;
    uint32_t in1_block_h = in0_block_w;
    uint32_t in1_block_num_tiles = per_core_N * in0_block_w;
    uint32_t num_blocks = Kt / in0_block_w;


    PRINT_TITLE("MATMUL_MULTI_CORE");
    PRINT_HOST << "Mt=" << Mt << "; Kt=" << Kt << "; Nt=" << Nt << "; MtKt=" << MtKt << "; KtNt=" << KtNt << ENDL;
    PRINT_HOST << "batch=" << B << ENDL;
    PRINT_HOST << "per_core_M=" << per_core_M << "; per_core_N=" << per_core_N << ENDL;
    PRINT_HOST << "out_subblock_h=" << out_subblock_h << "; out_subblock_w=" << out_subblock_w << ENDL;
    PRINT_HOST << "num_cores_x=" << num_cores_x << "; num_cores_y=" << num_cores_y << ENDL;
    PRINT_HOST << "num_cores_r=" << num_cores_r << "; num_cores_c=" << num_cores_c << ENDL;
    PRINT_HOST << "num_blocks_y=" << num_blocks_y << "; num_blocks_x=" << num_blocks_x << "; num_blocks_total=" << num_blocks_total << ENDL;
    PRINT_HOST << "core_range" << core_range.str() << ENDL;
    PRINT_LINE;

    CoreRange in0_sender_in1_sender(
        {start_core_x, start_core_y}, {start_core_x, start_core_y});

    CoreRange in0_sender_in1_receiver(
        {start_core_x, start_core_y + 1},
        {start_core_x, start_core_y + num_cores_r - 1});

    CoreRange in0_receiver_in1_sender(
        {start_core_x + 1, start_core_y},
        {start_core_x + num_cores_c - 1, start_core_y});

    CoreRange in0_receiver_in1_receiver(
        {start_core_x + 1, start_core_y + 1},
        {start_core_x + num_cores_c - 1, start_core_y + num_cores_r - 1});

    // CoreArgs for core stats
    int num_cores_types = 4;
    std::array<string, 4> names = {
        "in0_sender_in1_sender",
        "in0_sender_in1_receiver",
        "in0_receiver_in1_sender",
        "in0_receiver_in1_receiver",
    };
    std::vector<CoreArgs> shared_core_args(num_cores_types);
    for (int i = 0; i < num_cores_types; i++) {
        shared_core_args[i].set_name(names[i]);
    }
    std::vector<CoreArgs> core_args(num_cores_r * num_cores_c);

    for(uint32_t core_idx_y = 0; core_idx_y < num_cores_r; core_idx_y++) {
        for(uint32_t core_idx_x = 0; core_idx_x < num_cores_c; core_idx_x++) {
            CoreCoord core = {start_core_x + core_idx_x, start_core_y + core_idx_y};

            CoreCoord left_core             = {start_core_x, core.y};
            CoreCoord left_core_plus_one    = {start_core_x + 1, core.y};
            CoreCoord right_core            = {start_core_x + num_cores_c - 1, core.y};
            CoreCoord top_core              = {core.x, start_core_y};
            CoreCoord top_core_plus_one     = {core.x, start_core_y + 1};
            CoreCoord bottom_core           = {core.x, start_core_y + num_cores_r - 1};

            uint32_t in0_buffer_start_tile_id = Kt * per_core_M * core_idx_y;
            uint32_t in1_buffer_start_tile_id = per_core_N * core_idx_x;
            uint32_t out_buffer_start_tile_id = core_idx_x * per_core_N + core_idx_y * per_core_M * Nt;
            
            uint32_t in0_mcast_dest_noc_start_x = right_core.x;
            uint32_t in0_mcast_dest_noc_start_y = right_core.y;
            uint32_t in0_mcast_dest_noc_end_x = left_core_plus_one.x;
            uint32_t in0_mcast_dest_noc_end_y = left_core_plus_one.y;
            uint32_t in0_mcast_num_dests = num_cores_c - 1;
            uint32_t in0_mcast_sender_noc_x = left_core.x;
            uint32_t in0_mcast_sender_noc_y = left_core.y;

            uint32_t in1_mcast_dest_noc_start_x = bottom_core.x;
            uint32_t in1_mcast_dest_noc_start_y = bottom_core.y;
            uint32_t in1_mcast_dest_noc_end_x = top_core_plus_one.x;
            uint32_t in1_mcast_dest_noc_end_y = top_core_plus_one.y;
            uint32_t in1_mcast_num_dests = num_cores_r - 1;
            uint32_t in1_mcast_sender_noc_x = top_core.x;
            uint32_t in1_mcast_sender_noc_y = top_core.y;
            
            bool is_in0_sender = in0_sender_in1_sender.contains({core_idx_x, core_idx_y}) || in0_sender_in1_receiver.contains({core_idx_x, core_idx_y});
            bool is_in1_sender = in0_sender_in1_sender.contains({core_idx_x, core_idx_y}) || in0_receiver_in1_sender.contains({core_idx_x, core_idx_y});
            int core_id = core_idx_y * num_cores_c + core_idx_x;
            
            if (is_in0_sender && is_in1_sender) {
                core_args[core_id] = assign_shared_core_args(shared_core_args, "in0_sender_in1_sender");
            } else if (is_in0_sender && !is_in1_sender) {
                core_args[core_id] = assign_shared_core_args(shared_core_args, "in0_sender_in1_receiver");
            } else if (!is_in0_sender && is_in1_sender) {
                core_args[core_id] = assign_shared_core_args(shared_core_args, "in0_receiver_in1_sender");
            } else {
                core_args[core_id] = assign_shared_core_args(shared_core_args, "in0_receiver_in1_receiver");
            }
            core_args[core_id].set_core_id(core_id);


            thread_func(
                in0_buffer_start_tile_id, 
                in0_buffer_stride_w, 
                in0_buffer_stride_h, 
                in0_buffer_next_block_stride,
                in0_block_w,
                in0_block_h,
                in0_block_num_tiles,

                in0_num_subblocks, // in0_num_subblocks
                in0_out_block_num_tiles, // in0_block_num_tiles for compute
                in0_subblock_num_tiles, // in0_subblock_num_tiles

                in1_buffer_start_tile_id,
                in1_buffer_stride_w,
                in1_buffer_stride_h,
                in1_buffer_next_block_stride,
                in1_block_w,
                in1_block_h,
                in1_block_num_tiles,
                
                in1_num_subblocks, // in1_num_subblocks
                in1_out_block_num_tiles, // in1_block_num_tiles for compute
                in1_per_core_w, // in1_per_core_w
                
                num_blocks,

                in0_mcast_dest_noc_start_x,
                in0_mcast_dest_noc_start_y,
                in0_mcast_dest_noc_end_x,
                in0_mcast_dest_noc_end_y,
                in0_mcast_num_dests,
                in0_mcast_sender_noc_x,
                in0_mcast_sender_noc_y,

                in1_mcast_dest_noc_start_x,
                in1_mcast_dest_noc_start_y,
                in1_mcast_dest_noc_end_x,
                in1_mcast_dest_noc_end_y,
                in1_mcast_num_dests,
                in1_mcast_sender_noc_x,
                in1_mcast_sender_noc_y,
    
                MtKt,
                KtNt,
                MtNt,
                B,
                bcast_B,

                out_buffer_start_tile_id,
                out_buffer_stride_w,
                out_buffer_stride_h,
                out_buffer_next_subblock_stride_w,
                out_buffer_next_subblock_stride_h,
                out_subblock_tile_count,
                out_subblock_h,
                out_subblock_w,
                out_num_subblocks_w,
                out_num_subblocks_h,
                out_subblock_num_tiles,

                is_in0_sender,
                is_in1_sender,
                &core_args[core_id]
            );
        }
    }

    print_all_stats(shared_core_args);

    return 0;
}
