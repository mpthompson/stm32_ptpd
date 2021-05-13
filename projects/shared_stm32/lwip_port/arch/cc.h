#ifndef __CC_H__
#define __CC_H__

#if !defined(__ARMCC_VERSION) && defined (__GNUC__)
#include <sys/time.h>
#endif

#if defined(DEBUG)
#undef LWIP_NOASSERT
#else
#define LWIP_NOASSERT
#endif

#ifndef BYTE_ORDER
#define BYTE_ORDER LITTLE_ENDIAN
#endif

// This platform does provide stddef.h and stdint.h.
#define LWIP_NO_STDDEF_H 0
#define LWIP_NO_STDINT_H 0

// Compiler hints for packing lwip's structures
// FSL: very important at high optimization level

#if defined(__GNUC__)
#define PACK_STRUCT_BEGIN
#elif defined(__IAR_SYSTEMS_ICC__)
#define PACK_STRUCT_BEGIN _Pragma("pack(1)")
#elif defined(__CC_ARM)
#define PACK_STRUCT_BEGIN __packed
#elif (__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
#define PACK_STRUCT_BEGIN
#else
#define PACK_STRUCT_BEGIN
#endif

#if defined(__GNUC__)
#define PACK_STRUCT_STRUCT __attribute__((__packed__))
#elif defined(__IAR_SYSTEMS_ICC__)
#define PACK_STRUCT_STRUCT
#elif defined(__CC_ARM)
#define PACK_STRUCT_STRUCT
#elif (__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
#define PACK_STRUCT_STRUCT __attribute__((packed))
#else
#define PACK_STRUCT_STRUCT
#endif

#if defined(__GNUC__)
#define PACK_STRUCT_END
#elif defined(__IAR_SYSTEMS_ICC__)
#define PACK_STRUCT_END _Pragma("pack()")
#elif defined(__CC_ARM)
#define PACK_STRUCT_END
#elif (__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
#define PACK_STRUCT_END
#else
#define PACK_STRUCT_END
#endif

#define PACK_STRUCT_FIELD(x) x

// Platform specific diagnostic output
#include "outputf.h"
extern void sys_assert(char *msg);

// Non-fatal, print a message.
#define LWIP_PLATFORM_DIAG(x)                     do { outputf x; } while(0)

// Fatal, print message and abandon execution.
#define LWIP_PLATFORM_ASSERT(x)                   sys_assert( x )

#endif /* __CC_H__ */
