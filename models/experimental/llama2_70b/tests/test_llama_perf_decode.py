# SPDX-FileCopyrightText: © 2023 Tenstorrent Inc.

# SPDX-License-Identifier: Apache-2.0

import pytest
import os
from functools import partial
from loguru import logger
from pathlib import Path
import torch
from torch import nn
import tt_lib
import ttnn
from ttnn import ShardTensorToMesh, ReplicateTensorToMesh, ConcatMeshToTensor, ListMeshToTensor

from models.experimental.llama2_70b.reference.llama.llama import Llama

from models.experimental.llama2_70b.tt.llama_model_optimized import TtLlamaModel_optimized
from models.experimental.llama2_70b.tt.llama_common import (
    setup_llama_env,
    check_device_mesh,
    MAX_SEQ_LEN,
    BASE_URL,
    load_llama_state_dict,
    should_skip_model_load,
)
from models.utility_functions import (
    profiler,
    enable_persistent_kernel_cache,
    disable_persistent_kernel_cache,
    disable_compilation_reports,
    skip_for_grayskull,
)
from models.perf.perf_utils import prep_perf_report


def load_prompts_file(tokenizer, prefill_length, generation_length, gap=64):
    with open("models/demos/t3000/llama2_70b/demo/data/a_tale_of_two_cities.txt", encoding="utf-8-sig") as f:
        tokenized = tokenizer.encode(f.read(), bos=True, eos=False)

    token_windows = []
    ground_truth_texts = []
    for i in range(0, len(tokenized) - prefill_length + 1, prefill_length + gap):
        token_windows.append(tokenized[i : i + prefill_length])
        ground_truth_text = tokenizer.decode(tokenized[i : i + generation_length + 1])
        ground_truth_texts.append(ground_truth_text)
        if len(token_windows) == 32:
            return token_windows, ground_truth_texts

    return token_windows, ground_truth_texts


def prepare_next_input(tokenizer, tokens, input_text_mask, cur_pos, next_token):
    # only replace token if prompt has already been generated
    next_token = torch.where(input_text_mask[:, cur_pos], tokens[:, cur_pos], next_token)
    tokens[:, cur_pos] = next_token

    eos_reached = (~input_text_mask[:, cur_pos]) & (next_token == tokenizer.eos_id)
    prev_pos = cur_pos

    return tokens, eos_reached, prev_pos


def intialize_inputs(tokenizer, prompt_tokens, bsz, total_len):
    # pad the model to maximum length
    pad_id = tokenizer.pad_id
    tokens = torch.full((bsz, total_len), pad_id, dtype=torch.long, device="cpu")
    for k, t in enumerate(prompt_tokens):
        tokens[k, : len(t)] = torch.tensor(t, dtype=torch.long, device="cpu")

    input_text_mask = tokens != pad_id  # use prefill token if that token is not masked
    return tokens, input_text_mask


def get_decode_time(profiler, start_token, end_token):
    total_time = 0
    num_tokens = end_token - start_token + 1

    for token in range(start_token, end_token + 1):
        total_time += profiler.get(f"model_run_for_inference_{token}")

    average_time = total_time / num_tokens
    return average_time


# Define a dictionary to hold the profiling ranges for each generation length
profiling_ranges = {32: [(20, 30)], 128: [(20, 30), (116, 126)], 2048: [(20, 30), (116, 126), (2036, 2046)]}


def is_in_profiling_range(cur_pos, generation_length, profiling_ranges):
    if generation_length in profiling_ranges:
        for start, end in profiling_ranges[generation_length]:
            if start <= cur_pos <= end:
                return True
    return False


def calculate_decode_times(profiler, generation_length):
    times = {}
    for start, end in profiling_ranges[generation_length]:
        label = f"decode_time_{end+2}"
        times[label] = get_decode_time(profiler, start, end)
    return times, times[f"decode_time_{generation_length}"]


def run_inference(tt_model, tokenizer, tokens, device_mesh, configuration, total_len, input_text_mask):
    start_pos = 0
    prev_pos = 0
    for cur_pos in range(start_pos + 1, total_len):
        logger.info(f"Generating token: {cur_pos}")

        tt_inp_emb, prev_pos, rot_mat, attn_mask = tt_model.prepare_inputs(tokens[:, prev_pos:cur_pos], prev_pos)

        tt_logits = tt_model(
            tt_inp_emb,
            rot_mat,
            prev_pos,
            attn_mask,
        )

        del tt_inp_emb
        del rot_mat
        del attn_mask
        logits = ttnn.to_torch(tt_logits, device=device_mesh, mesh_composer=ConcatMeshToTensor(device_mesh, dim=3))
        logits = logits[..., : configuration.vocab_size].float()  # [1, batch, vocab_size]
        del tt_logits

        next_token = torch.argmax(logits, dim=-1)
        next_token = next_token.reshape(-1)

        tokens, eos_reached, prev_pos = prepare_next_input(tokenizer, tokens, input_text_mask, cur_pos, next_token)


