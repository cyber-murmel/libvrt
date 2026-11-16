#ifndef INCLUDE_VRT_VRT_WRITE_H_
#define INCLUDE_VRT_VRT_WRITE_H_

#include "vrt_util.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct vrt_command;
struct vrt_fields;
struct vrt_header;
struct vrt_if_context;
struct vrt_packet;
struct vrt_trailer;

/**
 * Low-level function that writes VRT header section.
 *
 * \param header    Header to write.
 * \param buf       Buffer to write to.
 * \param words_buf Size of buf in 32-bit words.
 * \param validate  True if validation shall be done. If false, only buffer size is validated.
 *
 * \return Number of written 32-bit words (always 1), or a negative number if error.
 * \retval VRT_ERR_BUFFER_SIZE          Buffer is too small.
 * \retval VRT_ERR_INVALID_PACKET_TYPE  Packet type is an invalid value.
 * \retval VRT_ERR_INVALID_TSM          TSM is an invalid value.
 * \retval VRT_ERR_TSM_IN_DATA          Data packet has TSM bit set.
 * \retval VRT_ERR_INVALID_TSI          TSI is an invalid value.
 * \retval VRT_ERR_INVALID_TSF          TSF is an invalid value.
 * \retval VRT_ERR_BOUNDS_PACKET_COUNT  Packet count is outside valid bounds (> 0x0F).
 * \retval VRT_ERR_INDICATOR_IN_COMMAND             Command packet has the trailer, Nd0 or TSM indicator set.
 * \retval VRT_ERR_INVALID_SPECTRUM_OR_TIME         Spectrum or time is an invalid value.
 * \retval VRT_ERR_SPECTRUM_IN_NON_DATA             Spectrum indicator is set in a non data packet.
 * \retval VRT_ERR_COMMAND_INDICATOR_IN_NON_COMMAND Acknowledge or cancellation indicator is set in a non command
 *                                                  packet.
 *
 * \note May require output buffer data to be byte swapped if platform endianess isn't big endian (network order). See
 *       vrt_to_big_endian().
 * \note Header bits 26 to 24 are packet type specific (VITA 49.2). Without validation, has.trailer and acknowledge are
 *       ORed into bit 26, and tsm, spectrum_or_time and cancellation into bit 24.
 */
VRT_WARN_UNUSED
int32_t vrt_write_header(const struct vrt_header* header, void* buf, int32_t words_buf, bool validate);

/**
 * Low-level function that writes VRT fields section.
 *
 * \param header    Header.
 * \param fields    Fields to write.
 * \param buf       Buffer to write to. This must point to the start of the fields section, i.e. directly following the
 *                  header word.
 * \param words_buf Size of buf in 32-bit words.
 * \param validate  True if validation shall be done. If false, only buffer size is validated.
 *
 * \return Number of written 32-bit words, or a negative number if error.
 * \retval VRT_ERR_BUFFER_SIZE      Buffer is too small.
 * \retval VRT_ERR_BOUNDS_REAL_TIME TSF is VRT_TSF_REAL_TIME but fractional timestamp is outside valid bounds
 *                                  (> 999999999999 ps).
 * \retval VRT_ERR_BOUNDS_OUI       OUI is outside valid bounds (> 0x00FFFFFF).
 */
VRT_WARN_UNUSED
int32_t vrt_write_fields(const struct vrt_header* header,
                         const struct vrt_fields* fields,
                         void*                    buf,
                         int32_t                  words_buf,
                         bool                     validate);

/**
 * Low-level function that writes VRT trailer section.
 *
 * \param trailer    Trailer to write.
 * \param buf        Buffer to write to. This must point to the position of the trailer word, i.e. the last word in the
 *                   packet.
 * \param words_buf  Size of buf in 32-bit words.
 * \param validate   True if validation shall be done. If false, only buffer size is validated.
 *
 * \return Number of written 32-bit words, or a negative number if error.
 * \retval VRT_ERR_BUFFER_SIZE                              Buffer is too small.
 * \retval VRT_ERR_BOUNDS_ASSOCIATED_CONTEXT_PACKET_COUNT   Associated context packet count is outside valid bounds
 *                                                          (>0x7F).
 */
