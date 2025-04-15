#!/bin/bash

# Shared config for build scripts

export APP_NAME=mtls

export BUILDER_USER=builder
export BUILDER_HOST=192.168.10.190
export BUILDER="$BUILDER_USER@$BUILDER_HOST"

export REMOTE_YOCTO_DIR=/project-data/$USER/yocto/
export REMOTE_PROJECT_DIR=/project-data/$USER/mtls/

export LOCAL_PROJECT_DIR="$(dirname "$(realpath "$0")")/.."
export LOCAL_ARTIFACTS_DIR="$LOCAL_PROJECT_DIR/artifacts"

export PI_USER=root
export PI_HOST=192.168.10.48
export PI="$PI_USER@$PI_HOST"
