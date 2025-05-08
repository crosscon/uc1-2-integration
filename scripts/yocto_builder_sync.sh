#!/bin/bash
set -e

echo "# Attempting to sync project on builder."
source "$(dirname "$0")/common.sh"
rsync -avz --delete "$LOCAL_PROJECT_DIR/" "$BUILDER:$REMOTE_PROJECT_DIR/"
echo "# Syncing succeeded."
