/* client-tls.c
 *
 * Copyright (C) 2006-2020 wolfSSL Inc.
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

#define DEFAULT_PORT 12345

#define CA_FILE     "/root/ca-cert.pem"
#define CERT_FILE   "/root/client-cert.pem"
#define KEY_FILE    "/root/client-key.pem"
#define PRIV_KEY_ID  {0x01}

int main(int argc, char** argv)
{
    int                sockfd;
    struct sockaddr_in servAddr;
    char               buff[256];
    size_t             len;
    int                ret;
    unsigned char      privKeyId[] = PRIV_KEY_ID;

    /* declare wolfSSL objects */
    WOLFSSL_CTX* ctx;
    WOLFSSL*     ssl;
    WOLFSSL_CIPHER* cipher;

    const char* library = "/usr/lib/libckteec.so";
    const char* tokenName = "ClientToken";
    const char* userPin = "1234";
    Pkcs11Dev dev;
    Pkcs11Token token;
    int slotId = 1;
    int devId = 1;

    /* Check for proper calling convention */
    if (argc != 2) {
        printf("usage: %s <IPv4 address>\n", argv[0]);
        return 0;
    }

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

    ret = wc_CryptoDev_RegisterDevice(devId, wc_Pkcs11_CryptoDevCb, &token);
    if (ret != 0) {
      fprintf(stderr, "Failed to register PKCS#11 token\n");
      return ret;
    }

    /* Create a socket that uses an internet IPv4 address,
     * Sets the socket to be stream based (TCP),
     * 0 means choose the default protocol. */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "ERROR: failed to create the socket\n");
        ret = -1;
        goto end;
    }

    /* Initialize the server address struct with zeros */
    memset(&servAddr, 0, sizeof(servAddr));

    /* Fill in the server address */
    servAddr.sin_family = AF_INET;             /* using IPv4      */
    servAddr.sin_port   = htons(DEFAULT_PORT); /* on DEFAULT_PORT */

    /* Get the server IPv4 address from the command line call */
    if (inet_pton(AF_INET, argv[1], &servAddr.sin_addr) != 1) {
        fprintf(stderr, "ERROR: invalid address\n");
        ret = -1;
        goto end;
    }

    /* Connect to the server */
    ret = connect(sockfd, (struct sockaddr*) &servAddr, sizeof(servAddr));
    if (ret == -1) {
        fprintf(stderr, "ERROR: failed to connect\n");
        goto end;
    }

    /*---------------------------------*/
    /* Start of wolfSSL initialization and configuration */
    /*---------------------------------*/
    /* Initialize wolfSSL */
    ret = wolfSSL_Init();
    if (ret != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: Failed to initialize the library\n");
        goto socket_cleanup;
    }

    /* Create and initialize WOLFSSL_CTX */
#ifdef USE_TLSV13
    ctx = wolfSSL_CTX_new(wolfTLSv1_3_client_method());
#else
    ctx = wolfSSL_CTX_new(wolfTLSv1_2_client_method());
#endif
    if (ctx == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL_CTX\n");
        ret = -1;
        goto socket_cleanup;
    }

    if (wolfSSL_CTX_set_cipher_list(ctx, "ECDHE-ECDSA-AES256-GCM-SHA384") != WOLFSSL_SUCCESS) {
        fprintf(stderr, "Failed to set cipher list\n");
        ret = -1;
        goto ctx_cleanup;
    }

    /* Set devId associated with the PKCS11 device. */
    if (wolfSSL_CTX_SetDevId(ctx, devId) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL_CTX\n");
        ret = -1;
        goto ctx_cleanup;
    }

    /* Mutual Authentication */
    /* Load client certificate into WOLFSSL_CTX */
    ret = wolfSSL_CTX_use_certificate_file(ctx, CERT_FILE, WOLFSSL_FILETYPE_PEM);
    if (ret != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load %s, please check the file (%d).\n",
                CERT_FILE, ret);
        goto ctx_cleanup;
    }

    /* Load client key into WOLFSSL_CTX */
    if (wolfSSL_CTX_use_PrivateKey_Id(ctx, privKeyId, sizeof(privKeyId), devId) != SSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to set id.\n");
        ret = -1;
        goto ctx_cleanup;
    }

    /* Load CA certificate into WOLFSSL_CTX for validating peer */
    ret = wolfSSL_CTX_load_verify_locations(ctx, CA_FILE, NULL);
    if (ret != WOLFSSL_SUCCESS) {
        char *errString = wc_GetErrorString(ret);
        fprintf(stderr, "ERROR: failed to load %s, please check the file (%d).\n",
                CA_FILE, ret);
        fprintf(stderr, "wolfSSL error: %s (%d)\n", errString, ret);
        goto ctx_cleanup;
    }

    /* validate peer certificate */
    wolfSSL_CTX_set_verify(ctx, WOLFSSL_VERIFY_PEER, NULL);

    /* Open the PKCS11 token. */
    ret = wc_Pkcs11Token_Open(&token, 1);
    if (ret != 0) {
        fprintf(stderr, "ERROR: failed to open session on token (%d)\n", ret);
        return -1;
    }

    /* Create a WOLFSSL object */
    if ((ssl = wolfSSL_new(ctx)) == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL object\n");
        ret = -1;
        goto ctx_cleanup;
    }

    /* Attach wolfSSL to the socket */
    ret = wolfSSL_set_fd(ssl, sockfd);
    if (ret != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: Failed to set the file descriptor\n");
        goto cleanup;
    }

    /* Connect to wolfSSL on the server side */
    ret = wolfSSL_connect(ssl);
    if (ret != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to connect to wolfSSL\n");
        goto cleanup;
    }

    cipher = wolfSSL_get_current_cipher(ssl);
    printf("SSL cipher suite is %s\n", wolfSSL_CIPHER_get_name(cipher));

    /* Get a message for the server from stdin */
    printf("Message for server: ");
    memset(buff, 0, sizeof(buff));
    if (fgets(buff, sizeof(buff), stdin) == NULL) {
        fprintf(stderr, "ERROR: failed to get message for server\n");
        ret = -1;
        goto cleanup;
    }
    len = strnlen(buff, sizeof(buff));

    /* Send the message to the server */
    ret = wolfSSL_write(ssl, buff, len);
    if (ret != len) {
        fprintf(stderr, "ERROR: failed to write entire message\n");
        fprintf(stderr, "%d bytes of %d bytes were sent", ret, (int) len);
        goto cleanup;
    }

    /* Read the server data into our buff array */
    memset(buff, 0, sizeof(buff));
    ret = wolfSSL_read(ssl, buff, sizeof(buff)-1);
    if (ret == -1) {
      fprintf(stderr, "ERROR: failed to read\n");
      goto cleanup;
    }

    /* Print to stdout any data the server sends */
    printf("Server: %s\n", buff);

    /* Bidirectional shutdown */
    while (wolfSSL_shutdown(ssl) == WOLFSSL_SHUTDOWN_NOT_DONE) {
        printf("Shutdown not complete\n");
    }

    printf("Shutdown complete\n");

    ret = 0;

    /* Cleanup and return */
    wc_Pkcs11Token_Final(&token);
    wc_Pkcs11_Finalize(&dev);

cleanup:
  wolfSSL_free(ssl); /* Free the wolfSSL object                  */
ctx_cleanup:
  wolfSSL_CTX_free(ctx);  /* Free the wolfSSL context object          */
  wolfSSL_Cleanup();      /* Cleanup the wolfSSL environment          */
socket_cleanup:
  close(sockfd); /* Close the connection to the server       */
end:
  return ret; /* Return reporting a success               */
}
