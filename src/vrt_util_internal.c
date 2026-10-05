#include "vrt_util_internal.h"

#include "vrt/vrt_types.h"

/* Defined inline in header */
extern uint32_t                 vrt_b2u(bool b);
extern uint32_t                 vrt_u2b(uint32_t u);
extern bool                     vrt_has_fractional_timestamp(enum vrt_tsf t);
extern bool                     vrt_has_cif1(const struct vrt_context_indicators* ind);
extern int32_t                  vrt_words_indicator_words(const struct vrt_context_indicators* ind);
extern int32_t                  vrt_words_identifier(bool enable, enum vrt_identifier_format format);
extern bool                     vrt_has_ack_responses(const struct vrt_header* header, const struct vrt_cam* cam);
extern enum vrt_command_payload vrt_command_payload(const struct vrt_header* header, const struct vrt_cam* cam);

int32_t vrt_words_if_context_indicator(const struct vrt_context_indicators* ind) {
    /* For context indicator field */
    int32_t words = 1;

    /* Note that Context Field Change Identifier contributes with 0 words, since it's only in
     * the context indicator field. */
    if (ind->reference_point_identifier) {
        words += 1;
    }
    if (ind->bandwidth) {
        words += 2;
    }
    if (ind->if_reference_frequency) {
        words += 2;
    }
    if (ind->rf_reference_frequency) {
        words += 2;
    }
    if (ind->rf_reference_frequency_offset) {
        words += 2;
    }
    if (ind->if_band_offset) {
        words += 2;
    }
    if (ind->reference_level) {
        words += 1;
    }
    if (ind->gain) {
        words += 1;
    }
    if (ind->over_range_count) {
        words += 1;
    }
    if (ind->sample_rate) {
        words += 2;
    }
    if (ind->timestamp_adjustment) {
        words += 2;
    }
    if (ind->timestamp_calibration_time) {
        words += 1;
    }
    if (ind->temperature) {
        words += 1;
    }
    if (ind->device_identifier) {
        words += 2;
    }
    if (ind->state_and_event_indicators) {
        words += 1;
    }
    if (ind->data_packet_payload_format) {
        words += 2;
    }
    if (ind->formatted_gps_geolocation) {
        words += 11;
    }
    if (ind->formatted_ins_geolocation) {
        words += 11;
    }
    if (ind->ecef_ephemeris) {
        words += 13;
    }
    if (ind->relative_ephemeris) {
        words += 13;
    }
    if (ind->ephemeris_reference_identifier) {
        words += 1;
    }

    /* CIF1 word and fields */
    if (vrt_has_cif1(ind)) {
        words += 1;
    }
    if (ind->health_status) {
        words += 1;
    }
    if (ind->v49_spec_compliance) {
        words += 1;
    }
    if (ind->version_build_code) {
        words += 1;
    }

    return words;
}

int32_t vrt_words_ack_response_words(const struct vrt_context_indicators* ind) {
    const bool has[] = {ind->reference_point_identifier,
                        ind->bandwidth,
                        ind->if_reference_frequency,
                        ind->rf_reference_frequency,
                        ind->rf_reference_frequency_offset,
                        ind->if_band_offset,
                        ind->reference_level,
                        ind->gain,
                        ind->over_range_count,
                        ind->sample_rate,
                        ind->timestamp_adjustment,
                        ind->timestamp_calibration_time,
                        ind->temperature,
                        ind->device_identifier,
                        ind->state_and_event_indicators,
                        ind->data_packet_payload_format,
                        ind->formatted_gps_geolocation,
                        ind->formatted_ins_geolocation,
                        ind->ecef_ephemeris,
                        ind->relative_ephemeris,
                        ind->ephemeris_reference_identifier,
                        ind->gps_ascii,
                        ind->context_association_lists,
                        ind->health_status,
                        ind->v49_spec_compliance,
                        ind->version_build_code};

    /* One response word per field, regardless of field size */
    int32_t words = 0;
    for (uint32_t i = 0; i < sizeof(has) / sizeof(has[0]); ++i) {
        if (has[i]) {
            words += 1;
        }
    }

    return words;
}

void vrt_init_context_indicators(struct vrt_context_indicators* ind) {
    ind->reference_point_identifier     = false;
    ind->bandwidth                      = false;
    ind->if_reference_frequency         = false;
    ind->rf_reference_frequency         = false;
    ind->rf_reference_frequency_offset  = false;
    ind->if_band_offset                 = false;
    ind->reference_level                = false;
    ind->gain                           = false;
    ind->over_range_count               = false;
    ind->sample_rate                    = false;
    ind->timestamp_adjustment           = false;
    ind->timestamp_calibration_time     = false;
    ind->temperature                    = false;
    ind->device_identifier              = false;
    ind->state_and_event_indicators     = false;
    ind->data_packet_payload_format     = false;
    ind->formatted_gps_geolocation      = false;
    ind->formatted_ins_geolocation      = false;
    ind->ecef_ephemeris                 = false;
    ind->relative_ephemeris             = false;
    ind->ephemeris_reference_identifier = false;
    ind->gps_ascii                      = false;
    ind->context_association_lists      = false;
    ind->health_status                  = false;
    ind->v49_spec_compliance            = false;
    ind->version_build_code             = false;
}
