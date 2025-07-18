#include <wolfssl/ssl.h>
#include <string.h>
#include <stdio.h>
#include "transmission.h"

const uint8_t START_SEQ[START_SEQ_LEN] = {0x55, 0x55, 0x55, 0x55};
const uint8_t STOP_SEQ[STOP_SEQ_LEN]  = {0xFF, 0xFF, 0xFF, 0xFF};

const char ACK_STR[] = "ACK";
#define ACK_LEN 3

/* Transmission confirm */

int waitForAck(WOLFSSL* ssl) {
  char buf[ACK_LEN] = {0};
  int total_read = 0;

  while (total_read < ACK_LEN) {
    int ret = wolfSSL_read(ssl, buf + total_read, ACK_LEN - total_read);
    int err;

    if (ret > 0) {
      total_read += ret;
      continue;
    }

    err = wolfSSL_get_error(ssl, ret);

    if (err != WOLFSSL_ERROR_WANT_READ)
      return 1;
  }

  if (strncmp(buf, ACK_STR, ACK_LEN) == 0)
    return 0;
  return 1;
}

int sendAck(WOLFSSL* ssl) {
  size_t total_sent = 0;

  while (total_sent < ACK_LEN) {
    int ret = wolfSSL_write(ssl, ACK_STR + total_sent, ACK_LEN - total_sent);

    if (ret > 0) {
        total_sent += ret;
        continue;
    }
    if (ret == 0)
      return 1;

    int err = wolfSSL_get_error(ssl, ret);
    if (err != WOLFSSL_ERROR_WANT_WRITE)
      return 1;
  }

  return 0;
}

/* Decode transmission */

static int matchSeq(const uint8_t* buf, const uint8_t* seq, int len) {
    return memcmp(buf, seq, len) == 0;
}

static int readExact(WOLFSSL* ssl, uint8_t* buf, size_t len) {
  size_t total_read = 0;

  while (total_read < len) {
    int ret = wolfSSL_read(ssl, buf + total_read, len - total_read);
    int err;

    if (ret == 0)
      return 1;

    if (ret > 0) {
      total_read += ret;
      continue;
    }

    err = wolfSSL_get_error(ssl, ret);
    if (err != WOLFSSL_ERROR_WANT_READ)
      return 1;
  }

  return 0;
}

int recStream(WOLFSSL* ssl, uint8_t* out_buf, size_t payload_len) {
  uint8_t seq_buf[START_SEQ_LEN];
  int idx = 0;
  int err;

  while (1) {
    int ret = wolfSSL_read(ssl, &seq_buf[idx], 1);

    if (ret == 1) {
      if (idx < START_SEQ_LEN - 1) {
        idx++;
        continue;
      }

      if (matchSeq(seq_buf, START_SEQ, START_SEQ_LEN)) {
        break;
      }

      memmove(seq_buf, seq_buf + 1, START_SEQ_LEN - 1);
      idx = START_SEQ_LEN - 1;

      continue;
    }
    if (ret == 0)
      return 1;

    err = wolfSSL_get_error(ssl, ret);

    if (err != WOLFSSL_ERROR_WANT_READ)
      return 1;
  }

  if (readExact(ssl, out_buf, payload_len) < 0)
      return 1;
  if (readExact(ssl, seq_buf, STOP_SEQ_LEN) < 0)
      return 1;
  if (!matchSeq(seq_buf, STOP_SEQ, STOP_SEQ_LEN))
      return 1;
  return 0;
}