VRT_WARN_UNUSED
int32_t vrt_write_trailer(const struct vrt_trailer* trailer, void* buf, int32_t words_buf, bool validate);

/**
 * Low-level function that writes VRT IF context section.
 *
 * \param if_context IF context to write.
 * \param buf        Buffer to write to. This must point to the first context section word in the packet, i.e. directly
 *                   following the fields section.
 * \param words_buf  Size of buf in 32-bit words.
 * \param validate   True if validation shall be done. If false, only buffer size is validated.
 *
 * \return Number of written 32-bit words, or a negative number if error.
 * \retval VRT_ERR_BUFFER_SIZE                          Buffer is too small.
 * \retval VRT_ERR_BOUNDS_BANDWIDTH                     Bandwidth is outside valid bounds (< 0 Hz or > 8.79 THz).
 * \retval VRT_ERR_BOUNDS_IF_REFERENCE_FREQUENCY        IF reference frequency is outside valid bounds
 *                                                      (< ~-8.79 or > ~8.79 THz).
 * \retval VRT_ERR_BOUNDS_RF_REFERENCE_FREQUENCY        RF reference frequency is outside valid bounds
 *                                                      (< ~-8.79 or > ~8.79 THz).
 * \retval VRT_ERR_BOUNDS_RF_REFERENCE_FREQUENCY_OFFSET RF reference frequency offset is outside valid bounds
 *                                                      (< ~-8.79 or > ~8.79 THz).
 * \retval VRT_ERR_BOUNDS_IF_BAND_OFFSET                IF band offset is outside valid bounds
 *                                                      (< ~-8.79 or > ~8.79 THz).
 * \retval VRT_ERR_BOUNDS_REFERENCE_LEVEL               Reference level is outside valid bounds (< -256 or > ~256 dBm).
 * \retval VRT_ERR_BOUNDS_GAIN                          Gain stage 1 or 2 is outside valid bounds (< -256 or > ~256 dB).
 * \retval VRT_ERR_GAIN_STAGE2_SET                      Gain stage 1 must be used instead of stage 2 when only one of
 *                                                      them is set.
 * \retval VRT_ERR_BOUNDS_SAMPLE_RATE                   Sample rate is outside valid bounds (< 0 Hz or >
 * ~8.79 THz). \retval VRT_ERR_BOUNDS_TEMPERATURE       Temperature is outside valid bounds
 *                                                      (< -273.15 or > ~512 degrees C).
 * \retval VRT_ERR_BOUNDS_OUI                           OUI is outside valid bounds (> 0x00FFFFFF).
 * \retval VRT_ERR_INVALID_PACKING_METHOD               Packing method is an invalid value.
 * \retval VRT_ERR_INVALID_REAL_OR_COMPLEX              Real/Complex is an invalid value.
 * \retval VRT_ERR_INVALID_DATA_ITEM_FORMAT             Data item format is an invalid value.
 * \retval VRT_ERR_BOUNDS_EVENT_TAG_SIZE                Event tag size is outside valid bounds (> 0x07).
 * \retval VRT_ERR_BOUNDS_CHANNEL_TAG_SIZE              Channel tag size is outside valid bounds (> 0x0F).
 * \retval VRT_ERR_BOUNDS_ITEM_PACKING_FIELD_SIZE       Item packing fields size is outside valid bounds (> 0x3F).
 * \retval VRT_ERR_BOUNDS_DATA_ITEM_SIZE                Data item size is outside valid bounds (> 0x3F).
 * \retval VRT_ERR_INVALID_TSI                          TSI is an invalid value.
 * \retval VRT_ERR_INVALID_TSF                          TSF is an invalid value.
 * \retval VRT_ERR_SET_INTEGER_SECOND_TIMESTAMP         Integer second timestamp is not 0xFFFFFFFF when TSI is
 *                                                      VRT_TSI_UNDEFINED.
 * \retval VRT_ERR_SET_FRACTIONAL_SECOND_TIMESTAMP      Integer second timestamp is not 0xFFFFFFFF when TSI is
 *                                                      VRT_TSI_UNDEFINED.
 * \retval VRT_ERR_BOUNDS_REAL_TIME                     TSF is VRT_TSF_REAL_TIME but fractional timestamp is outside
 *                                                      valid bounds (> 999999999999 ps).
 * \retval VRT_ERR_BOUNDS_LATITUDE                      Latitude is outside valid bounds (< -90 or > 90 degrees).
 * \retval VRT_ERR_BOUNDS_LONGITUDE                     Longitude is outside valid bounds (< -180 or > 180 degrees).
 * \retval VRT_ERR_BOUNDS_ALTITUDE                      Altitude is outside valid bounds (< ~-67108 or > ~67108 km).
 * \retval VRT_ERR_BOUNDS_SPEED_OVER_GROUND             Speed over ground is outside valid bounds (< 0 or > ~65536 m/s).
 * \retval VRT_ERR_BOUNDS_HEADING_ANGLE                 Heading angle is outside valid bounds (< 0 or > ~360
 *                                                      degrees).
 * \retval VRT_ERR_BOUNDS_TRACK_ANGLE                   Track angle is outside valid bounds (< 0 or > ~360
 *                                                      degrees).
 * \retval VRT_ERR_BOUNDS_MAGNETIC_VARIATION            Magnetic variation is outside valid bounds (< -180 or > 180
 *                                                      degrees).
 * \retval VRT_ERR_BOUNDS_POSITION                      Position x, y, or z is outside valid bounds
 *                                                      (< ~-67108 or > ~67108 km).
 * \retval VRT_ERR_BOUNDS_ATTITUDE                      Attitude alpha, beta, or gamma is outside valid bounds
 *                                                      (< -512 or > ~512 degrees).
 * \retval VRT_ERR_BOUNDS_VELOCITY                      Velocity dx, dy, or dz is outside valid bounds
 *                                                      (< -32768 or > ~32768 m/s).
 * \retval VRT_ERR_BOUNDS_SOURCE_LIST_SIZE              Source list size is outside valid bounds (> 0x01FF).
 * \retval VRT_ERR_BOUNDS_SYSTEM_LIST_SIZE              System list size is outside valid bounds (> 0x01FF).
 * \retval VRT_ERR_BOUNDS_CHANNEL_LIST_SIZE             Channel list size is outside valid bounds (> 0x7FFF).
 * \retval VRT_ERR_INVALID_V49_SPEC_COMPLIANCE          V49 spec compliance is an invalid value.
 * \retval VRT_ERR_BOUNDS_VERSION_BUILD_CODE            Version and build code subfield is outside valid bounds.
 *
 * \note The CIF1 word (VITA 49.2) and its CIF0 enable bit are written exactly when at least one of has.health_status,
 *       has.v49_spec_compliance and has.version_build_code is set.
 *
 * \warning Fields represented as double but with an underlying 64-bit fixed precision format, i.e. bandwidth,
 *          if_reference_frequency, rf_reference_frequency, rf_reference_frequency_offset, if_band_offset, and
 *          sample_rate, may in rare cases lose precision since double only has 53 bits of precision.
 */
