# SPDX-FileCopyrightText: © 2023 Tenstorrent Inc.

# SPDX-License-Identifier: Apache-2.0

import pytest
from loguru import logger
import torch
import ttnn
import os

import scipy
from sklearn.metrics import top_k_accuracy_score
import numpy as np

from models.demos.t3000.llama2_70b.reference.llama.llama import Llama
from models.demos.tg.llama3_70b.tt.llama_model_galaxy import TtLlamaModel_galaxy

from models.utility_functions import skip_for_grayskull
from models.demos.t3000.llama2_70b.tt.llama_common import (
    setup_llama_env,
    check_device_mesh,
    extract_pcc_from_log,
    MAX_SEQ_LEN,
    MAX_SEQ_LEN_LLAMA3,
    BASE_URL,
    UNIT_TEST_START_POS,
    UNIT_TEST_GENERATION_LENGTH,
    comp_pcc,
    should_skip_model_load,
    check_kv_cache,
    ConcatMesh2DToTensor,
)
import gc


class PytorchLlamaModel(torch.nn.Module):
    def __init__(self, hf_reference_model):
        super().__init__()
        self.model = hf_reference_model

        # Disable dropout
        self.model.eval()

        configuration = hf_reference_model.params
        self.n_heads = configuration.n_heads
        hidden_dim = configuration.dim
        self.head_dim = hidden_dim // self.n_heads
        self.max_seq_len = configuration.max_seq_len

    def forward(self, x, start_pos):
        """
        x: (batch, seq)
        start_pos: int

        return: (batch, seq, hidden_dim)
        """
        return self.model(x, start_pos)


def run_test_LlamaModel_inference(
    device_mesh,
    cluster_shape,
    batch,
    seq_len,
    pcc,
    model_config,
    n_layers,
    llama_version,
    ckpt_dir,
    tokenizer_path,
    cache_path,
    prompt_file=None,
):
    # Load prompt file if provided
    prompt = None
    if prompt_file:
        assert os.path.isfile(prompt_file), "Input file does not exist!"
        with open(prompt_file, "r") as f:
            prompt = f.read()

    # Prepare paths and devices
    skip_model_load = should_skip_model_load()

    logger.info(f"Running num_layer: {n_layers}")
    hugging_face_reference = Llama.build(
        ckpt_dir,
        tokenizer_path,
        max_seq_len=MAX_SEQ_LEN if llama_version == "llama2" else MAX_SEQ_LEN_LLAMA3,
        max_batch_size=batch,
        n_layers=n_layers,
        skip_model_load=skip_model_load,
    )
    hugging_face_reference_model = hugging_face_reference.model
    hugging_face_reference_model.eval()
    state_dict = hugging_face_reference_model.state_dict()
    logger.info(state_dict.keys())
    torch.manual_seed(0)
    configuration = hugging_face_reference_model.params

    # PyTorch model --------------------------------------------------------------------
    pytorch_model = PytorchLlamaModel(hugging_face_reference_model)
    # TT model -------------------------------------------------------------------------
    tt_model = TtLlamaModel_galaxy(
        device_mesh,
        cluster_shape,
        state_dict,
        BASE_URL,
        n_layers,
        model_config,
        configuration,
        cache_path=cache_path,
    )

    if model_config["LLM_MODE"] == "prefill":
        generation_start_pos = 0
        generation_length = 1
    else:
        generation_start_pos = UNIT_TEST_START_POS
        generation_length = UNIT_TEST_GENERATION_LENGTH

    # Pre-process inputs in prompt mode
    if prompt:
        tokenizer = hugging_face_reference.tokenizer
        tokenized = tokenizer.encode(prompt, bos=True, eos=False)
        tokenized = torch.tensor(tokenized).unsqueeze(0)

        # Sliding window across sequence dimension for generation_length iterations
        tokenized = tokenized[:, : (seq_len + generation_length - 1) * batch]
        tokenized = torch.reshape(tokenized, (batch, (seq_len + generation_length - 1)))

        logger.info("Finished converting prompt to tokens.")

    all_tests_pass = True
    all_pccs, all_top1, all_top5 = [], [], []
    for i in range(generation_length):
        # Prepare input
        if prompt:
            pt_inp_ids = tokenized[:, i : i + seq_len]  # Slide window
            assert pt_inp_ids.shape == (batch, seq_len), f"Inputs must have shape {(batch, seq_len)}"
        else:
            pt_inp_ids = torch.randint(0, configuration.vocab_size, (batch, seq_len))
        tt_inp_ids = pt_inp_ids.clone()

        start_pos = generation_start_pos + i

        # PyTorch output --------------------------------------------------------------------
        pytorch_out = pytorch_model(
            pt_inp_ids,
            start_pos,
        )

        # TT hardware execution -------------------------------------------------------------
        tt_inp_emb, start_pos, rot_mat, attn_mask = tt_model.prepare_inputs(tt_inp_ids, start_pos)

        tt_out = tt_model(
            tt_inp_emb,
            rot_mat,
            start_pos,
            attn_mask,
        )
        del tt_inp_emb, rot_mat, attn_mask

        tt_out = ttnn.to_torch(
            tt_out, mesh_composer=ConcatMesh2DToTensor(device_mesh, dims=(1, 3), cluster_shape=cluster_shape)
        )
        tt_out = tt_out[:, 0:1, :, : configuration.vocab_size]
        tt_out = tt_out.permute(2, 1, 0, 3).squeeze()  # [batch, hidden_dim]
        if model_config["LLM_MODE"] == "decode":
            tt_out = tt_out[:batch]
        tt_out = tt_out.float()
        pytorch_out = pytorch_out.squeeze()  # [batch, hidden_dim]

        # check outputs ----------------------------------------------------------------------
        does_pass, output_pcc = comp_pcc(pytorch_out, tt_out, pcc)
        logger.info(f"Output: {output_pcc}")
        all_pccs.append(extract_pcc_from_log(output_pcc))

        kl_divs = scipy.stats.entropy(
            torch.nn.functional.softmax(pytorch_out, dim=-1), torch.nn.functional.softmax(tt_out, dim=-1), axis=-1
        )
        logger.info(f"Mean KL Divergence: {kl_divs.mean()}")

        # Write the code to check top-5 and top-1 accuracy. It should show the
        # percentage where the top-1 prediction in pytorch was in the top-5
        # predictions in tt.
        reference_top1 = np.argmax(pytorch_out, axis=-1)
        top1_acc = top_k_accuracy_score(reference_top1, tt_out, k=1, labels=np.arange(tt_out.shape[-1]))
        top5_acc = top_k_accuracy_score(reference_top1, tt_out, k=5, labels=np.arange(tt_out.shape[-1]))

        all_top1.append(top1_acc)
        all_top5.append(top5_acc)

        logger.info(f"Mean Top-1: {top1_acc}")
        logger.info(f"Mean Top-5: {top5_acc}")

        if does_pass:
            logger.info(f"[start_pos={start_pos}] {llama_version} Model output Passed!")
        else:
            logger.warning(
                f"[start_pos={start_pos}] {llama_version} Model output Failed! PCC value is lower than {pcc}"
            )
            all_tests_pass = False

    logger.info(f"Average PCC over {len(all_pccs)} tokens: {sum(all_pccs) / len(all_pccs)}")
    logger.info(f"Average Top-1 over {len(all_top1)} tokens: {sum(all_top1) / len(all_top1)}")
    logger.info(f"Average Top-5 over {len(all_top5)} tokens: {sum(all_top5) / len(all_top5)}")
    # Check kv cache
    # PyTorch output --------------------------------------------------------------------
    pytorch_layer_present = [
        pytorch_model.model.layers[0]
        .attention.cache_k.clone()
        .permute(0, 2, 1, 3)[:batch, ...],  # [batch, n_kv_heads, seq, head_dim]
        pytorch_model.model.layers[0]
        .attention.cache_v.clone()
        .permute(0, 2, 1, 3)[:batch, ...],  # [batch, n_kv_heads, seq, head_dim]
    ]

    tt_layer_present_all = [ttnn.from_device(lp) for lp in tt_model.layers[0].attention.layer_past]
    tt_layer_present_all = [
        ttnn.to_torch(
            lp, mesh_composer=ConcatMesh2DToTensor(device_mesh, dims=(1, 0), cluster_shape=cluster_shape)
        ).transpose(0, 1)[:batch, ...]
        for lp in tt_layer_present_all
    ]

    cache_test_pass = check_kv_cache(
        pytorch_layer_present,
        tt_layer_present_all,
        generation_start_pos,
        generation_length,
        seq_len,
        model_config["LLM_MODE"] == "prefill",
        pcc,
    )
    all_tests_pass = all_tests_pass and cache_test_pass
    if all_tests_pass:
        logger.info(f"{llama_version} output Passed!")
    else:
        gc.collect()
        logger.warning(f"{llama_version} output Failed!")
        assert all_tests_pass, f"PCC value is lower than {pcc} for some of the outputs. Check Warnings!"


