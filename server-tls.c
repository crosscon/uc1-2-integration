/* server-tls.c
 *
 * Copyright (C) 2006-2020 wolfSSL Inc.
 * Copyright (C) 2025 3mdeb Sp. z o.o.
 *
 * This file is part of wolfSSL. (formerly known as CyaSSL)
 *
 * wolfSSL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * wolfSSL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

/* the usual suspects */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* socket includes */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

/* wolfSSL */
#include <wolfssl/options.h>
#include <wolfssl/ssl.h>
#include <wolfssl/wolfcrypt/wc_pkcs11.h>

#include "include/common/log.h"
#ifdef NXP_PUF
  #include "include/common/challenge.h"
  #include "include/local_challenge.h"
  #include "include/puf_verifier.h"
#endif
#ifdef RPI_CBA
  #include "include/common/tee_client_api.h"
  #include "include/common/context_based_authentication.h"
  #include "include/common/challenge.h"
#endif

#define DEFAULT_PORT 12345

#define CA_FILE     "/root/ca-cert.pem"
#define PREFIX      "server"
#define CERT_FILE   "/root/" PREFIX "-cert.pem"
#define KEY_FILE    "/root/" PREFIX "-key.pem"
#define SLOT_ID 0
#define PRIV_KEY_ID  {0x01}


#ifdef RPI_CBA
TEEC_Result CBAGenerateNonce(char* nonce, size_t nonce_size) {
    TEEC_Result res;
    TEEC_Context ctx;
    TEEC_Session sess;
    TEEC_Operation op;
    TEEC_UUID uuid = TA_CONTEXT_BASED_AUTHENTICATION_UUID;
    uint32_t err_origin;

    printf("Generating a nonce...\n");

    res = TEEC_InitializeContext(NULL, &ctx);
    if (res != TEEC_SUCCESS) {
      printf("TEEC_InitializeContext failed with code 0x%x", res);
      return res;
    }

    res = TEEC_OpenSession(&ctx, &sess, &uuid, TEEC_LOGIN_PUBLIC, NULL, NULL, &err_origin);
    if (res != TEEC_SUCCESS) {
      fprintf(stderr, "TEEC_Opensession failed with code 0x%x origin 0x%x", res, err_origin);
      return res;
    }

    memset(&op, 0, sizeof(op));

    op.paramTypes = TEEC_PARAM_TYPES(
      TEEC_MEMREF_TEMP_OUTPUT,
      TEEC_NONE,
      TEEC_NONE,
      TEEC_NONE
    );

    op.params[0].tmpref.buffer = nonce;
    op.params[0].tmpref.size = nonce_size;

    res = TEEC_InvokeCommand(&sess, TA_CONTEXT_BASED_AUTHENTICATION_CMD_GET_NONCE, &op, &err_origin);
    if (res != TEEC_SUCCESS) {
      fprintf(stderr, "TEEC_InvokeCommand failed with code 0x%x, origin 0x%x", res, err_origin);
      return res;
    }

    printf("TA result: %x %x ... %x\n", nonce[0], nonce[1], nonce[15]);

    TEEC_CloseSession(&sess);
    TEEC_FinalizeContext(&ctx);

    return TEEC_SUCCESS;
}

TEEC_Result CBAVerifySignature(char* nonce, size_t nonce_size, char* signature, size_t signature_size) {
    TEEC_Result res;
    TEEC_Context ctx;
    TEEC_Session sess;
    TEEC_Operation op;
    TEEC_UUID uuid = TA_CONTEXT_BASED_AUTHENTICATION_UUID;
    uint32_t err_origin;

    printf("Verifying a signature...\n");

    res = TEEC_InitializeContext(NULL, &ctx);
    if (res != TEEC_SUCCESS) {
      fprintf(stderr, "TEEC_InitializeContext failed with code 0x%x", res);
      return res;
    }

    res = TEEC_OpenSession(&ctx, &sess, &uuid, TEEC_LOGIN_PUBLIC, NULL, NULL, &err_origin);
    if (res != TEEC_SUCCESS) {
      fprintf(stderr, "TEEC_Opensession failed with code 0x%x origin 0x%x", res, err_origin);
      return res;
    }

    memset(&op, 0, sizeof(op));

    op.paramTypes = TEEC_PARAM_TYPES(
      TEEC_MEMREF_TEMP_INPUT,
      TEEC_MEMREF_TEMP_INPUT,
      TEEC_NONE,
      TEEC_NONE
    );

    op.params[0].tmpref.buffer = nonce;
    op.params[0].tmpref.size = nonce_size;
    op.params[1].tmpref.buffer = signature;
    op.params[1].tmpref.size = signature_size;

    res = TEEC_InvokeCommand(&sess, TA_CONTEXT_BASED_AUTHENTICATION_CMD_VERIFY, &op, &err_origin);
    if (res != TEEC_SUCCESS) {
      fprintf(stderr, "TEEC_InvokeCommand failed with code 0x%x, origin 0x%x", res, err_origin);
      return res;
    }

    printf("TA result: Ok\n");

    TEEC_CloseSession(&sess);
    TEEC_FinalizeContext(&ctx);

    return TEEC_SUCCESS;
}
#endif /* RPI_CBA */

