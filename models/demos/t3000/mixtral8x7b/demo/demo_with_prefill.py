# SPDX-FileCopyrightText: © 2023 Tenstorrent Inc.

# SPDX-License-Identifier: Apache-2.0
import os
import torch
import json
import pytest
from loguru import logger
from time import time

import ttnn
from ttnn import ReplicateTensorToMesh, ConcatMeshToTensor
from models.demos.t3000.mixtral8x7b.tt.mixtral_common import (
    load_inputs,
    preprocess_inputs_prefill,
    prepare_inputs_ttnn,
    prepare_inputs_ttnn_prefill,
    get_single_rot_mat,
    sample,
    cache_attention,
    get_rot_transformation_mat,
    get_prefill_rot_mat,
)
from models.demos.t3000.mixtral8x7b.tt.mixtral_model import TtTransformer
from models.demos.t3000.mixtral8x7b.tt.mixtral_embedding import TtMixtralEmbedding
from models.demos.t3000.mixtral8x7b.reference.tokenizer import Tokenizer
from models.demos.t3000.mixtral8x7b.tt.model_config import TtModelArgs

from models.perf.benchmarking_utils import BenchmarkProfiler
from models.demos.utils.llm_demo_utils import create_benchmark_data, verify_perf


class Emb(torch.nn.Module):
    def __init__(self):
        super().__init__()
        self.emb = torch.nn.Embedding(32000, 4096)

    def forward(self, x):
        return self.emb(x)


