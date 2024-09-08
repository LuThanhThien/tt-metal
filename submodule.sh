#!/bin/bash

# Define the location of the third_party directory
THIRD_PARTY_DIR="tt_metal/third_party"
CONFIG_FILE="submodule_config.txt"

# Ensure the third_party directory exists
mkdir -p "$THIRD_PARTY_DIR"

# Read each line from the config file
while IFS=' ' read -r repo_url submodule_name branch_or_commit; do
    # Define the full path for the submodule directory
    submodule_path="$THIRD_PARTY_DIR/$submodule_name"

    echo "Processing submodule: $submodule_name"
    echo "Repo URL: $repo_url"
    echo "Checking out: $branch_or_commit"

    # Add the submodule
    git submodule add "$repo_url" "$submodule_path"

    # Move to the submodule directory
    cd "$submodule_path" || exit

    # Checkout the specific branch or commit
    git checkout "$branch_or_commit"

    # Go back to the main project directory
    cd "$TT_METAL_HOME" || exit

    echo "Submodule $submodule_name setup complete."

done < "$CONFIG_FILE"

echo "All submodules processed."
