#ifndef INCLUDE_VRT_VRT_UTIL_H_
#define INCLUDE_VRT_VRT_UTIL_H_

#include "vrt_types.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Force compiler to emit warning when return value is ignored */
#ifdef __GNUC__
#define VRT_WARN_UNUSED __attribute__((warn_unused_result))
#elif __clang__
#define VRT_WARN_UNUSED __attribute__((warn_unused_result))
#elif _MSC_VER
#define VRT_WARN_UNUSED _Check_return_
#else
#define VRT_WARN_UNUSED
#endif

/* Force compiler to emit warning when a deprecated value/function is used */
#ifdef __GNUC__
#define VRT_DEPRECATED __attribute__((deprecated))
#elif __clang__
#define VRT_DEPRECATED __attribute__((deprecated))
#elif _MSC_VER
#define VRT_DEPRECATED __declspec(deprecated)
#else
#define VRT_DEPRECATED
#endif

/* Silence C-style cast warnings when using C++ */
#ifdef __cplusplus
#define VRT_STATIC_CAST(T, X)      static_cast<T>(X)
#define VRT_REINTERPRET_CAST(T, X) reinterpret_cast<T>(X)
#else
#define VRT_STATIC_CAST(T, X)      (T)(X)
#define VRT_REINTERPRET_CAST(T, X) (T)(X)
#endif

/**
 * Check if a header indicates it is a data packet.
 *
 * \param header Packet header.
 *
 * \return True if it is IF or Ext data packet, with or without stream identifier.
 *
 * \warning Undefined behaviour if type is outside bounds.
 */
inline bool vrt_is_data(const struct vrt_header* header) {
    return (VRT_STATIC_CAST(uint32_t, header->packet_type) & 0xCU) == 0;
}

/**
 * Check if a header indicates it is a context packet.
 *
 * \param header Packet header.
 *
 * \return True if it is IF or Ext context packet.
 *
 * \warning Undefined behaviour if type is outside bounds.
 */
inline bool vrt_is_context(const struct vrt_header* header) {
    return (VRT_STATIC_CAST(uint32_t, header->packet_type) & 0xEU) == 0x4U;
}

/**
 * Check if a header indicates it is a command packet (VITA 49.2).
 *
 * \param header Packet header.
 *
 * \return True if it is a command or Ext command packet.
 *
 * \warning Undefined behaviour if type is outside bounds.
 */
inline bool vrt_is_command(const struct vrt_header* header) {
    return (VRT_STATIC_CAST(uint32_t, header->packet_type) & 0xEU) == 0x6U;
}

/**
 * Check if a indicates it has the Stream ID field.
 *
 * \param header Packet header.
 *
 * \return True if it has the Stream ID field.
 *
 * \warning Undefined behaviour if type is outside bounds.
 */
inline bool vrt_has_stream_id(const struct vrt_header* header) {
    return (VRT_STATIC_CAST(uint32_t, header->packet_type) & 0x5U) != 0;
}

/**
 * Check if platform is little endian. May be used to detect if byte swap is required for VRT data.
 *
 * \return True if little endian.
 */
inline bool vrt_is_platform_little_endian() {
    volatile uint32_t i = 0x01234567; /* Ensure written to memory */
    return *VRT_REINTERPRET_CAST(volatile uint8_t*, &i) == 0x67;
}

/**
 * Convert a buffer of 32-bit words in place from CPU byte order to big endian (network order), e.g. a packet after it
 * has been written. Does nothing on big endian platforms.
 *
 * \param buf   Buffer of 32-bit words.
 * \param words Number of words to convert. Nothing is done if this is not positive.
 */
void vrt_to_big_endian(void* buf, int32_t words);

/**
 * Convert a buffer of 32-bit words in place from big endian (network order) to CPU byte order, e.g. a received packet
 * before it is read. Does nothing on big endian platforms.
 *
 * \param buf   Buffer of 32-bit words.
 * \param words Number of words to convert. Nothing is done if this is not positive.
 */
void vrt_from_big_endian(void* buf, int32_t words);

#ifdef __cplusplus
}
#endif

#endif
