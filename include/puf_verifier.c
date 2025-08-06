#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/ecc.h>
#include <wolfssl/wolfcrypt/sha256.h>
#include <wolfssl/wolfcrypt/asn.h>
#include <stdbool.h>

#ifndef STANDALONE
  #include "common/challenge.h"
  #include "common/log.h"
#endif

// Try to determine which math backend is available
#ifdef WOLFSSL_SP_MATH
    #include <wolfssl/wolfcrypt/sp_int.h>
    typedef sp_int math_int_t;
    #define USE_SP_MATH 1
#else
    #include <wolfssl/wolfcrypt/integer.h>
    typedef mp_int math_int_t;
    #define USE_SP_MATH 0
#endif

#define HEX_BUFFER_SIZE 1024
#define COORDINATE_BYTES 32
#define NONCE_BYTES 64
#define PREIMAGE_BYTES 128

typedef struct {
    math_int_t x, y;
} EccPoint;

typedef struct {
    char *gx, *gy, *hx, *hy;
    char *COMx, *COMy, *Px, *Py;
    char *nonce, *v, *w;
} Args;

// Function prototypes
int parse_hex_to_math(const char* input_str, math_int_t* num);
void print_usage(const char* program_name);
int init_ecc_point(EccPoint* point);
void free_ecc_point(EccPoint* point);
void print_ecc_point(const char* name, EccPoint* point);
void print_scalar(const char* name, math_int_t* scalar);
int ecc_point_add_custom(EccPoint* result, EccPoint* a, EccPoint* b, ecc_key* key);
int ecc_point_mul_custom(EccPoint* result, math_int_t* scalar, EccPoint* point, ecc_key* key);
int ecc_points_equal(EccPoint* a, EccPoint* b);
int verify_zk_proof(Args* args);

// Helper function to test if a specific bit is set in a big integer
int test_bit(math_int_t* num, int bit_index) {
#if USE_SP_MATH
    // For SP math, use a simple approach similar to mp_int
    if (bit_index >= sp_count_bits(num)) return 0;

    sp_int temp;
    sp_init(&temp);
    sp_copy(num, &temp);

    // Divide by 2^bit_index to shift right
    for (int i = 0; i < bit_index; i++) {
        sp_div_2(&temp, &temp);
    }

    // Check if the result is odd (bit is set)
    int result = sp_isodd(&temp);

    sp_clear(&temp);
    return result;
#else
    // For mp_int, implement bit testing using division
    if (bit_index >= mp_count_bits(num)) return 0;

    mp_int temp;
    mp_init(&temp);

    // Copy the number
    mp_copy(num, &temp);

    // Divide by 2^bit_index to shift right
    for (int i = 0; i < bit_index; i++) {
        mp_div_2(&temp, &temp);
    }

    // Check if the result is odd (bit is set)
    int result = mp_isodd(&temp);

    mp_clear(&temp);
    return result;
#endif
}

int parse_hex_to_math(const char* hex_str, math_int_t* num) {
    char* clean_str = (char*)hex_str;
    int radix = 10;  // Default to decimal

    // Check if it starts with 0x or 0X (hexadecimal)
    if (strncmp(hex_str, "0x", 2) == 0 || strncmp(hex_str, "0X", 2) == 0) {
        clean_str = (char*)hex_str + 2;  // Skip 0x prefix
        radix = 16;  // Use hexadecimal
    }
    // Otherwise treat as decimal (radix = 10)

#if USE_SP_MATH
    return sp_read_radix(num, clean_str, radix);
#else
    return mp_read_radix(num, clean_str, radix);
#endif
}

#ifdef STANDALONE
// Print usage information
void print_usage(const char* program_name) {
    printf("Zero Knowledge Verifier Proof Tool (P-256)\n");
    printf("Usage: %s [OPTIONS]\n\n", program_name);
    printf("Options:\n");
    printf("  --gx <hex>           g_x coordinate (hex)\n");
    printf("  --gy <hex>           g_y coordinate (hex)\n");
    printf("  --hx <hex>           h_x coordinate (hex)\n");
    printf("  --hy <hex>           h_y coordinate (hex)\n");
    printf("  --COMx <hex>         COM_x coordinate (hex)\n");
    printf("  --COMy <hex>         COM_y coordinate (hex)\n");
    printf("  --Px <hex>           P_x coordinate (hex)\n");
    printf("  --Py <hex>           P_y coordinate (hex)\n");
    printf("  --nonce <hex>        nonce scalar (hex)\n");
    printf("  --v <hex>            scalar v (hex)\n");
    printf("  --w <hex>            scalar w (hex)\n");
    printf("  -h, --help           Show this help message\n");
}
#endif /* STANDALONE */

