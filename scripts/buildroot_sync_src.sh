#!/bin/bash

update_config_in() {
    local CONFIG_FILE="$CROSSCON_REPO_PATH/buildroot/package/Config.in"
    local SOURCE_LINE='    source "package/mtls/Config.in"'

    # If already included, do nothing
    grep -Fq 'package/mtls/Config.in' "$CONFIG_FILE" && return

    # Insert after 'menu "Miscellaneous"'
    sed -i "/^menu \"Miscellaneous\"\$/a\\
    $SOURCE_LINE
    " "$CONFIG_FILE"

    echo "# Added mtls to Config.in"
}

set -e
source "$(dirname "$0")/common.sh"

echo "# Attempting to sync changes with buildroot..."

echo "# Copying buildroot config..."
cp "$LOCAL_PROJECT_DIR/buildroot/config/crosscon-buildroot.config" "$CROSSCON_REPO_PATH/buildroot/build-aarch64/.config"

echo "# Copying wolfssl config..."
cp "$LOCAL_PROJECT_DIR/buildroot/config/wolfssl.mk" "$CROSSCON_REPO_PATH/buildroot/package/wolfssl/"

echo "# Copying opensc config..."
cp "$LOCAL_PROJECT_DIR/buildroot/config/opensc.mk" "$CROSSCON_REPO_PATH/buildroot/package/opensc/"

echo "# Checking Config.in..."
update_config_in

echo "# Copying app sources..."
rsync -avz --delete "$LOCAL_PROJECT_DIR/." "$CROSSCON_REPO_PATH/buildroot/package/mtls"

echo "# Done!"
