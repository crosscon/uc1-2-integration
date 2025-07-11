#!/usr/bin/env bash

set -e

ROOT_DIR=$(git -C "$(dirname "$(realpath $0)")" rev-parse --show-toplevel)
CERTS_DIR="$ROOT_DIR/certs"
GEN_CERTS_DIR="$ROOT_DIR/artifacts/certs"
OUTPUT_DIR="$ROOT_DIR/artifacts/certs/der"
mkdir -p "$OUTPUT_DIR"

# Generate local client private-key
cd $GEN_CERTS_DIR
openssl ecparam -name prime256v1 -genkey -noout -out local-client-key.pem
openssl req -new -key local-client-key.pem -out local-client.csr -subj "/CN=local-client"
openssl x509 -req -days 365 -in local-client.csr -CA $CERTS_DIR/ca-cert.pem -CAkey $CERTS_DIR/ca-key.pem -CAcreateserial -out local-client-cert.pem  -extfile $CERTS_DIR/cert.conf -extensions req_ext
openssl ec -in local-client-key.pem -pubout -out local-client-pubkey.pem

# List of input PEM files
FILES=(
    "certs/ca-cert.pem"
    "artifacts/certs/local-client-cert.pem"
    "artifacts/certs/local-client-key.pem"
)

echo "Converting PEM files to DER and generating C headers..."

for FILE in "${FILES[@]}"; do
    INPUT="$ROOT_DIR/$FILE"
    BASENAME=$(basename "$FILE" .pem)
    DER_FILE="$OUTPUT_DIR/${BASENAME}.der"
    C_HEADER="$OUTPUT_DIR/${BASENAME}.h"

    # Determine file type: key or cert
    if grep -q "PRIVATE KEY" "$INPUT"; then
        echo "Converting private key: $FILE"
        openssl pkcs8 -topk8 -nocrypt -inform PEM -outform DER -in "$INPUT" -out "$DER_FILE"
    elif grep -q "PUBLIC KEY" "$INPUT"; then
        echo "Converting public key: $FILE"
        openssl pkey -pubin -inform PEM -outform DER -in "$INPUT" -out "$DER_FILE"
    else
        echo "Converting certificate: $FILE"
        openssl x509 -inform PEM -outform DER -in "$INPUT" -out "$DER_FILE"
    fi

    # Convert DER to C-style array
    echo "Generating C header: $C_HEADER"
    (
        cd "$OUTPUT_DIR"
        xxd -i "$(basename "$DER_FILE")" > "$(basename "$C_HEADER")"
    )

    # Rename array to match the base name
    sed -i "s/${BASENAME}_der/${BASENAME}_der/g" "$C_HEADER"

    echo "Done: $FILE → $C_HEADER"
done

echo "All files converted and C headers generated in $OUTPUT_DIR/"
