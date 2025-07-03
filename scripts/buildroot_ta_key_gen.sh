#!/bin/bash
set -e

source "$(dirname "$0")/common.sh"

gen_and_fetch_keys() {
  local target="$1"
  local prefix="$2"
  local prefix_c="${prefix^}"
  local slot="$3"

  echo "# Attempting to generate $prefix keys via PKCS#11 TA!"
  ssh "$target" sh -i<<EOF
# List slots
pkcs11-tool --module /usr/lib/libckteec.so -L
# Init token
pkcs11-tool --module /usr/lib/libckteec.so --init-token --slot ${slot} --label "${prefix_c}Token" --so-pin 1234
# Init pin
pkcs11-tool --module /usr/lib/libckteec.so --slot ${slot} --login --so-pin 1234 --init-pin --pin 1234
# Generate key pair
pkcs11-tool --module /usr/lib/libckteec.so \
  --slot ${slot} --login --pin 1234 \
  --keypairgen --key-type EC:prime256v1 \
  --label "${prefix_c}Key" --id 01
# Export pubkey
pkcs11-tool --module /usr/lib/libckteec.so \
  --slot ${slot} --login --pin 1234 \
  --read-object --type pubkey --id 01 \
  --output-file ${prefix}-pubkey.der
EOF

  echo "# Attempting to fetch certificates!"
  scp $target:~/${prefix}-pubkey.der ./artifacts/certs/
}

if [ "$SINGLE_TARGET" = "true" ]; then
    echo "Warning! Single target configuration enabled!"
fi

mkdir -p ./artifacts/certs/

gen_and_fetch_keys $PI_SERVER $PI_SERVER_PREFIX $PI_SERVER_SLOT
if [ "$SINGLE_TARGET" = "true" ]; then
    gen_and_fetch_keys $PI_SERVER $PI_CLIENT_PREFIX $PI_CLIENT_SLOT
else
    gen_and_fetch_keys $PI_CLIENT $PI_CLIENT_PREFIX $PI_CLIENT_SLOT
fi


echo "# Keys generated, pubkeys fetched!"
