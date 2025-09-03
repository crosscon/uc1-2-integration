#!/bin/bash

set -e

source "$(dirname "$0")/common.sh"

gen_csr_and_cert() {
  local target="$1"
  local prefix="$2"
  local prefix_c="${prefix^}"

  scp ./certs/cert.conf $target:~

  echo "# Generating ${prefix} CSR"
  ssh "$target" sh -i<<EOF
export PKCS11_MODULE_PATH=/usr/lib/libckteec2.so
openssl req -engine pkcs11 -keyform engine \
-key "pkcs11:token=${prefix_c}Token;object=${prefix_c}Key;type=private;pin-value=1234" \
-new -config ~/cert.conf -out ~/${prefix}-csr.pem
date $DATE_STRING
EOF

  scp $target:~/$prefix-csr.pem ./artifacts/certs

  echo "# Generating $prefix certificate"
  openssl pkey -pubin -inform DER -in ./artifacts/certs/$prefix-pubkey.der -out ./artifacts/certs/$prefix-pubkey.pem
  openssl x509 -req -days 365 -in ./artifacts/certs/$prefix-csr.pem -CA ./certs/ca-cert.pem -CAkey ./certs/ca-key.pem -CAcreateserial -out ./artifacts/certs/$prefix-cert.pem -extfile ./certs/cert.conf -extensions req_ext

  scp ./artifacts/certs/$prefix-cert.pem "$target":~
  scp ./certs/ca-cert.pem "$target":~
}

DATE_STRING=$(date +"%m%d%H%M%Y")

if [ "$SINGLE_TARGET" = "true" ]; then
    echo "Warning! Single target configuration enabled!"
fi

gen_csr_and_cert $PI_SERVER $PI_SERVER_PREFIX
if [ "$SINGLE_TARGET" = "true" ]; then
    gen_csr_and_cert $PI_SERVER $PI_CLIENT_PREFIX
else
    gen_csr_and_cert $PI_CLIENT $PI_CLIENT_PREFIX
fi

echo "# Done!"
