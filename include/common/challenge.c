#include "challenge.h"
#include <wolfssl/ssl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "transmission.h"
#include "log.h"

#ifdef IS_ZEPHYR
  #include <zephyr/logging/log.h>
  #include <zephyr/kernel.h>

  LOG_MODULE_REGISTER(challenge);
#endif

const uint8_t pattern_init_commit[4] = {32, 32, 32, 32};
const uint8_t pattern_proofs[4] = {32, 32, 64, 64};

/* Workarounds */

// This function is a workaround for multiple buffering layers on LPC side
void waitASec() {
const uint8_t time = 1; // second

#ifdef IS_ZEPHYR
    k_msleep(time * 1000);
#else
    sleep(time);
#endif
}


/* (De)Allocate mem */

int initFunc(func_call_t* func, func_t func_id, const uint8_t pattern[DATA_PORTIONS]) {
    if (!func || !pattern)
        return 1;

    func->func = func_id;

    for (int i = 0; i < DATA_PORTIONS; i++) {
        func->data_p[i].len = pattern[i];

        if (pattern[i] > 0) {
            func->data_p[i].data = malloc(pattern[i]);
            if (!func->data_p[i].data) {
                for (int j = 0; j < i; j++) {
                    free(func->data_p[j].data);
                    func->data_p[j].data = NULL;
                    func->data_p[j].len = 0;
                }
                return 1;
            }
            memset(func->data_p[i].data, 0, pattern[i]);
            continue;
        }

        func->data_p[i].data = NULL;
    }

    return 0;
}

void freeFunc(func_call_t* call) {
    if (!call)
        return;

    for (int i = 0; i < DATA_PORTIONS; i++) {
        free(call->data_p[i].data);
        call->data_p[i].data = NULL;
        call->data_p[i].len = 0;
    }
}

/* Send / Receive Challenges */

int sendFramedStream(WOLFSSL* ssl, const uint8_t* data, uint8_t len) {
  if (wolfSSL_write(ssl, START_SEQ, START_SEQ_LEN) != START_SEQ_LEN)
    return 1;
  if (wolfSSL_write(ssl, data, len) != (int)len)
    return 1;
  if (wolfSSL_write(ssl, STOP_SEQ, STOP_SEQ_LEN) != STOP_SEQ_LEN)
    return 1;
  return 0;
}

int sendChallenge(WOLFSSL *ssl, func_call_t *const func) {
    if (!ssl || !func)
        return 1;

    LOCAL_LOG_DBG("Sending func id");
    if (sendFramedStream(ssl, (const uint8_t *)&func->func, ID_LEN))
        return 1;

    waitASec();
    LOCAL_LOG_DBG("Waiting for ack");
    if (waitForAck(ssl))
        return 1;

    for (int i = 0; i < DATA_PORTIONS; i++) {
        if (func->data_p[i].data && func->data_p[i].len > 0) {
            LOCAL_LOG_DBG("Sending data portion: %d", i);
            if (sendFramedStream(ssl, func->data_p[i].data, func->data_p[i].len))
                return 1;

            LOCAL_LOG_HEXDUMP_DBG(func->data_p[i].data, func->data_p[i].len, "Sent:");
            waitASec();

            LOCAL_LOG_DBG("Waiting for ack");
            if (waitForAck(ssl))
                return 1;
        }
    }

    LOCAL_LOG_DBG("sendChallenge() successful");
    return 0;
}

int sendResponse(WOLFSSL *ssl, func_call_t *const func) {
    return sendChallenge(ssl, func);
}


int recChallenge(WOLFSSL* ssl, func_call_t *func) {
    uint8_t buffer[BUF_SIZE] = {0};
    uint32_t id = 0;

    if (!ssl || !func)
        return 1;

    waitASec();
    LOCAL_LOG_DBG("Receiving the stream for func id");
    if (recStream(ssl, buffer, ID_LEN))
        return 1;

    LOCAL_LOG_DBG("Sending ack");
    if (sendAck(ssl))
        return 1;

    memcpy(&id, buffer, sizeof(uint32_t));
    func->func = id;
    LOCAL_LOG_DBG("Func id id 0x%08X", func->func);

    for (int i = 0; i < DATA_PORTIONS; i++) {
        if (func->data_p[i].data && func->data_p[i].len > 0) {
            waitASec();
            LOCAL_LOG_DBG("Receiving data portion: %d", i);
            if (recStream(ssl, buffer, func->data_p[i].len))
                return 1;

            LOCAL_LOG_DBG("Sending ack");
            if (sendAck(ssl))
                return 1;

            memcpy(func->data_p[i].data, buffer, func->data_p[i].len);
            LOCAL_LOG_HEXDUMP_DBG(func->data_p[i].data, func->data_p[i].len, "Rec:");
        }
    }

    LOCAL_LOG_DBG("recChallenge() successful!");
    return 0;
}

int recResponse(WOLFSSL* ssl, func_call_t *func) {
    return recChallenge(ssl, func);
}

/* Challenges */
