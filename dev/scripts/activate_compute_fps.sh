
#!/bin/bash

# Ensure TT_METAL_HOME variable is set
if [ -z "$TT_METAL_HOME" ]; then
    echo "Error: TT_METAL_HOME variable is not set."
    return 0
fi

# Ensure TT_METAL_HOME exists
if [ ! -d "$TT_METAL_HOME" ]; then
    echo "Error: TT_METAL_HOME directory does not exist."
    return 0
fi

SRC_TT_METAL_HOME="/home/thienluu/tt/tt-metal"
TARGET_FILE="$TT_METAL_HOME/tt_metal/tools/profiler/process_ops_logs.py"
TARGET_FUNC="generate_reports"

# Create directory $TT_METAL_HOME/dev/py/report if not exists
echo "Make directory "$PY_DEV_DIR/report" if not exists"
mkdir -p "$TT_METAL_HOME/dev/py/report"

# copying neccessary files
cp "$SRC_TT_METAL_HOME/dev/py/report/compute_fps.py" "$TT_METAL_HOME/dev/py/report/compute_fps.py"
cp "$SRC_TT_METAL_HOME/dev/py/*.py" "$TT_METAL_HOME/dev/py"

# Ensure process_ops_logs.py exists
if [ ! -f $TARGET_FILE ]; then
    echo "Error: process_ops_logs.py file does not exist."
    return 0
fi

# Ensure $TARGET_FUNC function exists
if ! grep -q "def $TARGET_FUNC" $TARGET_FILE; then
    echo "Error: $TARGET_FUNC function not found in process_ops_logs.py."
    return 0
fi

# Check if the import statement already exists
if grep -q "from dev.report.compute_fps import compute_fps" $TARGET_FILE; then
    echo "Error: Import statement already exists in process_ops_logs.py."
    return 0
fi

# Check if the function call already exists
if grep -q "compute_fps(allOpsCSVPath, outFolder)" $TARGET_FILE; then
    echo "Error: Function call already exists in process_ops_logs.py."
    return 0
fi

# Append lines to the $TARGET_FUNC function
line_number=519     # hard-coded line number
new_content="    from dev.report.compute_fps import compute_fps\n    compute_fps(allOpsCSVPath, outFolder)"

awk -v n=$line_number -v content="$new_content" 'NR==n {print; print content} NR!=n' "$TT_METAL_HOME/tt_metal/tools/profiler/process_ops_logs.py" > tmp_file && mv tmp_file "$TT_METAL_HOME/tt_metal/tools/profiler/process_ops_logs.py"

echo "Script execution completed successfully."
echo "Test with the following command:"
echo "./tt_metal/tools/profiler/profile_this.py -n resnet -c "pytest models/demos/resnet/tests/test_metal_resnet50_performant.py::test_run_resnet50_inference[False-LoFi-activations_BFLOAT8_B-weights_BFLOAT8_B-batch_20-device_params0]"