// Initialize ECC point
int init_ecc_point(EccPoint* point) {
#if USE_SP_MATH
    if (sp_init(&point->x) != MP_OKAY) return -1;
    if (sp_init(&point->y) != MP_OKAY) {
        sp_clear(&point->x);
        return -1;
    }
#else
    if (mp_init(&point->x) != MP_OKAY) return -1;
    if (mp_init(&point->y) != MP_OKAY) {
        mp_clear(&point->x);
        return -1;
    }
#endif
    return 0;
}

// Free ECC point
void free_ecc_point(EccPoint* point) {
#if USE_SP_MATH
    sp_clear(&point->x);
    sp_clear(&point->y);
#else
    mp_clear(&point->x);
    mp_clear(&point->y);
#endif
}

// Print ECC point in SageMath format: E((0xhex_x, 0xhex_y))
void print_ecc_point(const char* name, EccPoint* point) {
    byte x_bytes[64], y_bytes[64];  // Large enough for any coordinate
    int x_len, y_len;

    // Get the byte representation
#if USE_SP_MATH
    x_len = sp_unsigned_bin_size(&point->x);
    y_len = sp_unsigned_bin_size(&point->y);
    sp_to_unsigned_bin(&point->x, x_bytes);
    sp_to_unsigned_bin(&point->y, y_bytes);
#else
    x_len = mp_unsigned_bin_size(&point->x);
    y_len = mp_unsigned_bin_size(&point->y);
    mp_to_unsigned_bin(&point->x, x_bytes);
    mp_to_unsigned_bin(&point->y, y_bytes);
#endif

    // Print in SageMath format with lowercase hex to match exactly
    printf("%s = E((0x", name);
    for (int i = 0; i < x_len; i++) {
        printf("%02x", x_bytes[i]);  // lowercase hex to match SageMath
    }
    printf(", 0x");
    for (int i = 0; i < y_len; i++) {
        printf("%02x", y_bytes[i]);  // lowercase hex to match SageMath
    }
    printf("))\n");
}

// Print scalar in hex format to match SageMath
void print_scalar(const char* name, math_int_t* scalar) {
    byte bytes[64];  // Large enough for any scalar
    int len;

#if USE_SP_MATH
    len = sp_unsigned_bin_size(scalar);
    sp_to_unsigned_bin(scalar, bytes);
#else
    len = mp_unsigned_bin_size(scalar);
    mp_to_unsigned_bin(scalar, bytes);
#endif

    printf("%s = 0x", name);
    for (int i = 0; i < len; i++) {
        printf("%02x", bytes[i]);  // lowercase hex to match SageMath
    }
    printf("\n");
}

