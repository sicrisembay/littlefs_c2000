/*!
 * \file lfs_util_c2000.h
 */
#ifndef LFS_UTIL_C2000_H
#define LFS_UTIL_C2000_H

// System includes
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>

#ifndef LFS_NO_MALLOC
#include <stdlib.h>
#endif
#ifndef LFS_NO_ASSERT
#include <assert.h>
#endif
#if !defined(LFS_NO_DEBUG) || \
        !defined(LFS_NO_WARN) || \
        !defined(LFS_NO_ERROR) || \
        defined(LFS_YES_TRACE)
#include <stdio.h>
#endif

#include "driver_def.h"
#include <xdc/std.h>
#include <xdc/runtime/System.h>

#define LFS_TI_C2000    (1)

#ifndef int8_t
#define int8_t int_least8_t
#endif

#ifndef uint8_t
#define uint8_t uint_least8_t
#endif

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

// Macros, may be replaced by system specific wrappers. Arguments to these
// macros must not have side-effects as the macros can be removed for a smaller
// code footprint

// Logging functions
#ifndef LFS_TRACE
#ifdef LFS_YES_TRACE
#define LFS_TRACE_(fmt, ...) \
    System_printf("  %s:%d:trace: " fmt "%s\r\n", __FILENAME__, __LINE__, __VA_ARGS__)
#define LFS_TRACE(...) LFS_TRACE_(__VA_ARGS__, "")
#else
#define LFS_TRACE(...)
#endif
#endif

#ifndef LFS_DEBUG
#ifndef LFS_NO_DEBUG
#define LFS_DEBUG_(fmt, ...) \
    System_printf("  %s:%d:debug: " fmt "%s\r\n", __FILENAME__, __LINE__, __VA_ARGS__)
#define LFS_DEBUG(...) LFS_DEBUG_(__VA_ARGS__, "")
#else
#define LFS_DEBUG(...)
#endif
#endif

#ifndef LFS_WARN
#ifndef LFS_NO_WARN
#define LFS_WARN_(fmt, ...) \
    System_printf("  %s:%d:warn: " fmt "%s\r\n", __FILENAME__, __LINE__, __VA_ARGS__)
#define LFS_WARN(...) LFS_WARN_(__VA_ARGS__, "")
#else
#define LFS_WARN(...)
#endif
#endif

#ifndef LFS_ERROR
#ifndef LFS_NO_ERROR
#define LFS_ERROR_(fmt, ...) \
    System_printf("  %s:%d:error: " fmt "%s\r\n", __FILENAME__, __LINE__, __VA_ARGS__)
#define LFS_ERROR(...) LFS_ERROR_(__VA_ARGS__, "")
#else
#define LFS_ERROR(...)
#endif
#endif

// Runtime assertions
#ifndef LFS_ASSERT
#ifndef LFS_NO_ASSERT
#define LFS_ASSERT(test) assert(test)
#else
#define LFS_ASSERT(test)
#endif
#endif


// Builtin functions, these may be replaced by more efficient
// toolchain-specific implementations. LFS_NO_INTRINSICS falls back to a more
// expensive basic C implementation for debugging purposes

// Min/max functions for unsigned 32-bit numbers
static inline uint32_t lfs_max(uint32_t a, uint32_t b) {
    return (a > b) ? a : b;
}

static inline uint32_t lfs_min(uint32_t a, uint32_t b) {
    return (a < b) ? a : b;
}

// Align to nearest multiple of a size
static inline uint32_t lfs_aligndown(uint32_t a, uint32_t alignment) {
    return a - (a % alignment);
}

static inline uint32_t lfs_alignup(uint32_t a, uint32_t alignment) {
    return lfs_aligndown(a + alignment-1, alignment);
}

