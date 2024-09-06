#!/bin/bash
: '
TLDR: Follow the steps outlined below to build metal. For more in-depth information, keep reading

Steps:
    1. Create python_env (you only need to do this ONCE): ./create_venv.sh
        - The env will by default be created in $TT_METAL_HOME/python_env. If you want to change it, set the PYTHON_ENV_DIR.
        - This step is not dependent on any of the other steps; you only need to run it ONCE and in any order.
    2. Configure and generate build files: `cmake -B build -G Ninja`
        - The `-B` indicates where the build folder is; you can change the folder name to whatever you want.
        - The `-G Ninja` specifies to cmake to use the Ninja build system, which is faster and more reliable than make.
    3. Build metal: `ninja install -C build`
        - The general command would be: `cmake --build build --target install`
        - The `-C` indicates where to run the command; in this case, it will be your build folder(s).
        - We are targeting `install` since that will also just build src.
        - The install target will install pybinding .so (_C.so & _ttnn.so) into the src files, so pybinds can be used.
    4. Build cpp tests: `ninja tests -C build`
        - Building tests will also automatically build src.

Notes:
    - YOU ONLY NEED TO BUILD THE PYTHON_ENV ONCE!!!!! (unless you touch the dev python dependencies)
    - ALWAYS INSTALL, i.e., just run `ninja install -C build` as the new make build.
    - `cmake --build build --target install` is the EXACT same as running `ninja install -C build`. You would use the cmake command if you want to be agnostic of the build system (Ninja or Make).

Different configs: To change build type or use tracy, you have to change the configuration cmake step (step #2).
    - Changing build types: `cmake -B build -DCMAKE_BUILD_TYPE=<type> -G Ninja`
        - Valid CMAKE_BUILD_TYPE values: `Release`, `Debug`, `RelWithDebInfo`, `CI`
        - Release is the default if you do not set CMAKE_BUILD_TYPE.
    - Tracy: `cmake -B build -G Ninja -DENABLE_TRACY=ON`

Now you can have multiple build folders with different configs. If you want to switch, just run `ninja install -C <your_build_folder>` to install different pybinds.
    - Caveats:
        - At least one of these folders has to be named `build`, and if using tracy config, it has to be named `build`.. pending issue #8767.
        - They have to be built with the original build folder name, i.e., you cannot change the build folder name after building because it will mess up the RPATHs and linking.

Example:
    ./create_venv.sh
    cmake -B build -G Ninja && ninja -C build                       # <- Build in Release, inside folder called `build`.
    cmake -DCMAKE_BUILD_TYPE=Debug -B build_debug -G Ninja && ninja -C build_debug    # <- Build in Debug, inside folder called `build_debug`.
    source python_env/bin/activate                                  # <- You cannot run pytests yet since pybinds have not been installed.
    ninja install -C build                                          # <- Install Release pybinds.
    <run a pytest>                                                  # <- This test ran in Release config.
    ninja install -C build_debug                                    # <- Install Debug pybinds.
    <run a pytest>                                                  # <- This test ran in Debug config.

NOTE ON DEBUGGING!:
    GDB/LLDB is not stable right now. Recommend using GCC11 or higher for debugging or Clang-17 with GDB 14+.
'

#!/bin/bash

set -eo pipefail

# Function to display help
show_help() {
    echo "Usage: $0 [-h] [-e] [-c] [-b build_type] [-t] [-w]"
    echo "  -h  Show this help message."
    echo "  -e  Enable CMAKE_EXPORT_COMPILE_COMMANDS."
    echo "  -c  Enable ccache for the build."
    echo "  -b  Set the build type. Default is Release. Other options are Debug, RelWithDebInfo, and CI."
    echo "  -t  Enable build time trace (clang only)."
    echo "  -w  Enable Watcher mode with environment variables set for logging and monitoring."
    echo "  --tp  Enable Tracy profiler (also disable DPRINT)"
}

# Parse CLI options
export_compile_commands="OFF"
enable_ccache="OFF"
enable_time_trace="OFF"
build_type="Release"
enable_watcher="OFF"
enable_profiling="OFF"
enable_tracy_profiler="OFF"

export TTNN_ENABLE_LOGGING=1                 # for DPRINT
export ENABLE_PROFILER=1                     # for DPRINT
export TT_METAL_DPRINT_CORES=all             # for DPRINT 
export TT_METAL_DPRINT_CHIPS=0,1,2,3         # for DPRINT


while [[ $# -gt 0 ]]; do
    case "$1" in
        -h)
            show_help
            return 0
            ;;
        -e)
            export_compile_commands="ON"
            shift
            ;;
        -c)
            enable_ccache="ON"
            shift
            ;;
        -t)
            enable_time_trace="ON"
            shift
            ;;
        -b)
            build_type="$2"
            shift 2
            ;;
        -w)
            enable_watcher="ON"
            shift
            ;;
        --tp)
            enable_tracy_profiler="ON"
            shift
            ;;
        *)
            echo "Illegal option $1"
            show_help
            return 0
            ;;
    esac