// Parse decimal or hexadecimal string to math_int_t (matches SageMath parse_input)
int ecc_point_add_custom(EccPoint* result, EccPoint* a, EccPoint* b, ecc_key* key) {
    // Remove verbose output - only print on first call
    static int first_call = 1;
    if (first_call) {
        printf("Performing point addition manually...\n");
        first_call = 0;
    }

    // P-256 prime: p = 2^256 - 2^224 + 2^192 + 2^96 - 1
    math_int_t p, temp1, temp2, temp3, lambda, x3, y3;

    // Initialize temporary variables
#if USE_SP_MATH
    sp_init(&p); sp_init(&temp1); sp_init(&temp2); sp_init(&temp3);
    sp_init(&lambda); sp_init(&x3); sp_init(&y3);

    // Set P-256 prime
    sp_read_radix(&p, "FFFFFFFF00000001000000000000000000000000FFFFFFFFFFFFFFFFFFFFFFFF", 16);
#else
    mp_init(&p); mp_init(&temp1); mp_init(&temp2); mp_init(&temp3);
    mp_init(&lambda); mp_init(&x3); mp_init(&y3);

    // Set P-256 prime
    mp_read_radix(&p, "FFFFFFFF00000001000000000000000000000000FFFFFFFFFFFFFFFFFFFFFFFF", 16);
#endif

    // Check for point at infinity cases
#if USE_SP_MATH
    if (sp_iszero(&a->x) && sp_iszero(&a->y)) {
        // a is point at infinity, result = b
        sp_copy(&b->x, &result->x);
        sp_copy(&b->y, &result->y);
        goto cleanup_add;
    }
    if (sp_iszero(&b->x) && sp_iszero(&b->y)) {
        // b is point at infinity, result = a
        sp_copy(&a->x, &result->x);
        sp_copy(&a->y, &result->y);
        goto cleanup_add;
    }

    // Check if points are the same (point doubling case)
    if (sp_cmp(&a->x, &b->x) == MP_EQ) {
        if (sp_cmp(&a->y, &b->y) == MP_EQ) {
            // Point doubling: lambda = (3*x1^2 - 3) / (2*y1)
            // For P-256, a = -3, so 3*x1^2 + a = 3*x1^2 - 3

            // temp1 = x1^2
            sp_sqr(&a->x, &temp1);
            sp_mod(&temp1, &p, &temp1);

            // temp2 = 3*x1^2
            sp_mul_d(&temp1, 3, &temp2);
            sp_mod(&temp2, &p, &temp2);

            // temp1 = 3*x1^2 - 3
            sp_sub_d(&temp2, 3, &temp1);
            sp_mod(&temp1, &p, &temp1);

            // temp2 = 2*y1
            sp_mul_d(&a->y, 2, &temp2);
            sp_mod(&temp2, &p, &temp2);

            // lambda = (3*x1^2 - 3) / (2*y1) = temp1 * inv(temp2)
            sp_invmod(&temp2, &p, &temp3);
            sp_mulmod(&temp1, &temp3, &p, &lambda);
        } else {
            // Points are inverses, result is point at infinity
            sp_zero(&result->x);
            sp_zero(&result->y);
            goto cleanup_add;
        }
    } else {
        // Regular point addition: lambda = (y2 - y1) / (x2 - x1)

        // temp1 = y2 - y1
        sp_sub(&b->y, &a->y, &temp1);
        sp_mod(&temp1, &p, &temp1);

        // temp2 = x2 - x1
        sp_sub(&b->x, &a->x, &temp2);
        sp_mod(&temp2, &p, &temp2);

        // lambda = (y2 - y1) / (x2 - x1) = temp1 * inv(temp2)
        sp_invmod(&temp2, &p, &temp3);
        sp_mulmod(&temp1, &temp3, &p, &lambda);
    }

    // x3 = lambda^2 - x1 - x2
    sp_sqr(&lambda, &x3);
    sp_sub(&x3, &a->x, &x3);
    sp_sub(&x3, &b->x, &x3);
    sp_mod(&x3, &p, &x3);

    // y3 = lambda * (x1 - x3) - y1
    sp_sub(&a->x, &x3, &temp1);
    sp_mulmod(&lambda, &temp1, &p, &y3);
    sp_sub(&y3, &a->y, &y3);
    sp_mod(&y3, &p, &y3);

    // Copy result
    sp_copy(&x3, &result->x);
    sp_copy(&y3, &result->y);

cleanup_add:
    sp_clear(&p); sp_clear(&temp1); sp_clear(&temp2); sp_clear(&temp3);
    sp_clear(&lambda); sp_clear(&x3); sp_clear(&y3);
#else
    if (mp_iszero(&a->x) && mp_iszero(&a->y)) {
        // a is point at infinity, result = b
        mp_copy(&b->x, &result->x);
        mp_copy(&b->y, &result->y);
        goto cleanup_add_mp;
    }
    if (mp_iszero(&b->x) && mp_iszero(&b->y)) {
        // b is point at infinity, result = a
        mp_copy(&a->x, &result->x);
        mp_copy(&a->y, &result->y);
        goto cleanup_add_mp;
    }

    // Check if points are the same (point doubling case)
    if (mp_cmp(&a->x, &b->x) == MP_EQ) {
        if (mp_cmp(&a->y, &b->y) == MP_EQ) {
            // Point doubling: lambda = (3*x1^2 - 3) / (2*y1)

            // temp1 = x1^2
            mp_sqr(&a->x, &temp1);
            mp_mod(&temp1, &p, &temp1);

            // temp2 = 3*x1^2
            mp_mul_d(&temp1, 3, &temp2);
            mp_mod(&temp2, &p, &temp2);

            // temp1 = 3*x1^2 - 3
            mp_sub_d(&temp2, 3, &temp1);
            mp_mod(&temp1, &p, &temp1);

            // temp2 = 2*y1
            mp_mul_d(&a->y, 2, &temp2);
            mp_mod(&temp2, &p, &temp2);

            // lambda = (3*x1^2 - 3) / (2*y1) = temp1 * inv(temp2)
            mp_invmod(&temp2, &p, &temp3);
            mp_mulmod(&temp1, &temp3, &p, &lambda);
        } else {
            // Points are inverses, result is point at infinity
            mp_zero(&result->x);
            mp_zero(&result->y);
            goto cleanup_add_mp;
        }
    } else {
        // Regular point addition: lambda = (y2 - y1) / (x2 - x1)

        // temp1 = y2 - y1
        mp_sub(&b->y, &a->y, &temp1);
        mp_mod(&temp1, &p, &temp1);

        // temp2 = x2 - x1
        mp_sub(&b->x, &a->x, &temp2);
        mp_mod(&temp2, &p, &temp2);

        // lambda = (y2 - y1) / (x2 - x1) = temp1 * inv(temp2)
        mp_invmod(&temp2, &p, &temp3);
        mp_mulmod(&temp1, &temp3, &p, &lambda);
    }

    // x3 = lambda^2 - x1 - x2
    mp_sqr(&lambda, &x3);
    mp_sub(&x3, &a->x, &x3);
    mp_sub(&x3, &b->x, &x3);
    mp_mod(&x3, &p, &x3);

    // y3 = lambda * (x1 - x3) - y1
    mp_sub(&a->x, &x3, &temp1);
    mp_mulmod(&lambda, &temp1, &p, &y3);
    mp_sub(&y3, &a->y, &y3);
    mp_mod(&y3, &p, &y3);

    // Copy result
    mp_copy(&x3, &result->x);
    mp_copy(&y3, &result->y);

cleanup_add_mp:
    mp_clear(&p); mp_clear(&temp1); mp_clear(&temp2); mp_clear(&temp3);
    mp_clear(&lambda); mp_clear(&x3); mp_clear(&y3);
#endif

    return 0;
}

