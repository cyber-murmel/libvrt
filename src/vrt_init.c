#include "vrt/vrt_init.h"

#include "vrt/vrt_types.h"

#include "vrt_fixed_point.h"
#include "vrt_util_internal.h"

#include <stdbool.h>
#include <stddef.h>

void vrt_init_header(struct vrt_header* header) {
    header->packet_type      = VRT_PT_IF_DATA_WITHOUT_STREAM_ID;
    header->has.class_id     = false;
    header->has.trailer      = false;
    header->tsm              = VRT_TSM_FINE;
    header->nd0              = false;
    header->spectrum_or_time = VRT_SOT_TIME;
    header->acknowledge      = false;
    header->cancellation     = false;
    header->tsi              = VRT_TSI_NONE;
    header->tsf              = VRT_TSF_NONE;
    header->packet_count     = 0;
    header->packet_size      = 0;
}

void vrt_init_fields(struct vrt_fields* fields) {
    fields->stream_id                       = 0;
    fields->class_id.oui                    = 0;
    fields->class_id.information_class_code = 0;
    fields->class_id.packet_class_code      = 0;
    fields->integer_seconds_timestamp       = 0;
    fields->fractional_seconds_timestamp    = 0;
}

void vrt_init_trailer(struct vrt_trailer* trailer) {
    trailer->has.calibrated_time                 = false;
    trailer->has.valid_data                      = false;
    trailer->has.reference_lock                  = false;
    trailer->has.agc_or_mgc                      = false;
    trailer->has.detected_signal                 = false;
    trailer->has.spectral_inversion              = false;
    trailer->has.over_range                      = false;
    trailer->has.sample_loss                     = false;
    trailer->has.user_defined11                  = false;
    trailer->has.user_defined10                  = false;
    trailer->has.user_defined9                   = false;
    trailer->has.user_defined8                   = false;
    trailer->calibrated_time                     = false;
    trailer->valid_data                          = false;
    trailer->reference_lock                      = false;
    trailer->agc_or_mgc                          = VRT_AOM_MGC;
    trailer->detected_signal                     = false;
    trailer->spectral_inversion                  = false;
    trailer->over_range                          = false;
    trailer->sample_loss                         = false;
    trailer->user_defined11                      = false;
    trailer->user_defined10                      = false;
    trailer->user_defined9                       = false;
    trailer->user_defined8                       = false;
    trailer->has.associated_context_packet_count = false;
    trailer->associated_context_packet_count     = 0;
}

/**
 * Initialize Formatted GPS/INS geolocation struct.
 *
 * \param g Formatted geolocation.
 */
static void init_formatted_geolocation(struct vrt_formatted_geolocation* g) {
    g->tsi = VRT_TSI_UNDEFINED;
    g->tsf = VRT_TSF_UNDEFINED;
    g->oui = 0;
    /* Rule 7.1.5.19-1: When the TSI or TSF fields are zero the corresponding Timestamp of Position Fix subfield words
     * shall take the value 0xFFFFFFFF. */
    g->integer_second_timestamp    = VRT_UNSPECIFIED_TSI;
    g->fractional_second_timestamp = VRT_UNSPECIFIED_TSF;
    /* Rule 7.1.5.19-14: The Latitude, Longitude, Altitude, Speed Over Ground, Heading, Track Angle, and Magnetic
     * Variation subfields shall take the value 0x7FFFFFFF when unspecified. */

    g->has.latitude           = false;
    g->has.longitude          = false;
    g->has.altitude           = false;
    g->has.speed_over_ground  = false;
    g->has.heading_angle      = false;
    g->has.track_angle        = false;
    g->has.magnetic_variation = false;

    g->latitude           = vrt_fixed_point_i32_to_double(VRT_UNSPECIFIED_FIXED_POINT, VRT_RADIX_ANGLE);
    g->longitude          = vrt_fixed_point_i32_to_double(VRT_UNSPECIFIED_FIXED_POINT, VRT_RADIX_ANGLE);
    g->altitude           = vrt_fixed_point_i32_to_double(VRT_UNSPECIFIED_FIXED_POINT, VRT_RADIX_ALTITUDE);
    g->speed_over_ground  = vrt_fixed_point_u32_to_double(VRT_UNSPECIFIED_FIXED_POINT, VRT_RADIX_SPEED_VELOCITY);
    g->heading_angle      = vrt_fixed_point_i32_to_double(VRT_UNSPECIFIED_FIXED_POINT, VRT_RADIX_ANGLE);
    g->track_angle        = vrt_fixed_point_i32_to_double(VRT_UNSPECIFIED_FIXED_POINT, VRT_RADIX_ANGLE);
    g->magnetic_variation = vrt_fixed_point_i32_to_double(VRT_UNSPECIFIED_FIXED_POINT, VRT_RADIX_ANGLE);
}

