#ifndef CHALLENGE_H
#define CHALLENGE_H

#include <stdint.h>
#include <stddef.h>
#include <wolfssl/ssl.h>

#define ID_LEN  4 // uint32_t
#define LEN32   32
#define LEN64   64
#define DATA_PORTIONS 4

#define PUF_TA_INIT_FUNC_ID           ((uint32_t)0x00112233)
#define PUF_TA_GET_COMMITMENT_FUNC_ID ((uint32_t)0x11223344)
#define PUF_TA_GET_ZK_PROOFS_FUNC_ID  ((uint32_t)0x22334455)

typedef uint32_t func_t;

typedef struct {
    uint8_t len;
    uint8_t * data;
} data_portion_t;

typedef struct {
  func_t func;
  data_portion_t data_p[DATA_PORTIONS];
} func_call_t;

extern const uint8_t pattern_init_commit[4];
extern const uint8_t pattern_proofs[4];

int initFunc(func_call_t* func, func_t func_id, const uint8_t pattern[DATA_PORTIONS]);
void freeFunc(func_call_t* call);
int sendFramedStream(WOLFSSL *ssl, const uint8_t *data, uint8_t len);
int sendChallenge(WOLFSSL *ssl, func_call_t *const func);
int sendResponse(WOLFSSL *ssl, func_call_t *const func);
int recChallenge(WOLFSSL* ssl, func_call_t * func);
int recResponse(WOLFSSL* ssl, func_call_t *func);

#endif // CHALLENGE_H
