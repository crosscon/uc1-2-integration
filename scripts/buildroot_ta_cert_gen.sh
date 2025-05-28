#!/bin/bash

set -e

source "$(dirname "$0")/common.sh"

scp ./certs/cert.conf $PI_SERVER:~
scp ./certs/cert.conf $PI_CLIENT:~

DATE_STRING=$(date +"%m%d%H%M%Y")

echo "# Generating server CSR"
ssh "$PI_SERVER" sh -i<<EOF
export PKCS11_MODULE_PATH=/usr/lib/libckteec.so
openssl req -engine pkcs11 -keyform engine \
  -key "pkcs11:token=ServerToken;object=ServerKey;type=private;pin-value=1234" \
  -new -config ~/cert.conf -out ~/server-csr.pem 
date $DATE_STRING
EOF

ssh "$PI_CLIENT" sh -i<<EOF
export PKCS11_MODULE_PATH=/usr/lib/libckteec.so
openssl req -engine pkcs11 -keyform engine \
  -key "pkcs11:token=ClientToken;object=ClientKey;type=private;pin-value=1234" \
  -new -config ~/cert.conf -out ~/client-csr.pem 
date $DATE_STRING
EOF

scp $PI_SERVER:~/server-csr.pem ./artifacts/certs
scp $PI_CLIENT:~/client-csr.pem ./artifacts/certs

echo "# Generating server certificate"
openssl pkey -pubin -inform DER -in ./artifacts/certs/server-pubkey.der -out ./artifacts/certs/server-pubkey.pem
openssl x509 -req -days 365 -in ./artifacts/certs/server-csr.pem -CA ./certs/ca-cert.pem -CAkey ./certs/ca-key.pem -CAcreateserial -out ./artifacts/certs/server-cert.pem -extfile ./certs/cert.conf -extensions req_ext

echo "# Generating client cerificate"
openssl pkey -pubin -inform DER -in ./artifacts/certs/client-pubkey.der -out ./artifacts/certs/client-pubkey.pem
openssl x509 -req -days 365 -in ./artifacts/certs/client-csr.pem -CA ./certs/ca-cert.pem -CAkey ./certs/ca-key.pem -CAcreateserial -out ./artifacts/certs/client-cert.pem -extfile ./certs/cert.conf -extensions req_ext

scp ./artifacts/certs/server-cert.pem "$PI_SERVER":~ 
scp ./artifacts/certs/client-cert.pem "$PI_CLIENT":~
scp ./certs/ca-cert.pem "$PI_SERVER":~
scp ./certs/ca-cert.pem "$PI_CLIENT":~
