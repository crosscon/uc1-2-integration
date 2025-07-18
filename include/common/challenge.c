#include "challenge.h"
#include <crypt.h>
#include <wolfssl/ssl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "transmission.h"

/* (De)Allocate mem */

int initFunc(func_call_t* func, func_t func_id) {
  if (!func)
    return 1;

  func->func = func_id;
  func->data_p1 = malloc(CHALLENGE_LEN);
  func->data_p2 = malloc(CHALLENGE_LEN);
  func->nonce   = malloc(NONCE_LEN);

  if (!func->data_p1 || !func->data_p2 || !func->nonce) {
      free((void*)func->data_p1);
      free((void*)func->data_p2);
      free((void*)func->nonce);
      return 1;
  }

  memset((void*)func->data_p1, 0, CHALLENGE_LEN);
  memset((void*)func->data_p2, 0, CHALLENGE_LEN);
  memset((void*)func->nonce,   0, NONCE_LEN);

  return 0;
}

void freeFunc(func_call_t* call) {
  if (!call)
    return;

  free((void*)call->data_p1);
  free((void*)call->data_p2);
  free((void*)call->nonce);

  call->data_p1 = NULL;
  call->data_p2 = NULL;
  call->nonce = NULL;
}

/* Send / Receive Challenges */

int sendFramedStream(WOLFSSL* ssl, const uint8_t* data, size_t len) {
  if (wolfSSL_write(ssl, START_SEQ, START_SEQ_LEN) != START_SEQ_LEN)
    return 1;
  if (wolfSSL_write(ssl, data, len) != (int)len)
    return 1;
  if (wolfSSL_write(ssl, STOP_SEQ, STOP_SEQ_LEN) != STOP_SEQ_LEN)
    return 1;
  return 0;
}

int sendChallenge(WOLFSSL* ssl, func_call_t * const func) {
  if (sendFramedStream(ssl, (const uint8_t*)&func->func, sizeof(func_t)))
    return 1;
  if (waitForAck(ssl))
    return 1;
  if (sendFramedStream(ssl, func->data_p1, CHALLENGE_LEN))
    return 1;
  if (waitForAck(ssl))
    return 1;
  if (sendFramedStream(ssl, func->data_p2, CHALLENGE_LEN))
    return 1;
  if (waitForAck(ssl))
    return 1;
  if (!func->nonce)
    return 0;
  if (sendFramedStream(ssl, func->nonce, NONCE_LEN))
    return 1;
  return 0;
}

int recChallenge(WOLFSSL* ssl, func_call_t * func) {
  uint8_t buffer[BUF_SIZE];
  func_t func_id;

  if (!func)
    return 1;
  if (recStream(ssl, buffer, sizeof(func_t)))
    return 1;
  if (sendAck(ssl))
    return 1;

  func_id = buffer[0];

  if (recStream(ssl, buffer, 4))
      return 1;
  if (sendAck(ssl))
    return 1;
  memcpy((void*)func->data_p1, buffer, CHALLENGE_LEN);

  if (recStream(ssl, buffer, 4))
      return 1;
  if (sendAck(ssl))
    return 1;
  memcpy((void*)func->data_p2, buffer, CHALLENGE_LEN);

  if (func_id != GET_ZK_PROOFS)
    return 0;

  if (recStream(ssl, buffer, 4))
      return 1;
  if (sendAck(ssl))
    return 1;
  memcpy((void*)func->nonce, buffer, CHALLENGE_LEN);

  return 0;
}

/* Challenges */

#ifndef IS_ZEPHYR

const uint8_t comm_cha_p1[CHALLENGE_LEN] = {
    0xD1, 0x33, 0x53, 0xE8, 0x6B, 0x41, 0xF9, 0x4C, 0x88, 0x77, 0xF6,
    0x8F, 0xB9, 0x5A, 0xAD, 0x0A, 0x35, 0x82, 0x06, 0x95, 0xE2, 0x03,
    0x74, 0x13, 0xBD, 0x57, 0xA9, 0xC4, 0x47, 0xDF, 0x11, 0xD9
};

const uint8_t comm_cha_p2[CHALLENGE_LEN] = {
    0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
    0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77
};

const uint8_t proofs_cha_p1[CHALLENGE_LEN] = {
    0xD1, 0x33, 0x53, 0xE8, 0x6B, 0x41, 0xF9, 0x4C,
    0x88, 0x77, 0xF6, 0x8F, 0xB9, 0x5A, 0xAD, 0x0A,
    0x35, 0x82, 0x06, 0x95, 0xE2, 0x03, 0x74, 0x13,
    0xBD, 0x57, 0xA9, 0xC4, 0x47, 0xDF, 0x11, 0xD9
};

const uint8_t proofs_cha_p2[CHALLENGE_LEN] = {
    0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
    0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77
};

const uint8_t nonce[NONCE_LEN] = {
    0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
    0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
    0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
    0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77
};

const func_call_t comm_call = {
    GET_COMMITMENT, comm_cha_p1, comm_cha_p2, NULL
};

const func_call_t proofs_call = {
    GET_ZK_PROOFS, proofs_cha_p1, proofs_cha_p2, nonce
};

#endif // IS_ZEPHYR