VRT_WARN_UNUSED
int32_t vrt_write_if_context(const struct vrt_if_context* if_context, void* buf, int32_t words_buf, bool validate);

/**
 * Low-level function that writes the command section of command and Ext command packets (VITA 49.2), i.e. CAM, message
 * ID, controllee and controller identifiers, and for validation and execution acknowledge packets the warning and
 * error indicator and response words.
 *
 * \param header    Header, which tells if it is an acknowledge packet.
 * \param command   Command section to write.
 * \param buf       Buffer to write to. This must point to the CAM word, i.e. directly following the fields section.
 * \param words_buf Size of buf in 32-bit words.
 * \param validate  True if validation shall be done. If false, only buffer size is validated.
 *
 * \return Number of written 32-bit words, or a negative number if error.
 * \retval VRT_ERR_BUFFER_SIZE               Buffer is too small.
 * \retval VRT_ERR_INVALID_IDENTIFIER_FORMAT Controllee or controller identifier format is an invalid value.
 * \retval VRT_ERR_INVALID_ACTION_MODE       Action mode is an invalid value.
 * \retval VRT_ERR_INVALID_TIMING_CONTROL    Timing control is an invalid value.
 * \retval VRT_ERR_ACK_SUBTYPE               Acknowledge packet does not set exactly one of validation, execution and
 *                                           query-state.
 * \retval VRT_ERR_ACK_FIELD_IN_CONTROL      Acknowledge-only CAM field is set in a non acknowledge packet.
 *
 * \note Warning and error words are written when the header is an acknowledge packet, cam.validation or
 *       cam.execution is set, and cam.warnings or cam.errors respectively is set.
 */
