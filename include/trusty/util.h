/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef TRUSTY_UTIL_H_
#define TRUSTY_UTIL_H_

#include <trusty/sysdeps.h>

#define TRUSTY_STRINGIFY(x) #x
#define TRUSTY_TO_STRING(x) TRUSTY_STRINGIFY(x)

/*
 * Aborts the program if @expr is false.
 *
 * This has no effect unless TIPC_ENABLE_DEBUG is defined.
 */
#ifdef TIPC_ENABLE_DEBUG
#define trusty_assert(expr)                     \
  do {                                          \
    if (!(expr)) {                              \
      trusty_fatal("assert fail: " #expr "\n"); \
    }                                           \
  } while(0)
#else
#define trusty_assert(expr)
#endif

/*
 * Prints debug message.
 *
 * This has no effect unless TIPC_ENABLE_DEBUG and LOCAL_LOG is defined.
 */
#ifdef TIPC_ENABLE_DEBUG
#define trusty_debug(message, ...)                                       \
  do {                                                                   \
    if (LOCAL_LOG) {                                                     \
      trusty_printv(__FILE__ ":" TRUSTY_TO_STRING(__LINE__) ": DEBUG "); \
      trusty_printv(message, ##__VA_ARGS__, NULL);                       \
    }                                                                    \
  } while(0)
#else
#define trusty_debug(message, ...)
#endif

/*
 * Prints info message.
 */
#define trusty_info(message, ...)                \
  do {                                           \
    trusty_printv(__FILE__ ": INFO ");           \
    trusty_printv(message, ##__VA_ARGS__, NULL); \
  } while(0)

/*
 * Prints error message.
 */
#define trusty_error(message, ...)                                     \
  do {                                                                 \
    trusty_printv(__FILE__ ":" TRUSTY_TO_STRING(__LINE__) ": ERROR "); \
    trusty_printv(message, ##__VA_ARGS__, NULL);                       \
  } while(0)

/*
 * Prints message and calls trusty_abort.
 */
#define trusty_fatal(message, ...)                                     \
  do {                                                                 \
    trusty_printv(__FILE__ ":" TRUSTY_TO_STRING(__LINE__) ": FATAL "); \
    trusty_printv(message, ##__VA_ARGS__, NULL);                       \
    trusty_abort();                                                    \
  } while(0)

#endif /* TRUSTY_UTIL_H_ */