/**
 * Initialize ECEF/Relative ephemeris struct.
 *
 * \param e Ephemeris.
 */
static void init_ephemeris(struct vrt_ephemeris* e) {
    e->tsi = VRT_TSI_UNDEFINED;
    e->tsf = VRT_TSF_UNDEFINED;
    e->oui = 0;

    e->has.position_x     = false;
    e->has.position_y     = false;
    e->has.position_z     = false;
    e->has.attitude_alpha = false;
    e->has.attitude_beta  = false;
    e->has.attitude_phi   = false;
    e->has.velocity_dx    = false;
    e->has.velocity_dy    = false;
    e->has.velocity_dz    = false;

    /* Rule 7.1.5.21-4: The TSI, TSF, OUI, and Timestamp of Position Fix fields shall follow the rules of the
     * corresponding Formatted GPS Geolocation fields given in Section 7.1.5.19. */
    /* Clash here between Rule 7.1.5.21-4 and Rule 7.1.5.21-8. Rule 7.1.5.21-4 seems more reasonable. */
    e->integer_second_timestamp    = VRT_UNSPECIFIED_TSI;
    e->fractional_second_timestamp = VRT_UNSPECIFIED_TSF;
    e->position_x                  = vrt_fixed_point_i32_to_double(VRT_UNSPECIFIED_FIXED_POINT, VRT_RADIX_POSITION);
    e->position_y                  = vrt_fixed_point_i32_to_double(VRT_UNSPECIFIED_FIXED_POINT, VRT_RADIX_POSITION);
    e->position_z                  = vrt_fixed_point_i32_to_double(VRT_UNSPECIFIED_FIXED_POINT, VRT_RADIX_POSITION);
    e->attitude_alpha              = vrt_fixed_point_i32_to_double(VRT_UNSPECIFIED_FIXED_POINT, VRT_RADIX_ANGLE);
    e->attitude_beta               = vrt_fixed_point_i32_to_double(VRT_UNSPECIFIED_FIXED_POINT, VRT_RADIX_ANGLE);
    e->attitude_phi                = vrt_fixed_point_i32_to_double(VRT_UNSPECIFIED_FIXED_POINT, VRT_RADIX_ANGLE);
    e->velocity_dx = vrt_fixed_point_i32_to_double(VRT_UNSPECIFIED_FIXED_POINT, VRT_RADIX_SPEED_VELOCITY);
    e->velocity_dy = vrt_fixed_point_i32_to_double(VRT_UNSPECIFIED_FIXED_POINT, VRT_RADIX_SPEED_VELOCITY);
    e->velocity_dz = vrt_fixed_point_i32_to_double(VRT_UNSPECIFIED_FIXED_POINT, VRT_RADIX_SPEED_VELOCITY);
}