int main()
{
    int                sockfd = SOCKET_INVALID;
    int                connd = SOCKET_INVALID;
    struct sockaddr_in servAddr;
    struct sockaddr_in clientAddr;
    socklen_t          size = sizeof(clientAddr);
    char               buff[256];
    size_t             len;
    int                shutdown = 0;
    int                ret;
    const char*        reply = "Hello from WolfSSL TLS server!\n";
    char               wolfsslErrorStr[80];

#ifdef RPI_CBA
    const char* library = "/usr/lib/libckteec2.so";
#else
    const char* library = "/usr/lib/libckteec.so";
#endif /* ifdef RPI_CBA */
    const char* tokenName = "ServerToken";
    const char* userPin = "1234";
    Pkcs11Dev dev;
    Pkcs11Token token;
    int slotId = SLOT_ID;
    int devId = 1;
    unsigned char      privKeyId[] = PRIV_KEY_ID;

    /* declare wolfSSL objects */
    WOLFSSL_CTX* ctx = NULL;
    WOLFSSL*     ssl = NULL;
    WOLFSSL_CIPHER *cipher;

#ifdef NXP_PUF
    /* Challenges*/
    func_call_t initCh;
    func_call_t commCh;
    func_call_t proofsCh;
    data_portion_t nonceP;
#endif

#ifdef RPI_CBA
    char CBANonce[CBA_NONCE_SIZE];
    char* CBASignature;
    size_t CBASignatureSize;

    func_call_t CBARequest, CBAResponce;
    /* Are needed for initFunc(). */
    const uint8_t CBASignaturePatternSize[DATA_PORTIONS] = {(uint8_t)CBA_SIGNATURE_BUFFER_SIZE};
    const uint8_t CBANoncePatternSize[DATA_PORTIONS] = {(uint8_t)CBA_NONCE_SIZE};
#endif

#ifndef NXP_PUF
    fprintf(stdout, "App compiled for dual RPI demo!\n");
#else
    fprintf(stdout, "App compiled for NXP demo!\n");
#endif
#ifdef DEBUG
    fprintf(stdout, "Debug enabled!\n");
#endif

    wolfCrypt_Init();

    ret = wc_Pkcs11_Initialize(&dev, library, NULL);
    if (ret != 0) {
      fprintf(stderr, "Failed to initialize PKCS#11 library\n");
      return ret;
    }

    ret = wc_Pkcs11Token_Init(&token, &dev, slotId, tokenName, (byte *)userPin,
                              strlen(userPin));
    if (ret != 0) {
      fprintf(stderr, "Failed to initialize PKCS#11 token\n");
      return ret;
    }

    ret = wc_CryptoDev_RegisterDevice(devId, wc_Pkcs11_CryptoDevCb,
                                              &token);
    if (ret != 0) {
      fprintf(stderr, "Failed to register PKCS#11 token\n");
      return ret;
    }

    /* Initialize wolfSSL */
    wolfSSL_Init();

    /* Create a socket that uses an internet IPv4 address,
     * Sets the socket to be stream based (TCP),
     * 0 means choose the default protocol. */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        fprintf(stderr, "ERROR: failed to create the socket\n");
        ret = -1;
        goto exit;
    }

    /* Create and initialize WOLFSSL_CTX */
#ifdef USE_TLSV13
    LOCAL_LOG_DBG("Using TLS v1.3\n");
    ctx = wolfSSL_CTX_new(wolfTLSv1_3_server_method());
#else
    LOCAL_LOG_DBG("using TLS v1.2\n");
    ctx = wolfSSL_CTX_new(wolfTLSv1_2_server_method());
