#!/bin/bash
set -e

source "$(dirname "$0")/common.sh"

echo "# Attempting to generate server keys via PKCS#11 TA!"
ssh "$PI_SERVER" sh -i<<EOF
# List slots
pkcs11-tool --module /usr/lib/libckteec.so -L
# Init token
pkcs11-tool --module /usr/lib/libckteec.so --init-token --slot 0 --label "ServerToken" --so-pin 1234
# Init pin
pkcs11-tool --module /usr/lib/libckteec.so --slot 0 --login --so-pin 1234 --init-pin --pin 1234
# Generate key pair
pkcs11-tool --module /usr/lib/libckteec.so \
  --slot 0 --login --pin 1234 \
  --keypairgen --key-type EC:prime256v1 \
  --label "ServerKey" --id 01
# Export pubkey
pkcs11-tool --module /usr/lib/libckteec.so \
  --slot 0 --login --pin 1234 \
  --read-object --type pubkey --id 01 \
  --output-file server-pubkey.der
EOF

echo "# Attempting to generate client keys via PKCS#11 TA!"
ssh "$PI_CLIENT" sh -i<<EOF
# Init token
pkcs11-tool --module /usr/lib/libckteec.so --init-token --slot 0 --label "ClientToken" --so-pin 1234
# Init pin
pkcs11-tool --module /usr/lib/libckteec.so --slot 0 --login --so-pin 1234 --init-pin --pin 1234
# Generate key pair
pkcs11-tool --module /usr/lib/libckteec.so \
  --slot 0 --login --pin 1234 \
  --keypairgen --key-type EC:prime256v1 \
  --label "ClientKey" --id 01
# Export pubkey
pkcs11-tool --module /usr/lib/libckteec.so \
  --slot 0 --login --pin 1234 \
  --read-object --type pubkey --id 01 \
  --output-file client-pubkey.der

EOF

mkdir -p ./artifacts/certs/
scp $PI_SERVER:~/server-pubkey.der ./artifacts/certs/
scp $PI_CLIENT:~/client-pubkey.der ./artifacts/certs/

echo "# Keys generated, pubkeys fetched!"
