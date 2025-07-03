#!/bin/bash

############
# Editable #
############

# Target IPs
export PI_SERVER_HOST=192.168.10.51
export PI_CLIENT_HOST=192.168.10.52

# A path to CROSSCON demo repositories.
# It is used to sync contents of this repository with CROSSCON's buildroot.
export CROSSCON_REPO_PATH=/home/$USER/Development/crosscon-demos

##############
# Uneditable #
##############

export APP_NAME=mtls
export BINARIES='*-tls'

# Must be respected

export LOCAL_PROJECT_DIR="$(dirname "$(realpath "$0")")/.."
export LOCAL_ARTIFACTS_DIR="$LOCAL_PROJECT_DIR/artifacts"

export PI_USER=root
export PI_SERVER="$PI_USER@$PI_SERVER_HOST"
export PI_CLIENT="$PI_USER@$PI_CLIENT_HOST"
