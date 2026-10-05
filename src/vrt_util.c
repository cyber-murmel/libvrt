#include "vrt/vrt_util.h"

struct vrt_header;

/* Defined inline in header */
extern bool vrt_is_data(const struct vrt_header* header);
extern bool vrt_is_context(const struct vrt_header* header);
extern bool vrt_is_command(const struct vrt_header* header);
extern bool vrt_has_stream_id(const struct vrt_header* header);
extern bool vrt_is_platform_little_endian();

/**
 * Reverse byte order of a 32-bit word.
 *
 * \param v Word.
 *
 * \return Byte swapped word.
 */
static inline uint32_t vrt_bswap32(uint32_t v) {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_bswap32(v);
#else
    return (v >> 24U) | ((v >> 8U) & 0x0000FF00U) | ((v << 8U) & 0x00FF0000U) | (v << 24U);
#endif
}

/**
 * Check if byte swap is needed to convert between CPU byte order and big endian.
 *
 * \return True on little endian platforms.
 */
static inline bool vrt_needs_swap(void) {
#if defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return false;
#elif defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return true;
#else
    return vrt_is_platform_little_endian();
#endif
}

void vrt_to_big_endian(void* buf, int32_t words) {
    if (!vrt_needs_swap()) {
        return;
    }
    uint32_t* b = (uint32_t*)buf;
    for (int32_t i = 0; i < words; ++i) {
        b[i] = vrt_bswap32(b[i]);
    }
}

void vrt_from_big_endian(void* buf, int32_t words) {
    /* Byte swap is its own inverse */
    vrt_to_big_endian(buf, words);
}
