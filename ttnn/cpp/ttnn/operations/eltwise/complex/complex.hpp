// SPDX-FileCopyrightText: © 2023 Tenstorrent Inc.
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <functional>
#include <array>

#include "ttnn/tensor/tensor.hpp"
#include "ttnn/decorators.hpp"

namespace ttnn {
namespace operations::complex {

class ComplexTensor {
    private:
        std::array<Tensor, 2> m_real_imag;

    public:
        ComplexTensor(std::array<Tensor, 2> val);
        const Tensor& operator[](uint32_t index) const;
        const Tensor& real() const;
        const Tensor& imag() const;
        void deallocate();
};

struct CreateComplexTensor {

    static ComplexTensor operator()(
        const Tensor &input_tensor_a_arg,
        const Tensor &input_tensor_b_arg);
};

}  // namespace operations::complex

using ComplexTensor = operations::complex::ComplexTensor;

constexpr auto complex_tensor = ttnn::register_operation<
    "ttnn::complex_tensor",
    operations::complex::CreateComplexTensor>();


} // namespace ttnn
