#/bin/bash

# set -eo pipefail

if [[ -z "$TT_METAL_HOME" ]]; then
  echo "Must provide TT_METAL_HOME in environment" 1>&2
  exit 1
fi
fail=0

echo "Running ttnn nightly tests for GS only"

env pytest -n auto tests/ttnn/integration_tests -m "not models_performance_bare_metal and not models_device_performance_bare_metal" ; fail+=$?

if [[ $fail -ne 0 ]]; then
  exit 1
fi
