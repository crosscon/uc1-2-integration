#!/bin/bash

source "$(dirname "$0")/common.sh"

BUILD_SCRIPT="./rpi4-ws/run.sh"
BINARIES_PATH="./buildroot/build-aarch64/build/mtls-1.0"
TARGET_PATH="/usr/bin"

set -e

cd $CROSSCON_REPO_PATH

echo "# Rebuilding mtls app..."
"$BUILD_SCRIPT" bash -c "cd buildroot && make mtls-dirclean O=build-aarch64/ && make mtls O=build-aarch64/"

echo "# Available binaries:"
ls "$BINARIES_PATH"/*-tls

if [ "$SINGLE_TARGET" = "true" ]; then
    echo "# Deploying both binaries on a single target..."
    scp "$BINARIES_PATH"/*-tls root@"$PI_SERVER_HOST":"$TARGET_PATH/" || {
        echo "Ensure the server is down!"
        exit 1l
    }
else
    echo "# Deploying binaries on both targets..."
    scp $BINARIES_PATH/server-tls root@$PI_SERVER_HOST:"$TARGET_PATH/";
    scp $BINARIES_PATH/client-tls root@$PI_CLIENT_HOST:"$TARGET_PATH/";
fi

echo "# Done!"
