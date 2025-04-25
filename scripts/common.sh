#!/bin/bash

############
# Editable #
############

export PI_HOST=192.168.10.29
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
export PI="$PI_USER@$PI_HOST"