// ECC scalar multiplication using double-and-add algorithm
int ecc_point_mul_custom(EccPoint* result, math_int_t* scalar, EccPoint* point, ecc_key* key) {
    // Remove verbose output - only print on first call per operation
    static int first_call = 1;
    if (first_call) {
        printf("Performing scalar multiplication using double-and-add...\n");
        first_call = 0;
    }

    // Check if scalar is zero
#if USE_SP_MATH
    if (sp_iszero(scalar)) {
        // Result is point at infinity
        sp_zero(&result->x);
        sp_zero(&result->y);
        return 0;
    }
#else
    if (mp_iszero(scalar)) {
        // Result is point at infinity
        mp_zero(&result->x);
        mp_zero(&result->y);
        return 0;
    }
#endif

    // Initialize result to point at infinity
    EccPoint temp_result, temp_point;
    if (init_ecc_point(&temp_result) < 0 || init_ecc_point(&temp_point) < 0) {
        return -1;
    }

#if USE_SP_MATH
    sp_zero(&temp_result.x);
    sp_zero(&temp_result.y);

    // Copy input point
    sp_copy(&point->x, &temp_point.x);
    sp_copy(&point->y, &temp_point.y);

    // Get number of bits in scalar
    int bits = sp_count_bits(scalar);

    // Double-and-add algorithm - process bits from MSB to LSB
    for (int i = bits - 1; i >= 0; i--) {
        // Double the current result (except on first iteration)
        if (i < bits - 1) {
            int ret = ecc_point_add_custom(&temp_result, &temp_result, &temp_result, key);
            if (ret != 0) {
                free_ecc_point(&temp_result);
                free_ecc_point(&temp_point);
                return ret;
            }
        }

        // Check if bit i is set (add point if bit is 1)
        if (test_bit(scalar, i)) {
            // Add input point to result
            int ret = ecc_point_add_custom(&temp_result, &temp_result, &temp_point, key);
            if (ret != 0) {
                free_ecc_point(&temp_result);
                free_ecc_point(&temp_point);
                return ret;
            }
        }
    }
#else
    mp_zero(&temp_result.x);
    mp_zero(&temp_result.y);

    // Copy input point
    mp_copy(&point->x, &temp_point.x);
    mp_copy(&point->y, &temp_point.y);

    // Get number of bits in scalar
    int bits = mp_count_bits(scalar);

    // Double-and-add algorithm - process bits from MSB to LSB
    for (int i = bits - 1; i >= 0; i--) {
        // Double the current result (except on first iteration)
        if (i < bits - 1) {
            int ret = ecc_point_add_custom(&temp_result, &temp_result, &temp_result, key);
            if (ret != 0) {
                free_ecc_point(&temp_result);
                free_ecc_point(&temp_point);
                return ret;
            }
        }

        // Check if bit i is set (add point if bit is 1)
        if (test_bit(scalar, i)) {
            // Add input point to result
            int ret = ecc_point_add_custom(&temp_result, &temp_result, &temp_point, key);
            if (ret != 0) {
                free_ecc_point(&temp_result);
                free_ecc_point(&temp_point);
                return ret;
            }
        }
    }
#endif

    // Copy result
#if USE_SP_MATH
    sp_copy(&temp_result.x, &result->x);
    sp_copy(&temp_result.y, &result->y);
#else
    mp_copy(&temp_result.x, &result->x);
    mp_copy(&temp_result.y, &result->y);
#endif

    free_ecc_point(&temp_result);
    free_ecc_point(&temp_point);

    return 0;
}

