#!/bin/bash

############
# Editable #
############

# Used for deploying binaries on target when build with yocto
export PI_SERVER_HOST=192.168.10.51
export PI_CLIENT_HOST=192.168.10.52

# A path to CROSSCON demo repositories.
# It is used to sync contents of this repository with CROSSCON's buildroot.
export CROSSCON_REPO_PATH=/home/$USER/Development/tmp/crosscon-demos

##############
# Uneditable #
##############

export APP_NAME=mtls
export BINARIES='*-tls'

# Unlikely to change
export BUILDER_USER=builder
export BUILDER_HOST=192.168.10.190
export BUILDER="$BUILDER_USER@$BUILDER_HOST"

# Must be respected
export REMOTE_YOCTO_DIR=/project-data/$USER/yocto/
export REMOTE_PROJECT_DIR=/project-data/$USER/$APP_NAME/

export LOCAL_PROJECT_DIR="$(dirname "$(realpath "$0")")/.."
export LOCAL_ARTIFACTS_DIR="$LOCAL_PROJECT_DIR/artifacts"

export PI_USER=root
export PI_SERVER="$PI_USER@$PI_SERVER_HOST"
export PI_CLIENT="$PI_USER@$PI_CLIENT_HOST"
