#!/bin/bash
set -e

source "$(dirname "$0")/common.sh"

echo "# Attempting to generate private keys on Pi via PKCS#11 TA!"
ssh "$PI" sh -i<<EOF
# List slots
pkcs11-tool --module /usr/lib/libckteec.so -L
# Init token
pkcs11-tool --module /usr/lib/libckteec.so --init-token --slot 0 --label "ServerToken" --so-pin 1234
# Init pin
pkcs11-tool --module /usr/lib/libckteec.so --slot 0 --login --so-pin 1234 --init-pin --pin 1234
# Generate key pair
pkcs11-tool --module /usr/lib/libckteec.so \
  --slot 0 --login --pin 1234 \
  --keypairgen --key-type rsa:2048 \
  --label "ServerKey" --id 01
# Export pubkey
pkcs11-tool --module /usr/lib/libckteec.so \
  --slot 0 --login --pin 1234 \
  --read-object --type pubkey --id 01 \
  --output-file server-pubkey.der
EOF
mkdir -p ./artifacts/certs/
scp $PI:~/server-pubkey.der ./artifacts/certs/
echo "# Keys generated, pubkey fetched!"