// Compare two ECC points for equality
int ecc_points_equal(EccPoint* a, EccPoint* b) {
#if USE_SP_MATH
    return (sp_cmp(&a->x, &b->x) == MP_EQ && sp_cmp(&a->y, &b->y) == MP_EQ);
#else
    return (mp_cmp(&a->x, &b->x) == MP_EQ && mp_cmp(&a->y, &b->y) == MP_EQ);
#endif
}

int verify_zk_proof(Args* args) {
    int ret = 0;
    ecc_key key;
    EccPoint g, h, COM, P;
    math_int_t nonce, v, w, alpha;
    wc_Sha256 sha;
    byte hash[WC_SHA256_DIGEST_SIZE];
    byte preimage[PREIMAGE_BYTES];

#if STANDALONE
    printf("\n=== Zero Knowledge Verifier Proof Tool (CLI Mode) ===\n\n");
#else
    printf("=== Attempting to verify PUF! ===\n");
#endif

    // Step 1: Initialize P-256 curve
    printf("Step 1: Initialize secp256r1 curve (P-256)...\n");
    printf("Math backend: %s\n", USE_SP_MATH ? "SP Math" : "mp_int");

    ret = wc_ecc_init(&key);
    if (ret != 0) {
        printf("Error initializing ECC key: %d\n", ret);
        return ret;
    }

    ret = wc_ecc_set_curve(&key, 32, ECC_SECP256R1);
    if (ret != 0) {
        printf("Error setting P-256 curve: %d\n", ret);
        wc_ecc_free(&key);
        return ret;
    }
    printf("Curve initialized: y^2 = x^3 + ax + b\n\n");

    // Initialize points and scalars
    if (init_ecc_point(&g) < 0 || init_ecc_point(&h) < 0 ||
        init_ecc_point(&COM) < 0 || init_ecc_point(&P) < 0) {
        printf("Error initializing ECC points\n");
        ret = -1;
        goto cleanup;
    }

#if USE_SP_MATH
    if (sp_init(&nonce) != MP_OKAY || sp_init(&v) != MP_OKAY ||
        sp_init(&w) != MP_OKAY || sp_init(&alpha) != MP_OKAY) {
        printf("Error initializing sp_int variables\n");
        ret = -1;
        goto cleanup;
    }
#else
    if (mp_init(&nonce) != MP_OKAY || mp_init(&v) != MP_OKAY ||
        mp_init(&w) != MP_OKAY || mp_init(&alpha) != MP_OKAY) {
        printf("Error initializing mp_int variables\n");
        ret = -1;
        goto cleanup;
    }
#endif

    // CLI mode - parse from arguments
    printf("=== CLI Mode ===\n\n");

    if (!args->gx || !args->gy || !args->hx || !args->hy ||
        !args->COMx || !args->COMy || !args->Px || !args->Py ||
        !args->nonce || !args->v || !args->w) {
        printf("Error: All parameters required in CLI mode\n");
        ret = -1;
        goto cleanup;
    }

    printf("Step 2: Parsing base points g and h\n");
    parse_hex_to_math(args->gx, &g.x);
    parse_hex_to_math(args->gy, &g.y);
    print_ecc_point("g", &g);

    parse_hex_to_math(args->hx, &h.x);
    parse_hex_to_math(args->hy, &h.y);
    print_ecc_point("h", &h);

    printf("\nStep 3: Parsing COM commitment\n");
    parse_hex_to_math(args->COMx, &COM.x);
    parse_hex_to_math(args->COMy, &COM.y);
    print_ecc_point("COM", &COM);

    printf("\nStep 4: Parsing nonce\n");
    parse_hex_to_math(args->nonce, &nonce);
    print_scalar("nonce", &nonce);

    printf("\nStep 5: Parsing P commitment\n");
    parse_hex_to_math(args->Px, &P.x);
    parse_hex_to_math(args->Py, &P.y);
    print_ecc_point("P", &P);

    printf("\nStep 6: Parsing scalars v and w\n");
    parse_hex_to_math(args->v, &v);
    parse_hex_to_math(args->w, &w);
    print_scalar("v", &v);
    print_scalar("w", &w);


    // Step 7: Reconstruct raw 64-byte P = P.x || P.y
    printf("\nStep 7: Reconstruct raw P.x||P.y as hex\n");

    // Convert P coordinates to bytes (32 bytes each, big-endian)
    byte px_bytes[COORDINATE_BYTES], py_bytes[COORDINATE_BYTES];
#if USE_SP_MATH
    sp_to_unsigned_bin_len(&P.x, px_bytes, COORDINATE_BYTES);
    sp_to_unsigned_bin_len(&P.y, py_bytes, COORDINATE_BYTES);
#else
    mp_to_unsigned_bin_len(&P.x, px_bytes, COORDINATE_BYTES);
    mp_to_unsigned_bin_len(&P.y, py_bytes, COORDINATE_BYTES);
#endif

    // Copy to preimage buffer
    memcpy(preimage, px_bytes, COORDINATE_BYTES);
    memcpy(preimage + COORDINATE_BYTES, py_bytes, COORDINATE_BYTES);

    printf("P (hex) = ");
    for (int i = 0; i < 2 * COORDINATE_BYTES; i++) {
        printf("%02X", preimage[i]);
    }
    printf("\n");

    // Step 8: Reconstruct raw 64-byte nonce
    printf("\nStep 8: Reconstruct raw 64-byte nonce as hex\n");

    byte nonce_bytes[NONCE_BYTES];

    // Get the actual nonce size and copy to the right position
    int nonce_actual_size;
#if USE_SP_MATH
    nonce_actual_size = sp_unsigned_bin_size(&nonce);
    // Place nonce at the end (right-aligned in 16 bytes)
    sp_to_unsigned_bin(&nonce, nonce_bytes);
#else
    nonce_actual_size = mp_unsigned_bin_size(&nonce);
    // Place nonce at the end (right-aligned in 16 bytes)
    mp_to_unsigned_bin(&nonce, nonce_bytes);
#endif

    if (nonce_actual_size != NONCE_BYTES) {
        fprintf(stderr, "Nonce size mismatch: expected 64, got %d\n", nonce_actual_size);
        goto cleanup;
    }

    memcpy(preimage + 2 * COORDINATE_BYTES, nonce_bytes, NONCE_BYTES);

    printf("n (hex) = ");
    for (int i = 0; i < NONCE_BYTES; i++) {
        printf("%02X", nonce_bytes[i]);
    }
    printf("\n");
    printf("Nonce actual size: %d bytes\n", nonce_actual_size);

    // Step 9: Build 80-byte preimage P||n
    printf("\nStep 9: Preimage (P || nonce) as hex:\n");
    for (int i = 0; i < PREIMAGE_BYTES; i++) {
        printf("%02X", preimage[i]);
    }
    printf("\n");

    // Step 10: Compute α = H(P, n)
    printf("\nStep 10: Compute α = H(P, n)\n");

    ret = wc_InitSha256(&sha);
    if (ret != 0) {
        printf("Error initializing SHA-256: %d\n", ret);
        goto cleanup;
    }

    ret = wc_Sha256Update(&sha, preimage, PREIMAGE_BYTES);
    if (ret != 0) {
        printf("Error updating SHA-256: %d\n", ret);
        goto cleanup;
    }

    ret = wc_Sha256Final(&sha, hash);
    if (ret != 0) {
        printf("Error finalizing SHA-256: %d\n", ret);
        goto cleanup;
    }

    printf("α (as hex) = ");
    for (int i = 0; i < WC_SHA256_DIGEST_SIZE; i++) {
        printf("%02X", hash[i]);
    }
    printf("\n");

    // Convert hash to math_int_t
#if USE_SP_MATH
    sp_read_unsigned_bin(&alpha, hash, WC_SHA256_DIGEST_SIZE);
#else
    mp_read_unsigned_bin(&alpha, hash, WC_SHA256_DIGEST_SIZE);
#endif

    // Step 11: Verify proof g^v*h^w = P*COM^α
    printf("\nStep 11: Check if g^v*h^w = P*COM^α\n");

    EccPoint vg, wh, left_side, alpha_COM, right_side;

    // Initialize temporary points
    if (init_ecc_point(&vg) < 0 || init_ecc_point(&wh) < 0 ||
        init_ecc_point(&left_side) < 0 || init_ecc_point(&alpha_COM) < 0 ||
        init_ecc_point(&right_side) < 0) {
        printf("Error initializing temporary ECC points\n");
        ret = -1;
        goto cleanup;
    }

    // Left side: g^v * h^w = (v*g) + (w*h)
    printf("Computing left side: g^v * h^w\n");

    // Compute v*g
    printf("Computing v*g...\n");
    ret = ecc_point_mul_custom(&vg, &v, &g, &key);
    if (ret != 0) {
        printf("Error computing v*g: %d\n", ret);
        goto step11_cleanup;
    }
    print_ecc_point("v*g", &vg);

    // Compute w*h
    printf("Computing w*h...\n");
    ret = ecc_point_mul_custom(&wh, &w, &h, &key);
    if (ret != 0) {
        printf("Error computing w*h: %d\n", ret);
        goto step11_cleanup;
    }
    print_ecc_point("w*h", &wh);

    // Compute (v*g) + (w*h)
    printf("Computing (v*g) + (w*h)...\n");
    ret = ecc_point_add_custom(&left_side, &vg, &wh, &key);
    if (ret != 0) {
        printf("Error computing (v*g) + (w*h): %d\n", ret);
        goto step11_cleanup;
    }
    print_ecc_point("g^v * h^w", &left_side);

    // Right side: P * COM^α = P + α*COM
    printf("Computing right side: P * COM^α\n");

    // Compute α*COM
    printf("Computing α*COM...\n");
    ret = ecc_point_mul_custom(&alpha_COM, &alpha, &COM, &key);
    if (ret != 0) {
        printf("Error computing α*COM: %d\n", ret);
        goto step11_cleanup;
    }
    print_ecc_point("α*COM", &alpha_COM);

    // Compute P + α*COM
    printf("Computing P + α*COM...\n");
    ret = ecc_point_add_custom(&right_side, &P, &alpha_COM, &key);
    if (ret != 0) {
        printf("Error computing P + α*COM: %d\n", ret);
        goto step11_cleanup;
    }
    print_ecc_point("P * COM^α", &right_side);

    // Check equality
    printf("Comparing points for equality...\n");
    if (ecc_points_equal(&left_side, &right_side)) {
        printf("✅ Proof verifies: g^v·h^w = P·COM^α\n");
        ret = 0;
    } else {
        printf("❌ Proof FAILED: g^v·h^w ≠ P·COM^α\n");
        ret = -1;
    }

step11_cleanup:
    // Clean up temporary points
    free_ecc_point(&vg);
    free_ecc_point(&wh);
    free_ecc_point(&left_side);
    free_ecc_point(&alpha_COM);
    free_ecc_point(&right_side);

    printf("Computation complete\n");

cleanup:
    // Clean up
    free_ecc_point(&g);
    free_ecc_point(&h);
    free_ecc_point(&COM);
    free_ecc_point(&P);
#if USE_SP_MATH
    sp_clear(&nonce);
    sp_clear(&v);
    sp_clear(&w);
    sp_clear(&alpha);
#else
    mp_clear(&nonce);
    mp_clear(&v);
    mp_clear(&w);
    mp_clear(&alpha);
#endif
    wc_ecc_free(&key);

    return ret;
}

