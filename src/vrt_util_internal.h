#ifndef SRC_VRT_UTIL_H_
#define SRC_VRT_UTIL_H_

#include "vrt/vrt_types.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Value when TSI is unspecified in Formatted GPS/INS geolocation and ECEF/Relative ephemeris.
 */
static const uint32_t VRT_UNSPECIFIED_TSI = 0xFFFFFFFFU;
/**
 * Value when TSF is unspecified in Formatted GPS/INS geolocation and ECEF/Relative ephemeris.
 */
static const uint64_t VRT_UNSPECIFIED_TSF = 0xFFFFFFFFFFFFFFFFU;
/**
 * Value when fixed point number fields are unspecified in Formatted GPS/INS geolocation and ECEF/Relative ephemeris.
 */
static const uint32_t VRT_UNSPECIFIED_FIXED_POINT = 0x7FFFFFFFU;

/** CIF0 bit enabling the CIF1 word (VITA 49.2). */
static const uint32_t VRT_CIF0_CIF1_ENABLE = 0x00000002U;
/** CIF0 bits enabling the CIF2, CIF3 and CIF7 words, which are not supported. */
static const uint32_t VRT_CIF0_UNSUPPORTED = 0x0000008CU;
/** Reserved CIF0 bits. */
static const uint32_t VRT_CIF0_RESERVED = 0x00000071U;
/** CIF1 bit of the Health status field. */
static const uint32_t VRT_CIF1_HEALTH_STATUS = 0x00000010U;
/** CIF1 bit of the V49 spec compliance field. */
static const uint32_t VRT_CIF1_V49_SPEC_COMPLIANCE = 0x00000008U;
/** CIF1 bit of the Version and build code field. */
static const uint32_t VRT_CIF1_VERSION_BUILD_CODE = 0x00000004U;
/** CIF1 bits of fields that are not supported. Their sizes are unknown here, so they cannot be skipped. */
static const uint32_t VRT_CIF1_UNSUPPORTED = 0xFF1FEEE2U;
/** Reserved CIF1 bits. */
static const uint32_t VRT_CIF1_RESERVED = 0x00E01101U;
/** Reserved CAM bits. */
static const uint32_t VRT_CAM_RESERVED = 0x002083FFU;

/**
 * Convert bool to uint32.
 *
 * \param b Bool to convert.
 *
 * \return 1 if true and 0 otherwise.
 */
inline uint32_t vrt_b2u(bool b) {
    return b ? 1 : 0;
}

/**
 * Convert uint32 to bool.
 *
 * \param u uint32 to convert.
 *
 * \return false if 0 and true otherwise.
 */
inline uint32_t vrt_u2b(uint32_t u) {
    return u != 0;
}

/**
 * Check if a packet has the fractional timestamp.
 *
 * \param t TSF field.
 *
 * \return true if it has the fractional timestamp.
 */
inline bool vrt_has_fractional_timestamp(enum vrt_tsf t) {
    return t != VRT_TSF_NONE;
}

/**
 * Check if indicators require the CIF1 word (VITA 49.2).
 *
 * \param ind Context indicator field.
 *
 * \return true if at least one CIF1 field is present.
 */
inline bool vrt_has_cif1(const struct vrt_context_indicators* ind) {
    return ind->health_status || ind->v49_spec_compliance || ind->version_build_code;
}

/**
 * Calculate size in 32-bit words of the indicator words, i.e. CIF0 and the optional CIF1 (VITA 49.2).
 *
 * \param ind Context indicator field.
 *
 * \return Number of 32-bit words.
 */
inline int32_t vrt_words_indicator_words(const struct vrt_context_indicators* ind) {
    return vrt_has_cif1(ind) ? 2 : 1;
}

/**
 * Calculate size in 32-bit words of a controllee or controller identifier (VITA 49.2).
 *
 * \param enable True if the identifier is included.
 * \param format Identifier format.
 *
 * \return Number of 32-bit words.
 */
inline int32_t vrt_words_identifier(bool enable, enum vrt_identifier_format format) {
    if (!enable) {
        return 0;
    }
    return format == VRT_IDF_UUID ? 4 : 1;
}

/**
 * Check if a command packet has warning and error indicator and response words (VITA 49.2), which is the case for
 * validation and execution acknowledge packets.
 *
 * \param header Header.
 * \param cam    Control/Acknowledge mode.
 *
 * \return true if the CAM warnings and errors bits enable the warning and error sections.
 *
 * \note If an acknowledge packet sets more than one of validation, execution and query-state, which is invalid,
 *       validation and execution take precedence.
 */
inline bool vrt_has_ack_responses(const struct vrt_header* header, const struct vrt_cam* cam) {
    return header->acknowledge && (cam->validation || cam->execution);
}

/**
 * What follows the command section of a Command packet (VITA 49.2).
 */
enum vrt_command_payload {
    VRT_CP_NONE,      /**< Nothing. Validation and execution acknowledge packets. */
    VRT_CP_CONTEXT,   /**< Indicator words and fields, like IF context. Control and query-state acknowledge packets. */
    VRT_CP_INDICATORS /**< Indicator words without fields. Cancellation packets. */
};

/**
 * Determine what follows the command section of a Command packet (VITA 49.2).
 *
 * \param header Header.
 * \param cam    Control/Acknowledge mode.
 *
 * \return Kind of payload.
 *
 * \note Only valid for packet type VRT_PT_COMMAND. Ext command packets have a custom body instead.
 */
inline enum vrt_command_payload vrt_command_payload(const struct vrt_header* header, const struct vrt_cam* cam) {
    if (header->acknowledge) {
        return !vrt_has_ack_responses(header, cam) && cam->query_state ? VRT_CP_CONTEXT : VRT_CP_NONE;
    }
    return header->cancellation ? VRT_CP_INDICATORS : VRT_CP_CONTEXT;
}

/**
 * Calculate partial size in 32-bit words of IF context section, from the information available in the context indicator
 * section.
 *
 * \note This does not take the variable sizes from the GPS ASCII as well as Context association lists fields into
 *       account. It includes the CIF1 word and fields.
 *
 * \param ind Context indicator field.
 *
 * \return Number of 32-bit words it consists of.
 */
int32_t vrt_words_if_context_indicator(const struct vrt_context_indicators* ind);

/**
 * Calculate number of response words of warning or error responses in acknowledge packets (VITA 49.2), i.e. one word
 * per indicated field. The indicator words are not included.
 *
 * \param ind Indicators of fields with a response word.
 *
 * \return Number of 32-bit words.
 */
int32_t vrt_words_ack_response_words(const struct vrt_context_indicators* ind);

/**
 * Initialize all context indicators to false.
 *
 * \param ind Context indicators.
 */
void vrt_init_context_indicators(struct vrt_context_indicators* ind);

#ifdef __cplusplus
}
#endif

#endif