#endif
    if (ctx == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL_CTX\n");
        ret = -1;
        goto exit;
    }

    /* Load server certificates into WOLFSSL_CTX */
    ret = wolfSSL_CTX_use_certificate_file(ctx, CERT_FILE, SSL_FILETYPE_PEM);
    if (ret != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load %s, please check the file.\n",
                CERT_FILE);
        ret = -1;
        goto exit;
    }

    if (wolfSSL_CTX_SetDevId(ctx, devId) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL_CTX\n");
        ret = -1;
        goto exit;
    }

    /* Load server key into WOLFSSL_CTX */
    if (wolfSSL_CTX_use_PrivateKey_Id(ctx, privKeyId, sizeof(privKeyId), devId) != SSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to set id.\n");
        ret = -1;
        goto exit;
    }

    /* Load CA certificate into WOLFSSL_CTX for validating peer */
    ret = wolfSSL_CTX_load_verify_locations(ctx, CA_FILE, NULL);
    if (ret != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load %s, please check the file.\n",
                CA_FILE);
        goto exit;
    }

    /* enable mutual authentication */
    wolfSSL_CTX_set_verify(ctx,
        WOLFSSL_VERIFY_PEER | WOLFSSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);

    /* Initialize the server address struct with zeros */
    memset(&servAddr, 0, sizeof(servAddr));

    /* Fill in the server address */
    servAddr.sin_family      = AF_INET;             /* using IPv4      */
    servAddr.sin_port        = htons(DEFAULT_PORT); /* on DEFAULT_PORT */
    servAddr.sin_addr.s_addr = INADDR_ANY;          /* from anywhere   */

    /* Bind the server socket to our port */
    if (bind(sockfd, (struct sockaddr*)&servAddr, sizeof(servAddr)) == -1) {
        fprintf(stderr, "ERROR: failed to bind\n");
        ret = -1;
        goto exit;
    }

    /* Listen for a new connection, allow 5 pending connections */
    if (listen(sockfd, 5) == -1) {
        fprintf(stderr, "ERROR: failed to listen\n");
        ret = -1;
        goto exit;
    }

    /* Continue to accept clients until shutdown is issued */
    while (!shutdown) {
        printf("Waiting for a connection...\n");

        /* Accept client connections */
        connd = accept(sockfd, (struct sockaddr*)&clientAddr, &size);
        if (connd == -1) {
            fprintf(stderr, "ERROR: failed to accept the connection\n\n");
            ret = -1;
            goto exit;
        }

        /* Create a WOLFSSL object */
        ret = wc_Pkcs11Token_Open(&token, 1);
        if (ret != 0) {
            fprintf(stderr, "ERROR: failed to open session on token (%d)\n",
                ret);
           goto exit;
        }

        /* Create a WOLFSSL object */
        ssl = wolfSSL_new(ctx);
        if (ssl == NULL) {
            fprintf(stderr, "ERROR: failed to create WOLFSSL object\n");
            ret = -1;
            goto exit;
        }

        /* Attach wolfSSL to the socket */
        wolfSSL_set_fd(ssl, connd);

        /* Establish TLS connection */
        ret = wolfSSL_accept(ssl);
        printf("Server accept status: %d\n", ret);
        if (ret != WOLFSSL_SUCCESS) {
            wolfSSL_ERR_error_string(wolfSSL_get_error(ssl, ret), wolfsslErrorStr);
            fprintf(stderr, "wolfSSL_accept error: %s\n", wolfsslErrorStr);
            goto exit;
        }

        printf("Client connected successfully\n");

        cipher = wolfSSL_get_current_cipher(ssl);
        printf("SSL cipher suite is %s\n", wolfSSL_CIPHER_get_name(cipher));

#ifdef NXP_PUF
        // Init init challenge
        initFunc(&initCh, PUF_TA_INIT_FUNC_ID, pattern_init_commit);

        // Init commitment challenge
        initFunc(&commCh, PUF_TA_GET_COMMITMENT_FUNC_ID, pattern_init_commit);
        memcpy(commCh.data_p[0].data, comm_cha_p1, commCh.data_p[0].len);
        memcpy(commCh.data_p[1].data, comm_cha_p2, commCh.data_p[1].len);

        // Init proofs challenge
        initFunc(&proofsCh, PUF_TA_GET_ZK_PROOFS_FUNC_ID, pattern_proofs);
        memcpy(proofsCh.data_p[0].data, proofs_cha_p1, proofsCh.data_p[0].len);
        memcpy(proofsCh.data_p[1].data, proofs_cha_p2, proofsCh.data_p[1].len);
        memcpy(proofsCh.data_p[2].data, nonce, proofsCh.data_p[2].len);

        if (sendChallenge(ssl, (void *)&initCh)) {
          fprintf(stderr, "ERROR: init sendChallenge() failed!\n");
          goto exit;
        }

        if (sendChallenge(ssl, (void *)&commCh)) {
          fprintf(stderr, "ERROR: commitment sendChallenge() failed!\n");
          goto exit;
        }

        if (sendChallenge(ssl, (void *)&proofsCh)) {
          fprintf(stderr, "ERROR: proofs sendChallenge() failed!\n");
          goto exit;
        }

        /* Wait until challenges are processed on PUF */

        if (recResponse(ssl, (void *)&initCh)) {
          fprintf(stderr, "ERROR: recResponse() for init failed!\n");
          goto exit;
        }

        if (recResponse(ssl, (void *)&commCh)) {
          fprintf(stderr, "ERROR: recResponse() for commitment failed!\n");
          goto exit;
        }

        if (recResponse(ssl, (void *)&proofsCh)) {
          fprintf(stderr, "ERROR: second recResponse() for proofs failed!\n");
          goto exit;
        }

        nonceP.len = LEN64;
        nonceP.data = malloc(LEN64);
        if (!nonceP.data) {
          fprintf(stderr, "Error: Failed to allocate memory for nonce!\n");
          goto exit;
        }
        memcpy(nonceP.data, nonce, LEN64);

        if (verify(&initCh, &commCh, &proofsCh, &nonceP)) {
          fprintf(stderr, "Error: Could not verify PUF authenticity.\n");
          goto exit;
        }
#endif /* NXP_PUF */

#ifdef RPI_CBA
        memcpy(CBANonce, 0, (size_t)CBA_NONCE_SIZE);

        // Generate CBA nonce:
        if (CBAGenerateNonce(CBANonce, (size_t)CBA_NONCE_SIZE)) {
          fprintf(stderr, "ERROR: CBAGenerateNonce() failed!\n");
          goto exit;
        }

        initFunc(&CBARequest, CBA_PROVE_IDENTITY, CBANoncePatternSize);
        memcpy(CBARequest.data_p[0].data, CBANonce, (size_t)CBARequest.data_p[0].len);

        initFunc(&CBAResponce, 0, CBASignaturePatternSize);
        memcpy(CBAResponce.data_p[0].data, 0, (size_t)CBAResponce.data_p[0].len);

        if (sendChallenge(ssl, &CBARequest)) {
          fprintf(stderr, "ERROR: sendChallenge() failed!\n");
          goto exit;
        }

        if (recResponse(ssl, &CBAResponce)) {
          fprintf(stderr, "ERROR: recResponse() failed!\n");
          goto exit;
        }

        CBASignatureSize = strlen(CBAResponce.data_p[0].data);
        CBASignature = malloc(CBASignatureSize);
        if (!CBASignature) {
          fprintf(stderr, "ERROR: Context-Based Authentication signature buffer allocation failed!\n");
          goto exit;
        }
        memcpy(CBASignature, CBASignatureSize, CBAResponce.data_p[0].data);

        if (CBAVerifySignature(CBANonce, CBANonceSize, CBASignature, CBASignatureSize)) {
          fprintf(stderr, "ERROR: CBAVerifySignature() dailed!\n");
          goto exit;
        }

#endif /* RPI_CBA */

        /* Read the client data into our buff array */
        memset(buff, 0, sizeof(buff));
        ret = wolfSSL_read(ssl, buff, sizeof(buff) - 1);
        if (ret == -1) {
            fprintf(stderr, "ERROR: failed to read\n");
            goto exit;
        }

        /* Print to stdout any data the client sends */
        printf("Client: %s\n", buff);

        /* Check for server shutdown command */
        if (strncmp(buff, "shutdown", 8) == 0) {
            printf("Shutdown command issued!\n");
            shutdown = 1;
        }

        /* Write our reply into buff */
        memset(buff, 0, sizeof(buff));
        memcpy(buff, reply, strlen(reply));
        len = strnlen(buff, sizeof(buff));

        /* Reply back to the client */
        ret = wolfSSL_write(ssl, buff, len);
        if (ret != len) {
            fprintf(stderr, "ERROR: failed to write\n");
            goto exit;
        }

        /* Notify the client that the connection is ending */
        wolfSSL_shutdown(ssl);
        printf("Shutdown complete\n");

        /* Cleanup after this connection */
        wolfSSL_free(ssl);      /* Free the wolfSSL object              */
        ssl = NULL;
        wc_Pkcs11Token_Close(&token);
        close(connd);           /* Close the connection to the client   */
    }

    ret = 0;
    wc_Pkcs11Token_Close(&token);
    wc_Pkcs11Token_Final(&token);
    wc_Pkcs11_Finalize(&dev);

exit:
#ifdef NXP_PUF
    freeFunc(&initCh);
    freeFunc(&commCh);
    freeFunc(&proofsCh);
    free(nonceP.data);
#endif

#ifdef RPI_CBA
    freeFunc(&CBARequest);
    freeFunc(&CBAResponce);
    free(CBASignature);
#endif
    /* Cleanup and return */
    if (ssl)
        wolfSSL_free(ssl);      /* Free the wolfSSL object              */
    if (connd != SOCKET_INVALID)
        close(connd);           /* Close the connection to the client   */
    if (sockfd != SOCKET_INVALID)
        close(sockfd);          /* Close the socket listening for clients   */
    if (ctx)
        wolfSSL_CTX_free(ctx);  /* Free the wolfSSL context object          */
    wolfSSL_Cleanup();          /* Cleanup the wolfSSL environment          */
    wolfCrypt_Cleanup();
    return ret;               /* Return reporting a success               */
}
