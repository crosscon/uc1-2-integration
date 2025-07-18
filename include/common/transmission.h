#ifndef WOLFSSL_COMM_H
#define WOLFSSL_COMM_H

#include <wolfssl/ssl.h>
#include <stdint.h>
#include <stddef.h> // for size_t

// Start and stop sequences for framing binary streams
#define START_SEQ_LEN 4
#define STOP_SEQ_LEN 4

#define BUF_SIZE 256

extern const uint8_t START_SEQ[START_SEQ_LEN];
extern const uint8_t STOP_SEQ[STOP_SEQ_LEN];

// Reads exactly len bytes into buf, blocking until done or error
// Returns 0 on success, 1 on error
static int readExact(WOLFSSL* ssl, uint8_t* buf, size_t len);

// Matches buf against seq for len bytes
// Returns non-zero if equal, zero otherwise
static int matchSeq(const uint8_t* buf, const uint8_t* seq, int len);

// Blocking function to receive a framed binary stream:
// waits for start sequence, reads payload_len bytes, waits for stop sequence
// Returns 0 on success, 1 on error
int recStream(WOLFSSL* ssl, uint8_t* out_buf, size_t payload_len);

// Blocking function to send the ASCII "ACK" string reliably
// Returns 0 on success, 1 on error
int sendAck(WOLFSSL* ssl);

// Blocking function to wait for the ASCII "ACK" string from peer
// Returns 0 on success (ACK received), 1 on error or mismatch
int waitForAck(WOLFSSL* ssl);

#endif // WOLFSSL_COMM_H
