#!/bin/bash

set -e

source "$(dirname "$0")/common.sh"

DATE_STRING=$(date +"%m%d%H%M%Y")


print_usage() {
  echo "$0: Generate certificates out of CSR's produced on RPis. This script needs"
  echo "variables PI_SERVER and PI_CLIENT to be set accordingly. Check README.md"
  echo "and scripts/common.sh for more information."
  echo "Usage:"
  echo "  $0: generate certificates for both: client and server"
  echo "  $0 --server: generate certificate for server only"
  echo "  $0 --client: generate certificate for client only"

  exit 0
}

# Arguments:
# 1: target for SSH in form "user@ip"
# 2: file name prefix for the CSR, e.g. "client"
# 3: PKCS#11 token name
# 4: PKCS#11 object in the token
generate_cert() {
  local target, prefix, token, object
  target=$1
  prefix=$2
  token=$3
  object=$4

  scp ./certs/cert.conf $target:~

  echo "Generating CSR for target $target..."
  ssh "$target" sh -i<<EOF
export PKCS11_MODULE_PATH=/usr/lib/libckteec.so
openssl req -engine pkcs11 -keyform engine \
  -key "pkcs11:token=$token;object=$object;type=private;pin-value=1234" \
  -new -config ~/cert.conf -out ~/$prefix-csr.pem
date $DATE_STRING
EOF
  echo "Done!"

  echo "Copying the CSR to host..."
  scp $target:~/$prefix-csr.pem ./artifacts/certs
  echo "Done!"

  echo "Generating cerificate for target: $target..."
  openssl pkey -pubin -inform DER -in ./artifacts/certs/$prefix-pubkey.der -out ./artifacts/certs/$prefix-pubkey.pem
  openssl x509 -req -days 365 -in ./artifacts/certs/$prefix-csr.pem -CA ./certs/ca-cert.pem -CAkey ./certs/ca-key.pem -CAcreateserial -out ./artifacts/certs/$prefix-cert.pem -extfile ./certs/cert.conf -extensions req_ext
  echo "Done!"

  echo "Copying the CA and target certificate to target $target..."
  scp ./artifacts/certs/$prefix-cert.pem "$target":~
  scp ./certs/ca-cert.pem "$target":~
  echo "Done!"
}

# Handle params
for arg in "$@"; do
  case "$arg" in
    --host)
      echo "Generating certificates for server under IP: $PI_SERVER_HOST"
      generate_cert "$PI_SERVER_HOST" "server" "ServerToken" "ServerKey"
      ;;
    --client)
      echo "Generating certificates for client under IP: $PI_CLIENT_HOST"
      generate_cert "$PI_CLIENT_HOST" "client" "ClientToken" "ClientKey"
      ;;
    *)
      echo "Generating certificates for client under IP: $PI_CLIENT_HOST and for host under IP: $PI_SERVER_HOST"
      generate_cert "$PI_CLIENT_HOST" "client" "ClientToken" "ClientKey"
      generate_cert "$PI_SERVER_HOST" "server" "ServerToken" "ServerKey"
      ;;
  esac
done