void vrt_init_if_context(struct vrt_if_context* if_context) {
    if_context->context_field_change_indicator     = false;
    if_context->has.reference_point_identifier     = false;
    if_context->has.bandwidth                      = false;
    if_context->has.if_reference_frequency         = false;
    if_context->has.rf_reference_frequency         = false;
    if_context->has.rf_reference_frequency_offset  = false;
    if_context->has.if_band_offset                 = false;
    if_context->has.reference_level                = false;
    if_context->has.gain                           = false;
    if_context->has.over_range_count               = false;
    if_context->has.sample_rate                    = false;
    if_context->has.timestamp_adjustment           = false;
    if_context->has.timestamp_calibration_time     = false;
    if_context->has.temperature                    = false;
    if_context->has.device_identifier              = false;
    if_context->has.state_and_event_indicators     = false;
    if_context->has.data_packet_payload_format     = false;
    if_context->has.formatted_gps_geolocation      = false;
    if_context->has.formatted_ins_geolocation      = false;
    if_context->has.ecef_ephemeris                 = false;
    if_context->has.relative_ephemeris             = false;
    if_context->has.ephemeris_reference_identifier = false;
    if_context->has.gps_ascii                      = false;
    if_context->has.context_association_lists      = false;
    if_context->has.health_status                  = false;
    if_context->has.v49_spec_compliance            = false;
    if_context->has.version_build_code             = false;

    if_context->reference_point_identifier    = 0;
    if_context->bandwidth                     = 0.0;
    if_context->if_reference_frequency        = 0.0;
    if_context->rf_reference_frequency        = 0.0;
    if_context->rf_reference_frequency_offset = 0.0;
    if_context->if_band_offset                = 0.0;
    if_context->reference_level               = 0.0F;
    if_context->gain.stage1                   = 0.0F;
    if_context->gain.stage2                   = 0.0F;
    if_context->over_range_count              = 0;
    if_context->sample_rate                   = 0.0;
    if_context->timestamp_adjustment          = 0;
    if_context->timestamp_calibration_time    = 0;
    if_context->temperature                   = 0.0F;

    if_context->device_identifier.oui         = 0;
    if_context->device_identifier.device_code = 0;

    if_context->state_and_event_indicators.has.calibrated_time    = false;
    if_context->state_and_event_indicators.has.valid_data         = false;
    if_context->state_and_event_indicators.has.reference_lock     = false;
    if_context->state_and_event_indicators.has.agc_or_mgc         = false;
    if_context->state_and_event_indicators.has.detected_signal    = false;
    if_context->state_and_event_indicators.has.spectral_inversion = false;
    if_context->state_and_event_indicators.has.over_range         = false;
    if_context->state_and_event_indicators.has.sample_loss        = false;
    if_context->state_and_event_indicators.calibrated_time        = false;
    if_context->state_and_event_indicators.valid_data             = false;
    if_context->state_and_event_indicators.reference_lock         = false;
    if_context->state_and_event_indicators.agc_or_mgc             = VRT_AOM_MGC;
    if_context->state_and_event_indicators.detected_signal        = false;
    if_context->state_and_event_indicators.spectral_inversion     = false;
    if_context->state_and_event_indicators.over_range             = false;
    if_context->state_and_event_indicators.sample_loss            = false;
    if_context->state_and_event_indicators.user_defined           = 0;

    if_context->data_packet_payload_format.packing_method          = VRT_PM_PROCESSING_EFFICIENT;
    if_context->data_packet_payload_format.real_or_complex         = VRT_ROC_REAL;
    if_context->data_packet_payload_format.data_item_format        = VRT_DIF_SIGNED_FIXED_POINT;
    if_context->data_packet_payload_format.sample_component_repeat = false;
    if_context->data_packet_payload_format.event_tag_size          = 0;
    if_context->data_packet_payload_format.channel_tag_size        = 0;
    if_context->data_packet_payload_format.item_packing_field_size = 0;
    if_context->data_packet_payload_format.data_item_size          = 0;
    if_context->data_packet_payload_format.repeat_count            = 0;
    if_context->data_packet_payload_format.vector_size             = 0;

    init_formatted_geolocation(&if_context->formatted_gps_geolocation);
    init_formatted_geolocation(&if_context->formatted_ins_geolocation);
    init_ephemeris(&if_context->ecef_ephemeris);
    init_ephemeris(&if_context->relative_ephemeris);

    if_context->ephemeris_reference_identifier = 0;

    if_context->gps_ascii.oui             = 0;
    if_context->gps_ascii.number_of_words = 0;
    if_context->gps_ascii.ascii           = NULL;

    if_context->context_association_lists.has.asynchronous_channel_tag_list             = false;
    if_context->context_association_lists.source_list_size                              = 0;
    if_context->context_association_lists.system_list_size                              = 0;
    if_context->context_association_lists.vector_component_list_size                    = 0;
    if_context->context_association_lists.asynchronous_channel_list_size                = 0;
    if_context->context_association_lists.source_context_association_list               = NULL;
    if_context->context_association_lists.system_context_association_list               = NULL;
    if_context->context_association_lists.vector_component_context_association_list     = NULL;
    if_context->context_association_lists.asynchronous_channel_context_association_list = NULL;
    if_context->context_association_lists.asynchronous_channel_tag_list                 = NULL;

    if_context->health_status                   = 0;
    if_context->v49_spec_compliance             = VRT_V49_2;
    if_context->version_build_code.year         = 0;
    if_context->version_build_code.day          = 0;
    if_context->version_build_code.revision     = 0;
    if_context->version_build_code.user_defined = 0;
}

