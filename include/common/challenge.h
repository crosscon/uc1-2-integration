#ifndef CHALLENGE_H
#define CHALLENGE_H

#include <stdint.h>
#include <stddef.h>
#include <wolfssl/ssl.h>

#define CHALLENGE_LEN   32
#define NONCE_LEN       64

typedef enum {
  GET_COMMITMENT = 69,
  GET_ZK_PROOFS
} func_t;

typedef struct {
  func_t func;
  const uint8_t* data_p1;
  const uint8_t* data_p2;
  const uint8_t* nonce;
} func_call_t;


int initFunc(func_call_t *func, func_t func_id);
void freeFunc(func_call_t *call);
int sendFramedStream(WOLFSSL *ssl, const uint8_t *data, size_t len);
int sendChallenge(WOLFSSL *ssl, func_call_t *const func);

extern const uint8_t comm_cha_p1[CHALLENGE_LEN];
extern const uint8_t comm_cha_p2[CHALLENGE_LEN];
extern const uint8_t proofs_cha_p1[CHALLENGE_LEN];
extern const uint8_t proofs_cha_p2[CHALLENGE_LEN];
extern const uint8_t nonce[NONCE_LEN];

extern const func_call_t comm_call;
extern const func_call_t proofs_call;

#endif // CHALLENGE_H
