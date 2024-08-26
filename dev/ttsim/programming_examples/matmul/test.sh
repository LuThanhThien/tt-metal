#!/bin/bash

# Set the TEST_NAME from the first argument, or default to 'matmul_single' if no argument is provided
TEST_NAME=${1:-matmul_single}

# Shift the argument so that "$@" will contain any additional arguments passed to the script
shift

# Check if the -l or --log option is passed
LOGGING_ENABLED=false
for arg in "$@"; do
    if [[ "$arg" == "-l" || "$arg" == "--log" ]]; then
        LOGGING_ENABLED=true
        break
    fi
done

if [ ! -f ./build/${TEST_NAME} ]; then
    echo "Warning: Executable './build/${TEST_NAME}' not found."
else
    # Run the executable with any additional arguments passed to the script, except the -l or --log argument
    ./build/${TEST_NAME} "${@//-l/}" "${@//--log/}"
    
    # If logging is enabled, log the output to the appropriate folder
    if [ "$LOGGING_ENABLED" = true ]; then
        # LOG_DIR="log/${TEST_NAME}"
        # mkdir -p "$LOG_DIR"
        # LOG_FILE="${LOG_DIR}/$(date +'%Y%m%d_%H%M%S').log"

        LOG_DIR="log"
        mkdir -p "$LOG_DIR"
        LOG_FILE="${LOG_DIR}/${TEST_NAME}.log"
        
        echo "Logging enabled. Output will be saved to '$LOG_FILE'."
        ./build/${TEST_NAME} "$@" &> "$LOG_FILE"
    fi
fi

