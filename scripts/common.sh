#!/bin/bash

####################################
# A file for non-editable settings #
####################################

# First source user settings
source "$(dirname "$0")/settings.sh"

export APP_NAME=mtls
export BINARIES='*-tls'

export LOCAL_PROJECT_DIR="$(dirname "$(realpath "$0")")/.."
export LOCAL_ARTIFACTS_DIR="$LOCAL_PROJECT_DIR/artifacts"

export PI_USER=root
export PI_SERVER="$PI_USER@$PI_SERVER_HOST"
export PI_CLIENT="$PI_USER@$PI_CLIENT_HOST"