done

# Set the python environment directory if not already set
if [ -z "$PYTHON_ENV_DIR" ]; then
    PYTHON_ENV_DIR=$(pwd)/python_env
fi

# Debug output to verify parsed options
echo "Export compile commands: $export_compile_commands"
echo "Enable ccache: $enable_ccache"
echo "Build type: $build_type"
echo "Enable time trace: $enable_time_trace"
echo "Watcher mode: $enable_watcher"

# Handle Watcher Mode
if [ "$enable_watcher" = "ON" ]; then
    export TT_METAL_WATCHER=120        # the number of seconds between Watcher updates
    # export TT_METAL_WATCHER_APPEND=1   # append to the end of the existing log file
    # export TT_METAL_WATCHER_DUMP_ALL=1 # dump all state including unsafe state
    # Watcher features can be disabled individually using the following environment variables:
    # export TT_METAL_WATCHER_DISABLE_ASSERT=1
    export TT_METAL_WATCHER_DISABLE_PAUSE=1
    # export TT_METAL_WATCHER_DISABLE_RING_BUFFER=1
    # export TT_METAL_WATCHER_DISABLE_NOC_SANITIZE=1
    # export TT_METAL_WATCHER_DISABLE_STATUS=1
    # export TT_METAL_WATCHER_DISABLE_STACK_USAGE=1
    # In certain cases enabling watcher can cause the binary to be too large. In this case, disable inlining.
    # export TT_METAL_WATCHER_NOINLINE=1
    echo "Watcher mode enabled with TT_METAL_WATCHER settings."
else
    # Unset the Watcher environment variables if they are not needed
    unset TT_METAL_WATCHER
    unset TT_METAL_WATCHER_APPEND
    unset TT_METAL_WATCHER_DUMP_ALL
    unset WATCHER_ENABLED
    unset TT_METAL_WATCHER_DISABLE_ASSERT
    unset TT_METAL_WATCHER_DISABLE_PAUSE
    unset TT_METAL_WATCHER_DISABLE_RING_BUFFER
    unset TT_METAL_WATCHER_DISABLE_NOC_SANITIZE
    unset TT_METAL_WATCHER_DISABLE_STATUS
    unset TT_METAL_WATCHER_DISABLE_STACK_USAGE
    unset TT_METAL_WATCHER_NOINLINE
    echo "Watcher mode disabled."
fi


# Handle Watcher Mode
if [ "$enable_tracy_profiler" = "ON" ]; then
    unset TTNN_ENABLE_LOGGING             
    unset ENABLE_PROFILER        
    unset TT_METAL_DPRINT_CORES            
    unset TT_METAL_DPRINT_CHIPS     

    echo "DPRINT mode disabled."
    echo "Tracy profiler mode enabled."

    source ./scripts/build_scripts/build_with_profiler_opt.sh
    return 0

else
    # Unset the Watcher environment variables if they are not needed
    unset ENABLE_TRACY

    echo "DPRINT mode enable."
    echo "Tracy Profiler mode disabled."
fi
# Use default build directory name
build_dir="build_$build_type"

# Create and link the build directory
mkdir -p "$build_dir"
ln -nsf "$build_dir" build

# Prepare cmake arguments
cmake_args="-B $build_dir -G Ninja -DCMAKE_BUILD_TYPE=$build_type -DCMAKE_EXPORT_COMPILE_COMMANDS=$export_compile_commands"

if [ "$enable_ccache" = "ON" ]; then
    cmake_args="$cmake_args -DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache"
fi

if [ "$enable_time_trace" = "ON" ]; then
    cmake_args="$cmake_args -DENABLE_BUILD_TIME_TRACE=ON"
fi

# Run cmake and build
cmake $cmake_args
cmake --build "$build_dir" --target install    # <- This is a general cmake way, can also just run `ninja install -C build`

# Build cpp tests
echo "Building cpp tests"
cmake --build "$build_dir" --target tests      # <- Can also just run `ninja tests -C build