/**
 * Initialize warning or error responses of acknowledge packets.
 *
 * \param r Responses.
 */
static void init_ack_responses(struct vrt_ack_responses* r) {
    vrt_init_context_indicators(&r->has);
    r->reference_point_identifier     = 0;
    r->bandwidth                      = 0;
    r->if_reference_frequency         = 0;
    r->rf_reference_frequency         = 0;
    r->rf_reference_frequency_offset  = 0;
    r->if_band_offset                 = 0;
    r->reference_level                = 0;
    r->gain                           = 0;
    r->over_range_count               = 0;
    r->sample_rate                    = 0;
    r->timestamp_adjustment           = 0;
    r->timestamp_calibration_time     = 0;
    r->temperature                    = 0;
    r->device_identifier              = 0;
    r->state_and_event_indicators     = 0;
    r->data_packet_payload_format     = 0;
    r->formatted_gps_geolocation      = 0;
    r->formatted_ins_geolocation      = 0;
    r->ecef_ephemeris                 = 0;
    r->relative_ephemeris             = 0;
    r->ephemeris_reference_identifier = 0;
    r->gps_ascii                      = 0;
    r->context_association_lists      = 0;
    r->health_status                  = 0;
    r->v49_spec_compliance            = 0;
    r->version_build_code             = 0;
}

void vrt_init_command(struct vrt_command* command) {
    struct vrt_cam* cam = &command->cam;

    cam->controllee_enable     = false;
    cam->controllee_format     = VRT_IDF_WORD;
    cam->controller_enable     = false;
    cam->controller_format     = VRT_IDF_WORD;
    cam->permit_partial        = false;
    cam->permit_warnings       = false;
    cam->permit_errors         = false;
    cam->action_mode           = VRT_AM_NO_ACTION;
    cam->nack_only             = false;
    cam->validation            = false;
    cam->execution             = false;
    cam->query_state           = false;
    cam->warnings              = false;
    cam->errors                = false;
    cam->timing_control        = VRT_TC_IGNORE;
    cam->partial_action        = false;
    cam->scheduled_or_executed = false;

    command->message_id    = 0;
    command->controllee_id = 0;
    command->controller_id = 0;
    for (int i = 0; i < 4; ++i) {
        command->controllee_uuid[i] = 0;
        command->controller_uuid[i] = 0;
    }
    init_ack_responses(&command->warnings);
    init_ack_responses(&command->errors);
}

void vrt_init_packet(struct vrt_packet* packet) {
    vrt_init_header(&packet->header);
    vrt_init_fields(&packet->fields);
    packet->body       = NULL;
    packet->words_body = 0;
    vrt_init_trailer(&packet->trailer);
    vrt_init_if_context(&packet->if_context);
    packet->command = NULL;
}
