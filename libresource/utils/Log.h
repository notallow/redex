/**
 * Copyright (c) 2016-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */



#ifndef _FB_LOG_H_REIMPLEMENTATION
#define _FB_LOG_H_REIMPLEMENTATION

#define OS_PATH_SEPARATOR '/'

#include <cstdio>
#include <cstdlib>
// do {...} while(0) is a trick to allow multiple statements to be used in macros without messing up
// if statements and the like.
#define ALOGF(args...) \
    do {fprintf(stderr, "FATAL: "); fprintf(stderr, args); fprintf(stderr, "\n");} while(0)
#define ALOGE(args...) \
    do {fprintf(stderr, "ERROR: "); fprintf(stderr, args); fprintf(stderr, "\n");} while(0)
#define ALOGW(args...) \
    do {fprintf(stderr, "WARNING: "); fprintf(stderr, args); fprintf(stderr, "\n");} while(0)
#define ALOGI(args...) \
    do {fprintf(stderr, "INFO: "); fprintf(stderr, args); fprintf(stderr, "\n");} while(0)
#define ALOGD(args...) \
    do {fprintf(stderr, "DEBUG: "); fprintf(stderr, args); fprintf(stderr, "\n");} while(0)
#define ALOGV(args...) \
    do {fprintf(stderr, "VERBOSE: "); fprintf(stderr, args); fprintf(stderr, "\n");} while(0)

#define LOG_ALWAYS_FATAL(...) \
    do {ALOGF(__VA_ARGS__); exit(-1);} while(0)

#define LOG_FATAL_IF(cond, ...) \
    do { if (cond) {ALOGF(#cond); ALOGF(__VA_ARGS__); exit(-1);}} while(0)

#define LOG_ALWAYS_FATAL_IF(...) \
    LOG_FATAL_IF(__VA_ARGS__)

#define ALOGW_IF(cond, ...) \
    do { if (cond) {ALOGW(#cond); ALOGW(__VA_ARGS__);}} while(0)

#define ALOG_ASSERT(cond, ...) \
    do { if (!(cond)) {ALOGF("Assertion failed"); ALOGF(#cond); ALOGF(__VA_ARGS__); exit(-1);}} while(0)


#endif // _FB_LOG_H_REIMPLEMENTATION
