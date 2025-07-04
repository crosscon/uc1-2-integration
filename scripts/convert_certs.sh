#!/usr/bin/env bash

set -e

CERT_DIR="./certs/ecc"
OUTPUT_DIR="./artifacts/certs/ecc"
mkdir -p "$OUTPUT_DIR"

# List of input PEM files
FILES=(
    "ca-cert.pem"
    "client-cert.pem"
    "client-key.pem"
    "client-keyPub.pem"
    "server-cert.pem"
    "server-key.pem"
)

echo "Converting PEM files to DER and generating C headers..."

for FILE in "${FILES[@]}"; do
    INPUT="$CERT_DIR/$FILE"
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
    xxd -i "$DER_FILE" > "$C_HEADER"

    # Rename array to match the base name
    sed -i "s/${BASENAME}_der/${BASENAME}_der/g" "$C_HEADER"

    echo "Done: $FILE → $C_HEADER"
done

echo "All files converted and C headers generated in $OUTPUT_DIR/"
