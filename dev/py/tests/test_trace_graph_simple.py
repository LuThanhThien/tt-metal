import torch, ttnn

device_id = 0
device = ttnn.open_device(device_id=device_id)

with ttnn.tracer.trace():
	torch_input_tensor = torch.rand(32, 32, dtype=torch.float32)
	input_tensor = ttnn.from_torch(torch_input_tensor, dtype=ttnn.bfloat16, layout=ttnn.TILE_LAYOUT, device=device)
	output_tensor = ttnn.exp(input_tensor)
	torch_output_tensor = ttnn.to_torch(output_tensor)

ttnn.tracer.visualize(torch_output_tensor, file_name="trace_graph_simple.svg")

ttnn.close_device(device)
