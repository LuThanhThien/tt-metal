#include <stdint.h>
#include <iostream>
#include "ttsim.h"
#include "../bmm_op.hpp"

// Function to handle the "writer" logic
void writer_bmm_8bank(
    uint32_t batch, 
    uint32_t Mt, 
    uint32_t Nt, 
    CoreArgs *core_args
) {
    int core_id = core_args->core_id;
    #define PRINT PRINT_CORE(WRITER, core_id)
    uint32_t dst_addr = 0;
    uint32_t itileC = 0;

    PRINT << "WRITER writer_bmm_8bank.cpp" << std::endl;
    // C is MN so we iterate in tile RM order
    for (uint32_t nb = 0; nb < batch; nb++)
    for (uint32_t mt_C = 0; mt_C < Mt; ++mt_C)   // output tile of C
    for (uint32_t nt_C = 0; nt_C < Nt; ++nt_C) { // output tile index of C
        PRINT_LEVEL(2); PRINT << 'W' << 'C' << itileC << ' ' << 'a' << dst_addr << ' ' << itileC << std::endl;
        itileC++;
    }
}
