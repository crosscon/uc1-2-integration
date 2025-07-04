#ifndef LOCAL_LOG_H
#define LOCAL_LOG_H

#ifdef DEBUG
  #define LOG_DBG(fmt, ...) printf("DEBUG: " fmt, ##__VA_ARGS__)
#else
  #define LOG_DBG(fmt, ...) do {} while (0)
#endif

#endif // LOCAL_LOG_H
