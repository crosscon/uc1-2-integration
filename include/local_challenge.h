#ifndef LOCAL_CHALLENGE_H
#define LOCAL_CHALLENGE_H
#include <unistd.h>
#include "common/challenge.h"

extern const uint8_t comm_cha_p1[LEN32];
extern const uint8_t comm_cha_p2[LEN32];
extern const uint8_t proofs_cha_p1[LEN32];
extern const uint8_t proofs_cha_p2[LEN32];
extern const uint8_t nonce[LEN64];

// extern const func_call_t init_call;
// extern const func_call_t comm_call;
// extern const func_call_t proofs_call;

#endif
