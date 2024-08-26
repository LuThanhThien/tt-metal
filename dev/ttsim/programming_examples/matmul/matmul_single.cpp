// SPDX-FileCopyrightText: © 2023 Tenstorrent Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include <stdint.h>
#include <iostream>
#include "kernel/dataflow/reader_bmm_8bank.cpp"
#include "kernel/dataflow/writer_bmm_8bank.cpp"
#include "kernel/compute/bmm.cpp"

using namespace std;

void thread_func(
    uint32_t Mt,
    uint32_t Kt,
    uint32_t Nt,
    uint32_t MtKt,
    uint32_t KtNt,
    uint32_t batch,
    uint32_t bcast_B,
    CoreArgs *core_args
) {
    reader_bmm_8bank(batch, Mt, Kt, Nt, MtKt, KtNt, bcast_B, core_args);
    bmm(batch, Mt, Kt, Nt, core_args);
    writer_bmm_8bank(batch, Mt, Nt, core_args);
}

int main() {
    constexpr bool src0_is_dram = true;
    constexpr bool src1_is_dram = true;

    uint32_t src0_addr = 0;
    uint32_t src1_addr = 0;
    uint32_t Mt = 2;
    uint32_t Kt = 2;
    uint32_t Nt = 2;
    uint32_t MtKt = Mt * Kt; // if 0
    uint32_t KtNt = Kt * Nt;
    uint32_t batch = 1;
    uint32_t bcast_B = 0; // if 1 we broadcast B to batch

    PRINT_TITLE("MATMUL_SINGLE_CORE");
    PRINT_HOST << "Mt=" << Mt << " Kt=" << Kt << " Nt=" << Nt << " MtKt=" << MtKt << " KtNt=" << KtNt << ENDL;
    PRINT_HOST << "src0=" << src0_addr << " src1=" << src1_addr << ENDL;
    PRINT_HOST << "batch=" << batch << ENDL;
    PRINT_HOST << ENDL;

    // Call the separated functions
    CoreArgs core_args(-1);
    thread_func(Mt, Kt, Nt, MtKt, KtNt, batch, bcast_B, &core_args);

    core_args.print_stats();

    return 0;
}
