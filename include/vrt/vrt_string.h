#ifndef INCLUDE_VRT_VRT_STRING_H_
#define INCLUDE_VRT_VRT_STRING_H_

#include "vrt_types.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Get a string representation of Packet type enum.
 *
 * \param packet_type Packet type.
 *
 * \return String representation.
 *
 * \note Inputting an unknown packet type returns "Unknown".
 */
const char* vrt_string_packet_type(enum vrt_packet_type packet_type);

/**
 * Get a string representation of TSM (Timestamp mode) enum.
 *
 * \param tsm TSM.
 *
 * \return String representation.
 *
 * \note Inputting an unknown TSM returns "Unknown".
 */
const char* vrt_string_tsm(enum vrt_tsm tsm);

/**
 * Get a string representation of TSI (Timestamp integer) enum.
 *
 * \param tsi TSI.
 *
 * \return String representation.
 *
 * \note Inputting an unknown TSI returns "Unknown".
 */
const char* vrt_string_tsi(enum vrt_tsi tsi);

/**
 * Get a string representation of TSF (Timestamp fractional) enum.
 *
 * \param tsf TSF.
 *
 * \return String representation.
 *
 * \note Inputting an unknown TSF returns "Unknown".
 */
const char* vrt_string_tsf(enum vrt_tsf tsf);

/**
 * Get a string representation of AGC/MGC enum.
 *
 * \param agc_or_mgc AGC/MGC.
 *
 * \return String representation.
 *
 * \note Inputting an unknown AGC/MGC returns "Unknown".
 */
const char* vrt_string_agc_or_mgc(enum vrt_agc_or_mgc agc_or_mgc);

/**
 * Get a string representation of Packing method enum.
 *
 * \param packing_method Packing method.
 *
 * \return String representation.
 *
 * \note Inputting an unknown Packing method returns "Unknown".
 */
const char* vrt_string_packing_method(enum vrt_packing_method packing_method);

/**
 * Get a string representation of Real/Complex enum.
 *
 * \param real_or_complex Real/Complex.
 *
 * \return String representation.
 *
 * \note Inputting an unknown Real/Complex returns "Unknown".
 */
const char* vrt_string_real_or_complex(enum vrt_real_complex real_or_complex);

/**
 * Get a string representation of Data item format enum.
 *
 * \param data_item_format Data item format.
 *
 * \return String representation.
 *
 * \note Inputting an unknown Data item format returns "Unknown".
 */
const char* vrt_string_data_item_format(enum vrt_data_item_format data_item_format);

/**
 * Get a string representation of Spectrum or time enum.
 *
 * \param spectrum_or_time Spectrum or time.
 *
 * \return String representation.
 *
 * \note Inputting an unknown Spectrum or time returns "Unknown".
 */
const char* vrt_string_spectrum_or_time(enum vrt_spectrum_or_time spectrum_or_time);

/**
 * Get a string representation of V49 spec compliance enum.
 *
 * \param v49_spec_compliance V49 spec compliance.
 *
 * \return String representation.
 *
 * \note Inputting an unknown V49 spec compliance returns "Unknown".
 */
const char* vrt_string_v49_spec_compliance(enum vrt_v49_spec_compliance v49_spec_compliance);

/**
 * Get a string representation of Identifier format enum.
 *
 * \param identifier_format Identifier format.
 *
 * \return String representation.
 *
 * \note Inputting an unknown Identifier format returns "Unknown".
 */
const char* vrt_string_identifier_format(enum vrt_identifier_format identifier_format);

/**
 * Get a string representation of Action mode enum.
 *
 * \param action_mode Action mode.
 *
 * \return String representation.
 *
 * \note Inputting an unknown Action mode returns "Unknown".
 */
const char* vrt_string_action_mode(enum vrt_action_mode action_mode);

/**
 * Get a string representation of Timing control enum.
 *
 * \param timing_control Timing control.
 *
 * \return String representation.
 *
 * \note Inputting an unknown Timing control returns "Unknown".
 */
const char* vrt_string_timing_control(enum vrt_timing_control timing_control);

/**
 * Get a string representation of an error code.
 *
 * \param error Error code.
 *
 * \return String representation.
 *
 * \note Inputting a code that isn't an error, i.e. >= 0, returns "Success". Inputting an unknown error code returns
 *       "Unknown".
 */
const char* vrt_string_error(int32_t error);

#ifdef __cplusplus
}
#endif

#endif
