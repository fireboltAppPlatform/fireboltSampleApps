#ifndef _LOGGER_H_
#define _LOGGER_H_

#include <cstdio>
#include <cassert>
#include <cstring>

/**
 * Enable rdk_logger. If not defined, fallback to stdout logging.
 */
// #define USE_RDK_LOGGER

#ifdef USE_RDK_LOGGER
#undef USE_RDK_LOGGER
#endif

/**
 * @brief Logging level with an increasing order of refinement
 * (TRACE_LEVEL = Finest logging).
 * It is essental to start with 0 and increase w/o gaps as the value
 * can be used for indexing in a mapping table.
 */
enum LogLevel {
  FATAL_LEVEL = 0,
  ERROR_LEVEL,
  WARNING_LEVEL,
  METRIC_LEVEL,
  INFO_LEVEL,
  VERBOSE_LEVEL,
  TRACE_LEVEL,
};

/**
 * @brief Init logging
 * Should be called once per program run before calling log-functions
 */
void log_init();

/**
 * @brief Log a message
 * The function is defined by logging backend.
 * Currently 2 variants are supported: rdk_logger (USE_RDK_LOGGER),
 *                                     stdout(default)
 */
void log (LogLevel level,
          const char* func,
          const char* file,
          int line,
          const char* format, ...);

#define LOG(LEVEL, FORMAT, ...)                                     \
            log( LEVEL,                                             \
                 __func__, __FILE__, __LINE__,                      \
                 FORMAT,                                            \
                 ##__VA_ARGS__)

#define LOG_INIT                log_init
#define LOG_TRACE(FMT, ...)     LOG(TRACE_LEVEL, FMT, ##__VA_ARGS__)
#define LOG_VERBOSE(FMT, ...)   LOG(VERBOSE_LEVEL, FMT, ##__VA_ARGS__)
#define LOG_METRIC(FMT, ...)    LOG(METRIC_LEVEL, FMT, ##__VA_ARGS__)
#define LOG_INFO(FMT, ...)      LOG(INFO_LEVEL, FMT, ##__VA_ARGS__)
#define LOG_WARNING(FMT, ...)   LOG(WARNING_LEVEL, FMT, ##__VA_ARGS__)
#define LOG_ERROR(FMT, ...)     LOG(ERROR_LEVEL, FMT, ##__VA_ARGS__)
#define LOG_FATAL(FMT, ...)     LOG(FATAL_LEVEL, FMT, ##__VA_ARGS__)

#ifndef NDEBUG
#define LOG_ASSERT(FMT, ...)    LOG(FATAL_LEVEL, FMT, ##__VA_ARGS__)
#else
#define LOG_ASSERT(FMT, ...)    LOG(ERROR_LEVEL, FMT, ##__VA_ARGS__)
#endif

#define ASSERT(x)                               \
  ((x)                                          \
   ? (void)(0)                                  \
   : LOG_ASSERT("'%s' check failed.", #x) )     \


#endif  // _LOGGER_H_
