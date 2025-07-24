#ifndef LOCAL_LOG_H
#define LOCAL_LOG_H

#ifdef IS_ZEPHYR
  #include <zephyr/logging/log.h>

  #define LOCAL_LOG_DBG(fmt, ...) LOG_DBG(fmt, ##__VA_ARGS__)
#else
  #ifdef DEBUG
    #include <stdio.h>
    #define LOCAL_LOG_DBG(fmt, ...) printf("DEBUG: " fmt "\n", ##__VA_ARGS__)
  #else
    #define LOCAL_LOG_DBG(fmt, ...) do {} while (0)
  #endif
#endif

#endif // LOCAL_LOG_H
