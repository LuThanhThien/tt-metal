# SPDX-FileCopyrightText: © 2023 Tenstorrent Inc.

# SPDX-License-Identifier: Apache-2.0

from torch import nn


import tt_lib
from models.utility_functions import torch_to_tt_tensor_rm
from models.helper_funcs import Linear as TtLinear
from models.experimental.deit.tt.deit_config import DeiTConfig

class TtDeiTSelfOutput(nn.Module):
    """
    The residual connection is defined in DeiTLayer instead of here (as is the case with other models), due to the
    layernorm applied before each block.
    """

    def __init__(
        self, config: DeiTConfig(), device, state_dict=None, base_address=""
    ) -> None:
        super().__init__()

        dense_weight = torch_to_tt_tensor_rm(
            state_dict[f"{base_address}.dense.weight"], device
        )
        dense_bias = torch_to_tt_tensor_rm(
            state_dict[f"{base_address}.dense.bias"], device
        )
        self.dense = TtLinear(
            config.hidden_size, config.hidden_size, dense_weight, dense_bias
        )

    def forward(
        self, hidden_states: tt_lib.tensor.Tensor, input_tensor: tt_lib.tensor.Tensor
    ) -> tt_lib.tensor.Tensor:
        hidden_states = self.dense(hidden_states)

        return hidden_states
