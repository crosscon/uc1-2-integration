#!/bin/bash
set -e

source "$(dirname "$0")/common.sh"

echo "# Attempting to fetch artifacts."
mkdir -p "$LOCAL_ARTIFACTS_DIR"
scp "$BUILDER:${REMOTE_YOCTO_DIR}build/tmp/work/cortexa72-zarhus-linux/mtls/1.0+git/devtooltmp-*/workdir/image/usr/bin/$APP_NAME" "$LOCAL_ARTIFACTS_DIR/"
echo "# Fetching artifacts done."
