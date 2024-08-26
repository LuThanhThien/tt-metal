// SPDX-FileCopyrightText: © 2023 Tenstorrent Inc.
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ttnn/decorators.hpp"

#include "ttnn/cpp/ttnn/deprecated/tt_dnn/op_library/reduce/common.hpp"

namespace ttnn {
namespace operations {
namespace ccl {

struct ExecuteReduceScatter {
    static ttnn::Tensor operator()(
        const ttnn::Tensor& input_tensor,
        const uint32_t scatter_dim,
        tt::tt_metal::ReduceOpMath math_op,
        const uint32_t num_links = 1,
        const std::optional<ttnn::MemoryConfig>& memory_config = std::nullopt);
};

}  // namespace ccl
}  // namespace operations

constexpr auto reduce_scatter =
    ttnn::register_operation<"ttnn::reduce_scatter", ttnn::operations::ccl::ExecuteReduceScatter>();

}  // namespace ttnn
