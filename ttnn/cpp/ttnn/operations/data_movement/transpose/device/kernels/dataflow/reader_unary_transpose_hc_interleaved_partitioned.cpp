// SPDX-FileCopyrightText: © 2023 Tenstorrent Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include <stdint.h>
#include "dataflow_api.h"

using uint32_t = std::uint32_t;

// tile index to address
inline uint32_t TADDR_FLOAT32(uint32_t ti) {
    return ti << 12;
}
inline uint32_t TADDR_BFLOAT16(uint32_t ti) {
    return ti << 11;
}

void kernel_main() {
    uint32_t src0_addr = get_arg_val<uint32_t>(0);
    uint32_t WT = get_arg_val<uint32_t>(1);
    uint32_t H = get_arg_val<uint32_t>(2);
    uint32_t CT = get_arg_val<uint32_t>(3);
    uint32_t HW_bytes = get_arg_val<uint32_t>(4);
    uint32_t CHW_bytes = get_arg_val<uint32_t>(5);
    uint32_t start_id  = get_arg_val<uint32_t>(6);
    uint32_t num_tiles = get_arg_val<uint32_t>(7);
    uint32_t batch_addr = get_arg_val<uint32_t>(8);
    uint32_t h = get_arg_val<uint32_t>(9);
    uint32_t htWT = get_arg_val<uint32_t>(10);
    uint32_t ct = get_arg_val<uint32_t>(11);
    uint32_t ctoffs = get_arg_val<uint32_t>(12);
    uint32_t wt = get_arg_val<uint32_t>(13);

    constexpr bool src0_is_dram = get_compile_time_arg_val(0) == 1;
    constexpr uint32_t SUBTILE_LINE_BYTES = get_compile_time_arg_val(1);
    constexpr uint32_t FLOAT32_DTYPE = get_compile_time_arg_val(2);

    constexpr uint32_t onetile = 1;
    constexpr uint32_t cb_id_in0 = 0;

    // DPRINT << "WT " << WT <<ENDL();
    // DPRINT << "H " << H <<ENDL();
    // DPRINT << "CT " << CT <<ENDL();
    // DPRINT << "HW_bytes " << HW_bytes <<ENDL();
    // DPRINT << "CHW_bytes " << CHW_bytes <<ENDL();
    // DPRINT << "start_id " << start_id <<ENDL();
    // DPRINT << "num_tiles " << num_tiles <<ENDL();
    // DPRINT << "batch_addr " << batch_addr <<ENDL();
    // DPRINT << "h " << h <<ENDL();
    // DPRINT << "htWT " << htWT <<ENDL();
    // DPRINT << "ct " << ct <<ENDL();
    // DPRINT << "ctoffs " << ctoffs <<ENDL();
    // DPRINT << "wt " << wt <<ENDL();


    // The basic idea here is to iterate over output tiles (that will be over CT,WT) and H
    // this will generate a linearly incremented output address in the inner loop
    // we then reverse map this linear dest address to src address

    const uint32_t tile_bytes = get_tile_size(cb_id_in0);
    const DataFormat data_format = get_dataformat(cb_id_in0);

    const InterleavedAddrGenFast<src0_is_dram> s0 = {
        .bank_base_address = src0_addr,
        .page_size = tile_bytes,
        .data_format = data_format
    };

    for (uint32_t t = 0; t < num_tiles; t++){
        auto h32 = (h&31);
        // what is the source address for the current tile?
        // c32 = intra-C-tile loop
        // every 32 C's acquire a new output tile address
        //    DPRINT << "8B h=" << h << " ct=" << ct << " wt=" << wt << " W=" << W << " HW2=" << HW2 << ENDL();

        cb_reserve_back(cb_id_in0, onetile);

        uint32_t dest_tr0_l1 = get_write_ptr(cb_id_in0);
        // uint32_t save_dest = dest_tr0_l1;
        uint32_t cSubtileOffs = 0;
        for (uint32_t sub = 0; sub < 4; sub++) {
            uint32_t c16offs = cSubtileOffs;
            for (uint32_t c16 = 0; c16 < 16; c16++) {
                // In this loop sub, c16 are source subtile, c16
                // dest in this loop is varying h implicitly via dest address increment

                // Dest is HCW
                // We are iterating over it as H Ct Wt-tiles
                // intra-tile FC16 for F going over 4-subtiles
                // the source address is (bytes):
                // src_addr = c*HW2 + (ht*Wt + wt)*1024*2 + f*256*2 + (h16*16 + w16)*2
                // we have 512 bytes per subtile and 32 bytes per subtile row of 16 elems
                // here sub<<9 is multiply by 512 which offset in bytes of a subtile
                // note that dest h is decomposed as h = ht+h32 and htWT is incremented by WT in the outer H loop

                // TODO(AP): not really trivial need better comments here
                // auto sub_src_offs = (sub & 1) << 9; // if dest subtile w==16, add 512 to src subtile offset
                uint32_t sub_src_offs;
                uint32_t src_offs;
                uint32_t bsrc_offs;
                uint32_t batch_itile;
                uint32_t rem;

                if constexpr(FLOAT32_DTYPE) {
                    sub_src_offs = (sub & 1) << 10; // if dest subtile w==16, add 1024 to src subtile offset
                    sub_src_offs += (((h32 >> 4) << 1) << 10); // if intra-tile source h is > 16, add 2*1024 to subtile offset
                    // below we only use the lower 4 bits out of 5-bit range for h, shift by 5 because 4 bytes per element
                    src_offs = ctoffs + c16offs + TADDR_FLOAT32(htWT + wt) + sub_src_offs + ((h32&15)<<6); // bytes offset
                    bsrc_offs = batch_addr + src_offs;
                    batch_itile = (bsrc_offs >> 12);
                    rem = (bsrc_offs & 4095);
                } else {

                    sub_src_offs = (sub & 1) << 9; // if dest subtile w==16, add 512 to src subtile offset
                    sub_src_offs += (((h32 >> 4) << 1) << 9); // if intra-tile source h is > 16, add 2*512 to subtile offset
                    // below we only use the lower 4 bits out of 5-bit range for h, shift by 5 because 2 bytes per element
                    src_offs = ctoffs + c16offs + TADDR_BFLOAT16(htWT + wt) + sub_src_offs + ((h32&15)<<5); // bytes offset
                    bsrc_offs = batch_addr + src_offs;
                    batch_itile = (bsrc_offs >> 11);
                    rem = (bsrc_offs & 2047);
                }

                // DPRINT << "rem " << rem <<ENDL();

                //if (h == 0 && ct == 0 && wt == 0) {
                //    DPRINT << "  Sub=" << sub << " c16=" << c16 << ENDL();
                //    DPRINT << "    Reading from src_offs=" << src_offs << ENDL();
                //    DPRINT << "    Writing to   dst_offs=" << dest_tr0_l1-save_dest << ENDL();
                //}

                uint64_t banked_addr = get_noc_addr(batch_itile, s0);
                banked_addr += rem;

                // this starts async NOC dma from DRAM to TR0_L1 buffer
                noc_async_read(banked_addr, dest_tr0_l1, SUBTILE_LINE_BYTES);

                //if (h == 0 && ct == 0 && wt == 0)
                //    DPRINT << uint32_t( reinterpret_cast<uint16_t*>( dest_tr0_l1 )[0] ) << ENDL();

                // the output address is just linearly incremented
                dest_tr0_l1 += SUBTILE_LINE_BYTES;
                c16offs += HW_bytes;
            }
            // subtiles are ordered like this:
            // 0 1
            // 2 3
            // Here we offset C by 16 starting with subtile=2
            if (sub == 1) // after we are done with subtile 1, increment for sub=2
                cSubtileOffs += (HW_bytes<<4); // 16*HWbytes, which is subtile vertical size
        } // sub<4

        // block on all outstanding noc DMA requests to complete
        noc_async_read_barrier();

        // notifies the unpacker that the buffer is populated
        cb_push_back(cb_id_in0, onetile);
        wt++;
        if (wt == WT) { // End of row
            wt = 0;
            ct++;
            ctoffs += (HW_bytes<<5); // since we increment ct, we need to multiply by 32
            if (ct == CT) { // End of column
                ct = 0;
                ctoffs = 0;
                h++;
                if (h == H) { // End of batch
                    batch_addr += CHW_bytes;
                    h = 0;
                    htWT = 0;
                }
                else if (h32 == 31) {
                    htWT += WT;
                }
            }
        }
    }
}