VRT_WARN_UNUSED
int32_t vrt_write_command(const struct vrt_header*  header,
                          const struct vrt_command* command,
                          void*                     buf,
                          int32_t                   words_buf,
                          bool                      validate);

/**
 * Higher-level function that writes a full VRT packet.
 *
 * \param packet     Packet to write.
 * \param buf        Buffer to write to.
 * \param words_buf  Size of buf in 32-bit words.
 * \param validate   True if validation shall be done. If false, only buffer size is validated.
 *
 * \return Number of written 32-bit words, or a negative number if error.
 * \retval VRT_ERR_BUFFER_SIZE                            Buffer is too small.
 * \retval VRT_ERR_INVALID_PACKET_TYPE                    Packet type is an invalid value.
 * \retval VRT_ERR_INVALID_TSM                            TSM is an invalid value.
 * \retval VRT_ERR_TSM_IN_DATA                            Data packet has TSM bit set.
 * \retval VRT_ERR_INVALID_TSI                            TSI is an invalid value.
 * \retval VRT_ERR_INVALID_TSF                            TSF is an invalid value.
 * \retval VRT_ERR_BOUNDS_PACKET_COUNT                    Packet count is outside valid bounds (> 0x0F).
 * \retval VRT_ERR_BOUNDS_REAL_TIME                       TSF is VRT_TSF_REAL_TIME but fractional timestamp is outside
 *                                                        valid bounds (> 999999999999 ps).
 * \retval VRT_ERR_BOUNDS_OUI                             OUI is outside valid bounds (> 0x00FFFFFF).
 * \retval VRT_ERR_BOUNDS_ASSOCIATED_CONTEXT_PACKET_COUNT Associated context packet count is outside
 *                                                        valid bounds
 *                                                        (>0x7F).
 * \retval VRT_ERR_BOUNDS_BANDWIDTH                       Bandwidth is outside valid bounds (< 0 Hz or > 8.79 THz).
 * \retval VRT_ERR_BOUNDS_IF_REFERENCE_FREQUENCY          IF reference frequency is outside valid bounds
 *                                                        (< ~-8.79 or > ~8.79 THz).
 * \retval VRT_ERR_BOUNDS_RF_REFERENCE_FREQUENCY          RF reference frequency is outside valid bounds
 *                                                        (< ~-8.79 or > ~8.79 THz).
 * \retval VRT_ERR_BOUNDS_RF_REFERENCE_FREQUENCY_OFFSET   RF reference frequency offset is outside valid bounds
 *                                                        (< ~-8.79 or > ~8.79 THz).
 * \retval VRT_ERR_BOUNDS_IF_BAND_OFFSET                  IF band offset is outside valid bounds
 *                                                        (< ~-8.79 or > ~8.79 THz).
 * \retval VRT_ERR_BOUNDS_REFERENCE_LEVEL                 Reference level is outside valid bounds
 *                                                        (< -256 or > ~256 dBm).
 * \retval VRT_ERR_BOUNDS_GAIN                            Gain stage 1 or 2 is outside valid bounds
 *                                                        (< -256 or > ~256 dB).
 * \retval VRT_ERR_GAIN_STAGE2_SET                        Gain stage 1 must be used instead of stage 2 when only one of
 *                                                        them is set.
 * \retval VRT_ERR_BOUNDS_SAMPLE_RATE                     Sample rate is outside valid bounds
 *                                                        (< 0 Hz or > ~8.79 THz).
 * \retval VRT_ERR_BOUNDS_TEMPERATURE                     Temperature is outside valid bounds
 *                                                        (< -273.15 or > ~512 degrees C).
 * \retval VRT_ERR_INVALID_PACKING_METHOD                 Packing method is an invalid value.
 * \retval VRT_ERR_INVALID_REAL_OR_COMPLEX                Real/Complex is an invalid value.
 * \retval VRT_ERR_INVALID_DATA_ITEM_FORMAT               Data item format is an invalid value.
 * \retval VRT_ERR_BOUNDS_EVENT_TAG_SIZE                  Event tag size is outside valid bounds (> 0x07).
 * \retval VRT_ERR_BOUNDS_CHANNEL_TAG_SIZE                Channel tag size is outside valid bounds (> 0x0F).
 * \retval VRT_ERR_BOUNDS_ITEM_PACKING_FIELD_SIZE         Item packing fields size is outside valid bounds (> 0x3F).
 * \retval VRT_ERR_BOUNDS_DATA_ITEM_SIZE                  Data item size is outside valid bounds (> 0x3F).
 * \retval VRT_ERR_SET_INTEGER_SECOND_TIMESTAMP           Integer second timestamp is not 0xFFFFFFFF when TSI is
 *                                                        VRT_TSI_UNDEFINED.
 * \retval VRT_ERR_SET_FRACTIONAL_SECOND_TIMESTAMP        Integer second timestamp is not 0xFFFFFFFF when TSI is
 *                                                        VRT_TSI_UNDEFINED.
 * \retval VRT_ERR_BOUNDS_LATITUDE                        Latitude is outside valid bounds (< -90 or > 90 degrees).
 * \retval VRT_ERR_BOUNDS_LONGITUDE                       Longitude is outside valid bounds (< -180 or > 180 degrees).
 * \retval VRT_ERR_BOUNDS_ALTITUDE                        Altitude is outside valid bounds (< ~-67108 or > ~67108 km).
 * \retval VRT_ERR_BOUNDS_SPEED_OVER_GROUND               Speed over ground is outside valid bounds
 *                                                        (< 0 or > ~65536 m/s).
 * \retval VRT_ERR_BOUNDS_HEADING_ANGLE                   Heading angle outside valid bounds (< 0 or > ~360
 *                                                        degrees).
 * \retval VRT_ERR_BOUNDS_TRACK_ANGLE                     Track angle is outside valid bounds (< 0 or > ~360
 *                                                        degrees).
 * \retval VRT_ERR_BOUNDS_MAGNETIC_VARIATION              Magnetic variation is outside valid bounds (< -180 or > 180
 *                                                        degrees).
 * \retval VRT_ERR_BOUNDS_POSITION                        Position x, y, or z is outside valid bounds
 *                                                        (< ~-67108 or > ~67108 km).
 * \retval VRT_ERR_BOUNDS_ATTITUDE                        Attitude alpha, beta, or gamma is outside valid bounds
 *                                                        (< -512 or > ~512 degrees).
 * \retval VRT_ERR_BOUNDS_VELOCITY                        Velocity dx, dy, or dz is outside valid bounds
 *                                                        (< -32768 or > ~32768 m/s).
 * \retval VRT_ERR_BOUNDS_SOURCE_LIST_SIZE                Source list size is outside valid bounds (> 0x01FF).
 * \retval VRT_ERR_BOUNDS_SYSTEM_LIST_SIZE                System list size is outside valid bounds (> 0x01FF).
 * \retval VRT_ERR_BOUNDS_CHANNEL_LIST_SIZE               Channel list size is outside valid bounds (> 0x7FFF).
 * \retval VRT_ERR_BOUNDS_PACKET_SIZE                     Calculated packet size is outside valid bounds (> 0xFFFF).
 * \retval VRT_ERR_MISSING_COMMAND                        Command packet, but packet->command is NULL.
 *
 * Command packets (VITA 49.2) are written as the command section from *packet->command, followed by the control
 * fields (control and query-state acknowledge packets) or the indicator bits (cancellation packets) from
 * packet->if_context. See vrt_write_header(), vrt_write_command() and vrt_write_if_context() for further errors. Ext
 * command packets have a custom body after the command section.
 *
 * \note Calculates packet_size field in header to match the number of written words. There is no need to calculate it
 *       beforehand. It is not modified in the const packet parameter, though.
 * \note May require output buffer data to be byte swapped if platform endianess isn't big endian (network order).
 * \note Will copy body data. For a zero-copy write, use the low-level routines.
 */
VRT_WARN_UNUSED
int32_t vrt_write_packet(const struct vrt_packet* packet, void* buf, int32_t words_buf, bool validate);

#ifdef __cplusplus
}
#endif

#endif
