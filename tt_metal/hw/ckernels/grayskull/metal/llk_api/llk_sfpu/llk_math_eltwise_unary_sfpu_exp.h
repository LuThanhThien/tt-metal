// SPDX-FileCopyrightText: © 2023 Tenstorrent Inc.
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "llk_math_eltwise_unary_sfpu_init.h"
#include "llk_math_eltwise_unary_sfpu_params.h"
#include "ckernel_sfpu_exp.h"

namespace ckernel {

// New LLK SFPU APIs

template <bool APPROXIMATE>
inline void llk_math_eltwise_unary_sfpu_exponential(uint dst_index, int vector_mode = VectorMode::RC, int param0 = 0) {

    constexpr bool zero_negative = false;
    llk_math_eltwise_unary_sfpu_params<APPROXIMATE>(
        ckernel::sfpu::calculate_exponential<APPROXIMATE, zero_negative>,
        dst_index,
        vector_mode,
        param0);
}

template <bool APPROXIMATE>
inline void llk_math_eltwise_unary_sfpu_exponential_init() {
    llk_math_eltwise_unary_sfpu_init<APPROXIMATE>(sfpu::exp_init<APPROXIMATE>);
}

}
