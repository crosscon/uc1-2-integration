#!/bin/bash
set -e

echo "# Attempting to sync changes with buildroot."
source "$(dirname "$0")/common.sh"
rsync -avz --delete "$LOCAL_PROJECT_DIR/." "$CROSSCON_REPO_PATH/buildroot/package/mtls" && \
echo "# Sync succeeded."