#ifdef STANDALONE
int main(int argc, char *argv[]) {
    Args args = {0};
    int opt;
    int option_index = 0;

    static struct option long_options[] = {
        {"gx", required_argument, 0, 1001},
        {"gy", required_argument, 0, 1002},
        {"hx", required_argument, 0, 1003},
        {"hy", required_argument, 0, 1004},
        {"COMx", required_argument, 0, 1005},
        {"COMy", required_argument, 0, 1006},
        {"Px", required_argument, 0, 1007},
        {"Py", required_argument, 0, 1008},
        {"nonce", required_argument, 0, 1009},
        {"v", required_argument, 0, 1010},
        {"w", required_argument, 0, 1011},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, "h", long_options, &option_index)) != -1) {
        switch (opt) {
            case 1001: args.gx = optarg; break;
            case 1002: args.gy = optarg; break;
            case 1003: args.hx = optarg; break;
            case 1004: args.hy = optarg; break;
            case 1005: args.COMx = optarg; break;
            case 1006: args.COMy = optarg; break;
            case 1007: args.Px = optarg; break;
            case 1008: args.Py = optarg; break;
            case 1009: args.nonce = optarg; break;
            case 1010: args.v = optarg; break;
            case 1011: args.w = optarg; break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    return verify_zk_proof(&args);
}
#else
char *bytes_to_hex_string(const uint8_t *data, size_t len, bool prefix_0x) {
    size_t hex_len = len * 2 + 1 + (prefix_0x ? 2 : 0); // 2 hex chars per byte + null + optional "0x"
    char *out = malloc(hex_len);
    if (!out) return NULL;

    char *p = out;

    if (prefix_0x) {
        *p++ = '0';
        *p++ = 'x';
    }

    for (size_t i = 0; i < len; ++i) {
        sprintf(p, "%02x", data[i]);
        p += 2;
    }

    *p = '\0';
    return out;
}

int verify(func_call_t *init, func_call_t *comm, func_call_t *proofs, data_portion_t *nonce) {
    Args args = {0};  // Use the real Args struct

    // Convert binary data to decimal strings
    args.gx    = bytes_to_hex_string(init->data_p[0].data, init->data_p[0].len, true);
    args.gy    = bytes_to_hex_string(init->data_p[1].data, init->data_p[1].len, true);
    args.hx    = bytes_to_hex_string(init->data_p[2].data, init->data_p[2].len, true);
    args.hy    = bytes_to_hex_string(init->data_p[3].data, init->data_p[3].len, true);

    args.COMx  = bytes_to_hex_string(comm->data_p[2].data, comm->data_p[2].len, true);
    args.COMy  = bytes_to_hex_string(comm->data_p[3].data, comm->data_p[3].len, true);

    args.Px    = bytes_to_hex_string(proofs->data_p[0].data, proofs->data_p[0].len, true);
    args.Py    = bytes_to_hex_string(proofs->data_p[1].data, proofs->data_p[1].len, true);
    args.nonce = bytes_to_hex_string(nonce->data, nonce->len, true);
    args.v     = bytes_to_hex_string(proofs->data_p[2].data, proofs->data_p[2].len, true);
    args.w     = bytes_to_hex_string(proofs->data_p[3].data, proofs->data_p[3].len, true);


    // Call the actual verifier
    int result = verify_zk_proof(&args);

    // Clean up
    free(args.gx); free(args.gy); free(args.hx); free(args.hy);
    free(args.COMx); free(args.COMy);
    free(args.Px); free(args.Py); free(args.nonce); free(args.v); free(args.w);

    return result;
}

#endif /*STANDALONE*/
