// SPDX-FileCopyrightText: © 2023 Tenstorrent Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include <iostream>
#include "ttsim.h"
#include <cstdint>
#include "../bmm_op.hpp"

using namespace std;
using uint32_t = std::uint32_t;

// #define OUT_SHARDED 
// #define BACKWARDS 

void writer_unary_interleaved_start_id(
    uint32_t num_tiles,
    uint32_t start_id,
    CoreArgs *core_args
) {
    #define PRINT PRINT_CORE(WRITER, core_args->core_id)
    PRINT << " WRITER writer_unary_interleaved_start_id" << std::endl;
    PRINT << "num_tiles=" << num_tiles << ", start_id=" << start_id << std::endl;
    PRINT_SEP;

    constexpr bool dst_is_dram = true;

    #ifdef OUT_SHARDED
    // cb_wait_front(cb_id_out, num_tiles);
    PRINT_CORE(WRITER, core_args->core_id) << " OUT_SHARED=ON :: Waiting for " << num_tiles << " tiles" << endl;
    return;
    #endif

    // single-tile ublocks
    constexpr uint32_t onetile = 1;

    #ifdef BACKWARDS
    PRINT_CORE(WRITER, core_args->core_id) << " BACKWARDS=ON :: Writing " << num_tiles << " tiles backwards" << endl;
    uint32_t end_id = start_id - num_tiles;
    for (uint32_t i = start_id; i != end_id; -- i) {
    #else
    uint32_t end_id = start_id + num_tiles;
    for (uint32_t i = start_id; i < end_id; ++ i) {
    #endif
        PRINT_LEVEL(0); PRINT << "Write tile " << i << std::endl;
    }
}