def run_test_LlamaModel_end_to_end(
    device_mesh,
    batch,
    seq_len,
    model_config,
    n_layers,
    n_devices,
    generation_length,
    expected_compile_time,
    expected_inference_time,
    ckpt_dir,
    tokenizer_path,
    cache_path,
):
    # Prepare paths and devices
    skip_model_load = should_skip_model_load()

    logger.info(f"Running num_layer: {n_layers}")

    generator = Llama.build(
        ckpt_dir,
        tokenizer_path,
        max_seq_len=MAX_SEQ_LEN,
        max_batch_size=batch,
        n_layers=1,
        skip_model_load=skip_model_load,
    )
    hugging_face_reference_model, tokenizer = generator.model, generator.tokenizer
    hugging_face_reference_model.eval()
    # state_dict = hugging_face_reference_model.state_dict()
    state_dict = load_llama_state_dict(ckpt_dir, n_layers=n_layers)
    configuration = hugging_face_reference_model.params

    # Prepare input -----------------------------------------------------------------------
    torch.manual_seed(0)
    total_len = min(MAX_SEQ_LEN, generation_length + 1)
    prefill_ids, ground_truth_texts = load_prompts_file(
        tokenizer, prefill_length=32 if generation_length > 32 else 20, generation_length=generation_length
    )
    tokens, input_text_mask = intialize_inputs(tokenizer, prefill_ids, batch, total_len)
    # Clear global profiler state before starting measurements
    profiler.clear()

    # Set up model -----------------------------------------------------------------------
    logger.info("Moving weights to devices; might take some time...")
    profiler.start("TT_llama_model_setup")
    tt_model = TtLlamaModel_optimized(
        device_mesh,
        state_dict,
        BASE_URL,
        n_layers,
        model_config,
        configuration,
        cache_path=cache_path,
        read_cache=True,
    )

    for i in device_mesh.get_device_ids():
        device = device_mesh.get_device(i)
        tt_lib.device.Synchronize(device)

    profiler.end("TT_llama_model_setup")

    del state_dict

    logger.info("Running 1st run decode stage with compile...")

    profiler.start(f"end_to_end_inference_with_compile")
    run_inference(tt_model, tokenizer, tokens, device_mesh, configuration, total_len, input_text_mask)
    profiler.end(f"end_to_end_inference_with_compile")
    profiler.print()
    compile_and_loop_time = profiler.get("end_to_end_inference_with_compile")
    compile_iter_time = compile_and_loop_time / total_len
    logger.info(f"decode with compile time, single iter latency: {compile_iter_time}")

    profiler.start(f"end_to_end_inference")
    run_inference(tt_model, tokenizer, tokens, device_mesh, configuration, total_len, input_text_mask)
    profiler.end(f"end_to_end_inference")
    profiler.print()
    loop_time = profiler.get("end_to_end_inference")
    iter_time = loop_time / total_len
    logger.info(f"decode cached, single iter latency: {iter_time}")

    comment = f"num_layers={n_layers}L_n_devices={n_devices}"

    prep_perf_report(
        model_name=f"llama2_70b_{comment}",
        batch_size=batch,
        inference_and_compile_time=compile_iter_time,
        inference_time=iter_time,
        expected_compile_time=expected_compile_time,
        expected_inference_time=expected_inference_time,
        comments=comment,
    )

    tokens_per_s_per_user = 1 / iter_time
    tokens_per_s_overall = tokens_per_s_per_user * batch

    logger.info(f"Time per iteration: {iter_time}")
    logger.info(f"Tokens per s per user: {tokens_per_s_per_user}")
    logger.info(f"Tokens per s overall: {tokens_per_s_overall}")

    # assert compile_time <= expected_compile_time
    assert iter_time <= expected_inference_time


@skip_for_grayskull("Requires eth connected devices to run")
@pytest.mark.model_perf_t3000
@pytest.mark.parametrize(
    "generation_length, expected_compile_time, expected_inference_time",
    (
        (32, 10000, 0.139 + 0.02),  # TODO: decrease expected compile time once as_tensor gets speedup
        (128, 10000, 0.138 + 0.02),  # Fudge delta
        (2048, 10000, 0.153 + 0.02),
    ),
    ids=["gen32", "gen128", "gen2048"],
)
def test_Llama_perf_host(
    generation_length,
    expected_compile_time,
    expected_inference_time,
    t3k_device_mesh,
    use_program_cache,
    n_layers=80,
    n_devices=8,
):
    if generation_length == 2048:
        pytest.skip("Skipping 2048 test for now. segfault issue #8637")

    batch, seq_len = 32, 1

    model_config, ckpt_dir, tokenizer_path, cache_path = setup_llama_env(
        batch=batch,
        seq_len=seq_len,
    )

    check_device_mesh(t3k_device_mesh, model_config)

    # for i in t3k_device_mesh.get_device_ids():
    #     device = t3k_device_mesh.get_device(i)
    #     device.enable_async(True)

    disable_compilation_reports()

    run_test_LlamaModel_end_to_end(
        t3k_device_mesh,
        batch,
        seq_len,
        model_config,
        n_layers,
        n_devices,
        generation_length,
        expected_compile_time,
        expected_inference_time,
        ckpt_dir,
        tokenizer_path,
        cache_path,
    )
