#ifndef PUF_VERIFIER_H
#define PUF_VERIFIER_H
#include "common/challenge.h"

int verify(func_call_t *init, func_call_t *comm, func_call_t *proofs, data_portion_t *nonce);

#endif
