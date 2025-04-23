#!/bin/bash
set -e

echo "# Attempting to copy sources to container."
source "$(dirname "$0")/common.sh"
docker cp "$LOCAL_PROJECT_DIR/." "$CONTAINER_NAME:/work/crosscon/buildroot/package/mtls" && \
echo "# Copying succeeded."
