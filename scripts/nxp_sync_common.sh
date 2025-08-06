#!/bin/bash

source "$(dirname "$0")/common.sh"

# Sync common with tls client for nxp
rsync -av $REPO_ROOT/include/common $UC1_INTEGRATION_PATH/tls_client/src/
