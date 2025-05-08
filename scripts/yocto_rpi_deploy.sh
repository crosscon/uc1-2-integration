#!/bin/bash
set -e

source "$(dirname "$0")/common.sh"

echo "# Attempting to deploy binaries on RPI!"
ssh "$BUILDER" bash -i<<EOF
cd "$REMOTE_YOCTO_DIR"
source /etc/bashrc
KAS_MACHINE=raspberrypi4-64 kas-container \
  --runtime-args "--volume $REMOTE_PROJECT_DIR:/workdir" \
  shell meta-zarhus/kas/common.yml:meta-zarhus/kas/cache.yml:meta-zarhus/kas/debug.yml:meta-zarhus/kas/rpi.yml \
  --command "devtool deploy-target -c $APP_NAME $PI"
EOF
echo "# Deployment finished!"