@torch.no_grad()
def run_mixtral_demo(user_input, batch_size, device_mesh, instruct_mode, is_ci_env):
    if batch_size == 32:
        max_seq_len = 16384
    elif batch_size in [4, 8, 16]:
        max_seq_len = 32768
    else:
        raise ValueError(f"Batch size {batch_size} not supported")

    dtype = ttnn.bfloat8_b

    embed_on_host = True  # Do embedding and argmax on host. TODO Seeing bad output when on device

    # We disregard any warmup iteration for profiling, in favour of just measuring compile time on the first iteration
    N_warmup_iter = {"inference_prefill": 0, "inference_decode": 0}

    # Start profiler
    profiler = BenchmarkProfiler()
    profiler.start("run")

    logger.info(f"Reading inputs...")
    profiler.start("loading_inputs")
    if len(user_input) == 1:
        input_prompts = user_input * batch_size  # Always process 32 users
    else:
        input_prompts = load_inputs(user_input, batch_size)
    profiler.end("loading_inputs")

    # Load model args, weights, and tokenizer
    model_args = TtModelArgs(
        device_mesh.get_device(0), instruct=instruct_mode, max_seq_len=max_seq_len, max_batch_size=batch_size
    )
    tokenizer = Tokenizer(model_args.tokenizer_path)

    model_args.n_layers = 32  # Full model

    logger.info("Loading weights...")
    profiler.start("weight_loading")
    state_dict = torch.load(model_args.state_dict_path)
    # If not using the full model, remove the layers that are not used
    keys_dict = list(state_dict.keys())[:]
    remv = [f"layers.{i}" for i in range(model_args.n_layers, 32)]
    for k in keys_dict:
        if any([r in k for r in remv]):
            state_dict.pop(k)

    # Embedding on host
    if embed_on_host:
        embd = Emb()
        embd.load_state_dict({"emb.weight": state_dict["tok_embeddings.weight"]})

    profiler.end("weight_loading")
    logger.info("Loading weights finished!")

    profiler.start("preprocess_prefill_inputs")
    # Preprocess initial prompt inputs
    (
        input_tokens_prefill_tt,
        input_tokens_decode_tt,
        max_prompt_len,
        input_mask,
        input_tokens_prefill_pt,
        input_tokens_decode_pt,
        input_mask_pt,
        prefill_seq_len,
        encoded_prompts,
    ) = preprocess_inputs_prefill(input_prompts, tokenizer, model_args, dtype, instruct_mode, device_mesh)
    profiler.end("preprocess_prefill_inputs")

    if instruct_mode:
        tokenizer._model.pad_id = tokenizer._model.eos_id

    if not embed_on_host:
        tt_embds = TtMixtralEmbedding(
            device_mesh=device_mesh,
            args=model_args,
            weight_cache_path=model_args.weight_cache_path(dtype),
            state_dict=state_dict,
            dtype=ttnn.bfloat16,  # Row major layout requires bfloat16
        )

    # Prepare the first token embedding for each user
    if embed_on_host:
        pt_decode_input = embd(input_tokens_decode_pt[:, 0]).view(batch_size, 1, -1)
        if prefill_seq_len > 0:
            pt_prefill_input = [
                embd(input_tokens_prefill_pt[b, :]).view(1, prefill_seq_len, -1) for b in range(batch_size)
            ]
    else:  # TODO Embedding on device
        pass

    logger.info("Loading weights to device...")
    profiler.start("loading_weights_to_device")
    tt_model = TtTransformer(
        device_mesh=device_mesh,
        state_dict=state_dict,
        args=model_args,
        layers=list(range(model_args.n_layers)),
        dtype=dtype,
        start_pos=prefill_seq_len,
        rotary_on_host=True,
    )
    profiler.end("loading_weights_to_device")
    logger.info("Finished loading weights to device.")

    profiler.start("prepare_rot_mat_for_decode")
    # Prepare rotary matrix for decode
    current_rot_mat, rot_matrix = get_single_rot_mat(
        model_args.head_dim,
        tt_model.device_mesh,
        model_args.max_seq_len,
    )
    profiler.end("prepare_rot_mat_for_decode")

    generation_start_pos = prefill_seq_len
    max_generated_tokens = 50

    profiler.start("cache_attention")
    cache_attention(
        device_mesh,
        state_dict,
        model_args,
        current_rot_mat,
        rot_matrix,
        generation_start_pos,
        max_generated_tokens,
        dtype,
    )
    profiler.end("cache_attention")

    if prefill_seq_len > 0:
        logger.info(f"Starting prefill [{prefill_seq_len} tokens]...")
        profiler.start("prepare_rot_mat_for_prefill")
        rot_mats_prefill = get_prefill_rot_mat(
            model_args.head_dim, model_args.max_seq_len, device_mesh, seq_len=prefill_seq_len
        )
        head_dim = model_args.dim // model_args.n_heads
        transformation_mat_torch = get_rot_transformation_mat(head_dim)
        transformation_mats = ttnn.as_tensor(
            transformation_mat_torch,
            dtype=ttnn.bfloat16,
            layout=ttnn.TILE_LAYOUT,
            device=device_mesh,
            memory_config=ttnn.DRAM_MEMORY_CONFIG,
            mesh_mapper=ReplicateTensorToMesh(device_mesh),
        )
        profiler.end("prepare_rot_mat_for_prefill")

        num_users_generated_prefill = batch_size
        profiler.start(f"inference_prefill")
        for batch_id in range(batch_size):
            if batch_id == 0:  # First user prefill also accounts for compile time
                profiler.start("compile_prefill")

            prefill_input, attn_mask, _ = prepare_inputs_ttnn_prefill(
                pt_prefill_input[batch_id],
                device_mesh,
            )
            tt_out = tt_model(
                prefill_input,
                0,  # Start position
                0,  # Current position
                attn_mask,
                rot_mats_prefill,
                transformation_mats,
                user_id=batch_id,
                mode="prefill",
            )

            if batch_id == 0:  # First user prefill also accounts for compile time
                profiler.end(f"compile_prefill")

        # Device synchrozization ensures profiler is accurate in end-to-end timing
        for dev in device_mesh.get_devices():
            ttnn.device.synchronize_device(dev)

        profiler.end(f"inference_prefill")
        logger.info(f"Prefill finished [{prefill_seq_len} tokens]!")

    logger.info("Starting decode...")

    # Keep track of generated outputs to print out every iteration
    all_outputs = [encoded_prompts[b][:prefill_seq_len] for b in range(batch_size)]

    # Keep track of users that are done generating and stop printing their outputs
    finished_generation = [False] * batch_size

    num_tokens_generated_decode = max_generated_tokens
    profiler.start("inference_decode")
    # Keep running inference as long as there is a user in the batch still decoding or max tokens per user are decoded
    for iteration in range(max_generated_tokens):
        if iteration == 0:  # First iteration also accounts for compile time
            profiler.start("compile_decode")

        # Check if all users have finished generating (reached EoS token). If so, stop decoding.
        if all(finished_generation):
            logger.info("All users have finished generating tokens")
            break

        iteration_time_start = time()
        start_pos = generation_start_pos + iteration
        current_pos = start_pos

        if embed_on_host:
            profiler.start("prepare_input_decode")
            decode_input_11BH = prepare_inputs_ttnn(
                pt_decode_input,
                model_args.dim,
                start_pos,
                model_args,
                tt_model.device_mesh,
            )
            profiler.end("prepare_input_decode")

        profiler.start("decode_and_argmax")
        # Run ttnn mixtral model
        tt_out_11BH = tt_model(decode_input_11BH, start_pos, current_pos)

        if embed_on_host:
            # Convert ttnn tensor to torch tensor
            tt_output_torch = (
                ttnn.to_torch(tt_out_11BH, mesh_composer=ConcatMeshToTensor(device_mesh, dim=0))[0]
                .squeeze(1)
                .view(32, 1, -1)
                .detach()
                .float()
            )[:batch_size, ...]
            # tt_token_batch = tt_output_torch.squeeze().argmax(axis=-1)
            # Argmax on host to get the new generated tokens
            tt_token_batch = sample(tt_output_torch, temperature=0, top_p=0.8)
            # Update the users that are still in prefill and the ones generating new tokens
            if iteration < max_prompt_len:
                tt_token_batch = torch.where(
                    input_mask_pt[:, iteration], input_tokens_decode_pt[:, iteration], tt_token_batch[:, 0]
                ).unsqueeze(1)
            # Next PT input embedding
            pt_decode_input = embd(tt_token_batch).view(batch_size, 1, -1)
        else:  # Embedding/argmax on device
            # TODO Update argmax to ttnn when OP becomes available
            tt_out_B11B = ttnn.argmax(tt_out_11BH, dim=-1)
            tt_out_1B = ttnn.reshape(tt_out_B11B[:1, :, :, :], ttnn.Shape([1, batch_size]))  # [1, 32] Bfloat16
            # Update the users that are still in prefill and the ones generating new tokens
            if iteration < max_prompt_len:
                decode_input_1B = ttnn.where(input_mask[iteration], input_tokens_decode_tt[iteration], tt_out_1B)
            else:
                decode_input_1B = tt_out_1B

            # Next TT input embeddings
            decode_input_1BH = tt_embds(decode_input_1B)
            decode_input_11BH = ttnn.reshape(decode_input_1BH, ttnn.Shape([1, 1, batch_size, model_args.dim]))
            decode_input_11BH = ttnn.to_layout(decode_input_11BH, layout=ttnn.TILE_LAYOUT)

            # Convert ttnn tensor to torch tensor and print decoded output (from a single device)
            # tt_output_torch = ttnn.to_torch(decode_input_1B).transpose(0, 1)
            tt_token_batch = ttnn.to_torch(decode_input_1B).transpose(0, 1)

        profiler.end("decode_and_argmax")
        # Still measure the iteration time to report perf at every iteration
        iteration_time = time() - iteration_time_start
        tokens_per_second_per_user = 1 / iteration_time

        profiler.start(f"log_printing_{iteration}")
        # Get the generated tokens for each user for printing in the log
        for user in range(batch_size):
            user_tok = int(tt_token_batch[user].item())
            if user_tok == tokenizer.eos_id:  # Stop saving the ouput after hitting the EOS token
                finished_generation[user] = True
            if finished_generation[user] == False:
                all_outputs[user].append(user_tok)

        # Print out generated outputs for each user at the end of every iteration
        if not is_ci_env:  # Avoid printing every iteration in CI
            if len(user_input) == 1:
                logger.info("[User 0] {}".format("".join(tokenizer.decode(all_outputs[0]))))
            else:
                for user in range(batch_size):
                    logger.info("[User {}] {}".format(user, "".join(tokenizer.decode(all_outputs[user]))))

        # Always print iteration perf
        logger.info(
            f"Iteration {iteration}: {1000*iteration_time:.2f}ms @ {tokens_per_second_per_user:.1f} tok/s/user ({batch_size*tokens_per_second_per_user:.1f} tok/s throughput)"
        )
        profiler.end(f"log_printing_{iteration}")

        if iteration == 0:
            profiler.end("compile_decode")

    profiler.end("inference_decode")
    profiler.end("run")

    # In CI only print the final generated output to avoid spamming the logs
    if is_ci_env:
        if len(user_input) == 1:
            logger.info("[User 0] {}".format("".join(tokenizer.decode(all_outputs[0]))))
        else:
            for user in range(batch_size):
                logger.info("[User {}] {}".format(user, "".join(tokenizer.decode(all_outputs[user]))))

        # When running in CI, check the output against the expected output to avoid accuracy regressions
        expected_output = "models/demos/t3000/mixtral8x7b/demo/expected_outputs_prefill_128.json"
        with open(expected_output, "r") as f:
            expected_out = json.load(f)

        for i in range(batch_size):
            user_output = "".join(tokenizer.decode(all_outputs[i]))
            user_expect = expected_out[i]["output_general"]

            assert user_output == user_expect, f"Output for user {i} does not match expected output!"
        logger.info("[CI-Only] Output token validation passed!")

    # Benchmark metrics
    compile_prefill_time = profiler.get_duration("compile_prefill")
    compile_decode_time = profiler.get_duration("compile_decode")
    inference_prefill_time = profiler.get_duration("inference_prefill")
    inference_decode_time = profiler.get_duration("inference_decode")
    log_printing_time = sum(profiler.get_duration(f"log_printing_{i}") for i in range(max_generated_tokens))

    # Correct the inference decode time to remove the time spent on compile (1st iteration) and log_printing (at the end of every iteration)
    inference_decode_time = inference_decode_time - compile_decode_time - log_printing_time
    # Correct the inference prefill time to remove the time spent on compile (1st iteration)
    inference_prefill_time = inference_prefill_time - compile_prefill_time

    # FIXME: Currently our prefill pass does not generate the first token, so we correct the time_to_first to include 1 prefill step + 1 decode step
    prefill_time_to_first = (inference_prefill_time / num_users_generated_prefill) + (
        inference_decode_time / num_tokens_generated_decode
    )

    measurements = {
        # Required measurements
        "compile_prefill": compile_prefill_time,
        "compile_decode": compile_decode_time,
        "inference_prefill": inference_prefill_time,
        "inference_decode": inference_decode_time,
        "prefill_time_to_token": prefill_time_to_first,
        "prefill_t/s": num_users_generated_prefill / inference_prefill_time * prefill_seq_len,  # tokens/s
        "decode_t/s/u": num_tokens_generated_decode / inference_decode_time,  # tokens/s
        "decode_t/s": num_tokens_generated_decode / inference_decode_time * batch_size,  # tokens/s/user
        # Optional measurements
        "loading_inputs": profiler.get_duration("loading_inputs"),
        "weight_loading": profiler.get_duration("weight_loading"),
        "preprocess_prefill_inputs": profiler.get_duration("preprocess_prefill_inputs"),
        "loading_weights_to_device": profiler.get_duration("loading_weights_to_device"),
        "prepare_rot_mat_for_decode": profiler.get_duration("prepare_rot_mat_for_decode"),
        "cache_attention": profiler.get_duration("cache_attention"),
        "prepare_rot_mat_for_prefill": profiler.get_duration("prepare_rot_mat_for_prefill"),
        "prepare_input_decode": profiler.get_duration("prepare_input_decode"),
        "decode_and_argmax": profiler.get_duration("decode_and_argmax"),
        "Total compile time": compile_prefill_time + compile_decode_time,
        "Full demo runtime": profiler.get_duration("run"),
    }

    # Print some of the perf metrics as well
    logger.info("---")
    logger.info(f"Prefill compile time: {round(measurements['compile_prefill'], 4)}s")
    logger.info(f"Decode compile time: {round(measurements['compile_decode'], 4)}s")
    logger.info(f"Prefill inference time per user: {round(measurements['inference_prefill']/(batch_size-1), 4)}s")
    logger.info(
        f"Total Decode inference time ({max_generated_tokens-1} iterations): {round(measurements['inference_decode'], 4)}s"
    )
    logger.info("---")
    logger.info(f"Time to first token: {round(measurements['prefill_time_to_token'], 4) * 1000}ms")
    logger.info(f"Average tokens/sec/user: {round(measurements['decode_t/s/u'], 2)}")

    target_prefill_ts = 5000  # TODO update target
    target_decode_ts = 1056
    decode_tsu = 33
    targets = {"prefill_t/s": target_prefill_ts, "decode_t/s": target_decode_ts, "decode_t/s/u": decode_tsu}

    # TODO move token verification here?
    # if expected_greedy_output_path is not None:
    #     token_check_does_pass, expected_output = check_tokens_match(generated_text, expected_greedy_output_path)
    #     measurements["token_verification"] = float(token_check_does_pass)

    # Save benchmark data for CI dashboard
    if is_ci_env:
        benchmark_data = create_benchmark_data(profiler, measurements, N_warmup_iter, targets)
        benchmark_data.prep_csvs(
            profiler,
            run_type=f"demo_with_prefill",
            ml_model_name="Mixtral8x7B",
            ml_model_type="llm",
            num_layers=model_args.n_layers,
            batch_size=batch_size,
            input_sequence_length=prefill_seq_len,
            output_sequence_length=1,
            # config_params=,
            # precision=,
        )


@pytest.mark.parametrize(
    "input_prompts, instruct_weights",
    [
        ("models/demos/t3000/mixtral8x7b/demo/input_data_prefill_128.json", False),
        ("models/demos/t3000/mixtral8x7b/demo/input_data_questions_prefill_128.json", True),
    ],
    ids=["general_weights", "instruct_weights"],
)
def test_mixtral8x7b_demo(t3k_device_mesh, use_program_cache, input_prompts, instruct_weights, is_ci_env):
    if is_ci_env and instruct_weights == True:
        pytest.skip("CI demo test only runs general weights to reduce CI pipeline load (both are supported)")

    for device in t3k_device_mesh.get_device_ids():
        t3k_device_mesh.get_device(device).enable_async(True)

    return run_mixtral_demo(
        user_input=input_prompts,
        batch_size=32,
        device_mesh=t3k_device_mesh,
        instruct_mode=instruct_weights,
        is_ci_env=is_ci_env,
    )
