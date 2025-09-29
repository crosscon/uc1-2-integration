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
cp "$BUILDROOT_CONFIG_LOCAL_PATH" "$BUILDROOT_CONFIG_TARGET_PATH"

echo "# Adjusting buildroot config for DEMO=$DEMO"
case "$DEMO" in
  "RPI_CBA")
    sed -i \
      -e 's/^BR2_PACKAGE_MTLS_NXP_PUF=.*/BR2_PACKAGE_MTLS_NXP_PUF=n/' \
      -e 's/^BR2_PACKAGE_MTLS_RPI_CBA=.*/BR2_PACKAGE_MTLS_RPI_CBA=y/' \
      "$BUILDROOT_CONFIG_TARGET_PATH"
    ;;
  "NXP_PUF")
    sed -i \
      -e 's/^BR2_PACKAGE_MTLS_NXP_PUF=.*/BR2_PACKAGE_MTLS_NXP_PUF=y/' \
      -e 's/^BR2_PACKAGE_MTLS_RPI_CBA=.*/BR2_PACKAGE_MTLS_RPI_CBA=n/' \
      "$BUILDROOT_CONFIG_TARGET_PATH"
    ;;
  "TWO_RPI")
    sed -i \
      -e 's/^BR2_PACKAGE_MTLS_NXP_PUF=.*/BR2_PACKAGE_MTLS_NXP_PUF=n/' \
      -e 's/^BR2_PACKAGE_MTLS_RPI_CBA=.*/BR2_PACKAGE_MTLS_RPI_CBA=n/' \
      "$BUILDROOT_CONFIG_TARGET_PATH"
    ;;
  *)
    echo "ERROR: Unknown DEMO value '$DEMO'" >&2
    exit 1
    ;;
esac

echo "# Copying wolfssl config..."
if ! cmp -s "$WOLFSSL_CONFIG_LOCAL_PATH" "$WOLFSSL_CONFIG_TARGET_PATH"; then
    cp "$WOLFSSL_CONFIG_LOCAL_PATH" "$WOLFSSL_CONFIG_TARGET_PATH"

    echo "# The configuration of Wolfssl has changed, setting up to be rebuilt"
    cd $CROSSCON_REPO_PATH
    "$DEMOS_CONTAINER_SCRIPT" bash -c "cd buildroot && make wolfssl-dirclean O=build-aarch64/"
    cd -
fi

echo "# Copying opensc config..."
if ! cmp -s "$OPENSC_CONFIG_LOCAL_PATH" "$OPENSC_CONFIG_TARGET_PATH"; then
    cp "$OPENSC_CONFIG_LOCAL_PATH" "$OPENSC_CONFIG_TARGET_PATH"

    echo "# The configuration of Opensc has changed, setting up to be rebuilt"
    cd $CROSSCON_REPO_PATH
    "$DEMOS_CONTAINER_SCRIPT" bash -c "cd buildroot && make opensc-dirclean O=build-aarch64/"
    cd -
fi

echo "# Checking Config.in..."
update_config_in

echo "# Copying app sources..."
rsync -avz --delete "$LOCAL_PROJECT_DIR/." "$CROSSCON_REPO_PATH/buildroot/package/mtls"

echo "# Done!"