@skip_for_grayskull("Requires eth connected devices to run")
@pytest.mark.parametrize(
    "cluster_shape, device_mesh", [pytest.param((4, 8), (8, 4), id="4x8_grid")], indirect=["device_mesh"]
)
@pytest.mark.parametrize(
    "llama_version",
    (
        # ("llama2"),
        ("llama3"),
    ),
)
@pytest.mark.parametrize(
    "pcc,n_layers",
    [
        (0.995, 1),
        (0.993, 2),
        (0.99, 80),
    ],
    ids=("1L", "2L", "80L"),
)
@pytest.mark.parametrize(
    "batch, seq_len",
    [(32, 1)],
    ids=["decode"],
)
@pytest.mark.parametrize(
    "max_batch_size, max_context_len",
    (
        (32, 2048),
        # (16, 8192),
    ),
    ids=(
        "short_context",
        # "long_context",
    ),
)
@pytest.mark.parametrize(
    "prompt_file",
    ("models/demos/t3000/llama2_70b/demo/data/a_tale_of_two_cities.txt", None),
    ids=("prompt_input", "rand_input"),
)
def test_LlamaModel_inference(
    batch,
    seq_len,
    pcc,
    n_layers,
    device_mesh,
    max_batch_size,
    max_context_len,
    llama_version,
    prompt_file,
    cluster_shape,
    use_program_cache,
):
    if seq_len == 1 and batch != max_batch_size:
        pytest.skip(f"Input batch size should match max_batch_size")

    if batch == 1 and seq_len > max_context_len:
        pytest.skip(f"Prefill with seq_len={seq_len} is not supported with short context")

    if llama_version == "llama2" and seq_len > 2048:
        pytest.skip(f"Llama2 with seq_len={seq_len} is not supported (max 2048)")

    model_config, ckpt_dir, tokenizer_path, cache_path = setup_llama_env(
        llama_version=llama_version,
        batch=batch,
        seq_len=seq_len,
        max_batch_size=max_batch_size,
        max_context_len=max_context_len,
    )

    check_device_mesh(device_mesh, model_config)

    run_test_LlamaModel_inference(
        device_mesh,
        cluster_shape,
        batch,
        seq_len,
        pcc,
        model_config,
        n_layers,
        llama_version,
        ckpt_dir,
        tokenizer_path,
        cache_path,
        prompt_file=prompt_file,
    )