// Find the smallest power of 2 greater than or equal to a
static inline uint32_t lfs_npw2(uint32_t a) {
#if !defined(LFS_NO_INTRINSICS) && (defined(__GNUC__) || defined(__CC_ARM))
    return 32 - __builtin_clz(a-1);
#else
    uint32_t r = 0;
    uint32_t s;
    a -= 1;
    s = (a > 0xffff) << 4; a >>= s; r |= s;
    s = (a > 0xff  ) << 3; a >>= s; r |= s;
    s = (a > 0xf   ) << 2; a >>= s; r |= s;
    s = (a > 0x3   ) << 1; a >>= s; r |= s;
    return (r | (a >> 1)) + 1;
#endif
}

// Count the number of trailing binary zeros in a
// lfs_ctz(0) may be undefined
static inline uint32_t lfs_ctz(uint32_t a) {
#if !defined(LFS_NO_INTRINSICS) && defined(__GNUC__)
    return __builtin_ctz(a);
#else
    return lfs_npw2((a & -a) + 1) - 1;
#endif
}

// Count the number of binary ones in a
static inline uint32_t lfs_popc(uint32_t a) {
#if !defined(LFS_NO_INTRINSICS) && (defined(__GNUC__) || defined(__CC_ARM))
    return __builtin_popcount(a);
#else
    a = a - ((a >> 1) & 0x55555555);
    a = (a & 0x33333333) + ((a >> 2) & 0x33333333);
    return (((a + (a >> 4)) & 0xf0f0f0f) * 0x1010101) >> 24;
#endif
}

// Find the sequence comparison of a and b, this is the distance
// between a and b ignoring overflow
static inline int lfs_scmp(uint32_t a, uint32_t b) {
    return (int)(unsigned)(a - b);
}

// Convert between 32-bit little-endian and native order
static inline uint32_t lfs_fromle32(uint32_t a) {
    return a;
}

static inline uint32_t lfs_tole32(uint32_t a) {
    return lfs_fromle32(a);
}

// Convert between 32-bit big-endian and native order
static inline uint32_t lfs_frombe32(uint32_t a) {
    return (((a << 24) & 0xFF000000U) |
            ((a << 8)  & 0x00FF0000U) |
            ((a >> 8)  & 0x0000FF00U) |
            ((a >> 24) & 0x000000FFU));
}

static inline uint32_t lfs_tobe32(uint32_t a) {
    return lfs_frombe32(a);
}

// Calculate CRC-32 with polynomial = 0x04c11db7
uint32_t lfs_crc(uint32_t crc, const void *buffer, size_t size);

// Allocate memory, only used if buffers are not provided to littlefs
// Note, memory must be 64-bit aligned
static inline void *lfs_malloc(size_t size) {
#ifndef LFS_NO_MALLOC
    return malloc(size);
#else
    (void)size;
    return NULL;
#endif
}

// Deallocate memory, only used if buffers are not provided to littlefs
static inline void lfs_free(void *p) {
#ifndef LFS_NO_MALLOC
    free(p);
#else
    (void)p;
#endif
}

static inline uint32_t c2000_buffer_to_uint32(uint8_t * buffer)
{
    return ((uint32_t)(buffer[0] & 0x00FF) |
            (((uint32_t)(buffer[1] & 0x0FF)) << 8) |
            (((uint32_t)(buffer[2] & 0x0FF)) << 16) |
            (((uint32_t)(buffer[3] & 0x0FF)) << 24)
            );
}


static inline void c2000_uint32_to_buffer(uint32_t val, uint8_t * buffer)
{
    buffer[0] = (uint8_t)(val & 0x000000FF);
    buffer[1] = (uint8_t)((val >> 8) & 0x000000FF);
    buffer[2] = (uint8_t)((val >> 16) & 0x000000FF);
    buffer[3] = (uint8_t)((val >> 24) & 0x000000FF);
}


int lfs_c2000_format();
int lfs_c2000_mount();
int lfs_c2000_umount();
int lfs_c2000_mkdir(const char * path);
int lfs_c2000_ls(const char * path, char * outBuffer, size_t bufferLen);

int lfs_c2000_fopen(const char * pathName);
int lfs_c2000_fwrite(const char * strData, size_t len);
int lfs_c2000_fread(char * outBuffer, size_t bufLen);
int lfs_c2000_fclose();

#endif /* LFS_UTIL_C2000_H */
