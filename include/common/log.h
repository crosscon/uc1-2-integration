#ifndef LOCAL_LOG_H
#define LOCAL_LOG_H

#ifdef IS_ZEPHYR
  #include <zephyr/logging/log.h>

  #define LOCAL_LOG_DBG(fmt, ...) LOG_DBG(fmt, ##__VA_ARGS__)
  #define LOCAL_LOG_HEXDUMP_DBG(data, length, msg) LOG_HEXDUMP_DBG(data, length, msg)

#else  // Not Zephyr

  #ifdef DEBUG
    #include <stdio.h>

    #define LOCAL_LOG_DBG(fmt, ...) \
        printf("DEBUG: " fmt "\n", ##__VA_ARGS__)

    static inline void LOCAL_LOG_HEXDUMP_DBG(const void *data, size_t len, const char *msg) {
        const unsigned char *bytes = (const unsigned char *)data;
        printf("DEBUG HEXDUMP: %s (%zu bytes)\n", msg, len);
        for (size_t i = 0; i < len; i++) {
            printf("%02X ", bytes[i]);
            if ((i + 1) % 16 == 0)
                printf("\n");
        }
        if (len % 16 != 0)
            printf("\n");
    }

  #else  // Not DEBUG

    #define LOCAL_LOG_DBG(fmt, ...) do {} while (0)
    #define LOCAL_LOG_HEXDUMP_DBG(data, length, msg) do {} while (0)

  #endif // DEBUG

#endif // IS_ZEPHYR

#endif // LOCAL_LOG_H
