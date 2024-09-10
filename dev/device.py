import ttnn
import tt_lib as ttl

device_params = {"l1_small_size": 32768, "num_hw_cqs": 2, "trace_region_size": 1332224}
device = ttl.device.CreateDevice(device_id=0, **device_params)
ttl.device.SetDefaultDevice(device)

print("Device created successfully")

ttl.device.DumpDeviceProfiler(device)
ttl.device.Synchronize(device)
ttl.device.CloseDevice(device)
