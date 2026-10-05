#ifndef INCLUDE_VRT_VRT_TYPES_H_
#define INCLUDE_VRT_VRT_TYPES_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Type of packet.
 *
 * \note Remaining values are reserved for future packet types.
 * \note IF data packets are called Signal Data packets in VITA 49.2.
 */
enum vrt_packet_type {
    VRT_PT_IF_DATA_WITHOUT_STREAM_ID  = 0x0, /**< IF data packet without stream identifier. */
    VRT_PT_IF_DATA_WITH_STREAM_ID     = 0x1, /**< IF data packet with stream identifier. */
    VRT_PT_EXT_DATA_WITHOUT_STREAM_ID = 0x2, /**< Extension data packet without stream identifier. */
    VRT_PT_EXT_DATA_WITH_STREAM_ID    = 0x3, /**< Extension data packet with stream identifier. */
    VRT_PT_IF_CONTEXT                 = 0x4, /**< IF context packet. */
    VRT_PT_EXT_CONTEXT                = 0x5, /**< Extension context packet. */
    VRT_PT_COMMAND                    = 0x6, /**< Command packet (VITA 49.2). Always has a stream identifier. */
    VRT_PT_EXT_COMMAND = 0x7 /**< Extension command packet (VITA 49.2). Always has a stream identifier. */
};

/**
 * Indicates presence of header fields.
 */
struct vrt_header_indicators {
    /** True if there is class identifier information. */
    bool class_id;
    /**
     *  True if there is a trailer word.
     *
     * \note True is only valid for IF and Extension data packets.
     */
    bool trailer;
};

/**
 * Timestamp mode. Resolution of timestamps.
 */
enum vrt_tsm {
    VRT_TSM_FINE   = 0x0, /**< Precise resolution. */
    VRT_TSM_COARSE = 0x1 /**< Context events occurred sometime within the Data Sampling Interval of a Data packet in the
                              paired Data Packet Stream. */
};

/**
 * Reference-point time in resolution of a second.
 */
enum vrt_tsi {
    VRT_TSI_NONE      = 0x0, /**< No one-second resolution time. */
    VRT_TSI_UNDEFINED = 0x0, /**< Replaces NONE in the Formatted GPS/INS geolocation fields. */
    VRT_TSI_UTC       = 0x1, /**< Seconds since January 1, 1970, GMT. */
    VRT_TSI_GPS       = 0x2, /**< Seconds since January 6, 1980. */
    VRT_TSI_OTHER     = 0x3  /**< Some other time reference. */
};

/**
 * Reference-point time in subsecond resolution.
 */
enum vrt_tsf {
    VRT_TSF_NONE               = 0x0, /**< No subsecond time resolution. */
    VRT_TSF_UNDEFINED          = 0x0, /**< Replaces NONE in the Formatted GPS/INS geolocation fields. */
    VRT_TSF_SAMPLE_COUNT       = 0x1, /**< Number of samples since last second shift. */
    VRT_TSF_REAL_TIME          = 0x2, /**< Picoseconds since last second shift. */
    VRT_TSF_FREE_RUNNING_COUNT = 0x3  /**< Counts samples from the first sample. Will eventually roll over. */
};

/**
 * Signal data packet content, i.e. the S indicator bit (VITA 49.2).
 */
enum vrt_spectrum_or_time {
    VRT_SOT_TIME     = 0x0, /**< Payload contains time domain samples. */
    VRT_SOT_SPECTRUM = 0x1  /**< Payload contains spectrum (frequency domain) data. */
};

/**
 * Header data.
 *
 * \note Header bits 26 to 24 are packet type specific. Bit 26 is has.trailer (data) or acknowledge (command), bit 25
 *       is nd0 (data, context), and bit 24 is spectrum_or_time (data), tsm (context) or cancellation (command).
 */
struct vrt_header {
    /** Type of packet. */
    enum vrt_packet_type packet_type;
    /** Indicates presence of extra fields. */
    struct vrt_header_indicators has;
    /**
     * Timestamp mode. Whether timestamps are with fine or coarse resolution. Coarse if an event happened sometime in
     * the data sampling interval.
     *
     * \note True is only valid for context packets.
     */
    enum vrt_tsm tsm;
    /**
     * Not a V49.0 packet indicator (Nd0, VITA 49.2). Set when the packet uses VITA 49.2 features, such as CIF1 fields
     * or a spectrum payload, that a VITA 49.0 receiver cannot interpret.
     *
     * \note Only valid for data and context packets. The bit is reserved in command packets, but some implementations
     *       set it there, so reading accepts it while writing with validation rejects it.
     */
    bool nd0;
    /**
     * Whether the payload contains time or spectrum data (S, VITA 49.2).
     *
     * \note Only valid for data packets.
     */
    enum vrt_spectrum_or_time spectrum_or_time;
    /**
     * True if this is an acknowledge packet (A, VITA 49.2).
     *
     * \note Only valid for command packets.
     */
    bool acknowledge;
    /**
     * True if this is a cancellation packet (CE, VITA 49.2).
     *
     * \note Only valid for command packets.
     */
    bool cancellation;
    /**
     * Type of integer second timestamp.
     */
    enum vrt_tsi tsi;
    /**
     * Type of fractional second timestamp.
     */
    enum vrt_tsf tsf;
    /**
     * Modulo-16 count of packets for the specified stream. Can (to some degree) be used to determine packet loss.
     *
     * \note Only the 4 least significant bits are used.
     * \note Rule 6.1.1-11: The Packet Count shall increment in each consecutive IF Data packet having the same Stream
     *       Identifier and the same packet type. The count shall roll over from “1111” to “0000”.
     */
    uint8_t packet_count;
    /**
     * Total number of 32-bit words in the packet.
     *
     * \note May vary from packet to packet.
     */
    uint16_t packet_size;
};

/**
 * Class identifier. Determines data origin.
 */
struct vrt_class_identifier {
    /**
     * IEEE Organizationally Unique Identifier. Identifies a company.
     *
     * \note Only the 24 least significant bits are used.
     */
    uint32_t oui;
    /**
     * Information class to which this packet stream belongs. A value of zero in this field shall indicate that the
     * Information Class is unspecified.
     */
    uint16_t information_class_code;
    /**
     * Packet class of company to which this packet stream belongs. A value of zero in this field shall indicate that
     * the Packet Class is unspecified.
     */
    uint16_t packet_class_code;
};

/**
 * Field data.
 */
struct vrt_fields {
    /**
     * Identifies which stream it belongs to. Activate by setting packet_type accordingly.
     *
     * \note Although this field is optional for IF data packets it is required for context packets.
     */
    uint32_t stream_id;
    /** Determines data origin. Activate by setting has.class_id. */
    struct vrt_class_identifier class_id;
    /**
     * Integer second timestamp as specified by tsi. Activate by setting tsi to anything but
     * VRT_TSI_NONE/VRT_TSI_UNDEFINED.
     */
    uint32_t integer_seconds_timestamp;
    /** Fractional second timestamp as specified by tsf. Activate by setting tsf to anything but
     * VRT_TSF_NONE/VRT_TSF_UNDEFINED.
     */
    uint64_t fractional_seconds_timestamp;
};

/**
 * Indicates presence of trailer fields.
 */
struct vrt_trailer_indicators {
    bool calibrated_time;                 /**< True if it has the Calibrated time indicator field. */
    bool valid_data;                      /**< True if it has the Valid data indicator field. */
    bool reference_lock;                  /**< True if it has the Reference lock indicator field. */
    bool agc_or_mgc;                      /**< True if it has the AGC/MGC indicator field. */
    bool detected_signal;                 /**< True if it has the Detected signal indicator field. */
    bool spectral_inversion;              /**< True if it has the Spectral inversion indicator field. */
    bool over_range;                      /**< True if it has the Over-range indicator field. */
    bool sample_loss;                     /**< True if it has the Sample loss indicator field. */
    bool user_defined11;                  /**< True if it has the User defined bit 11 indicator field. */
    bool user_defined10;                  /**< True if it has the User defined bit 10 indicator field. */
    bool user_defined9;                   /**< True if it has the User defined bit 9 indicator field. */
    bool user_defined8;                   /**< True if it has the User defined bit 8 indicator field. */
    bool associated_context_packet_count; /**< True if it has the Associated context packet count field. */
};

/**
 * Indicates if AGC (Automatic Gain Control) or MGC (Manual Gain Control) is active.
 */
enum vrt_agc_or_mgc { VRT_AOM_MGC = 0x0, VRT_AOM_AGC = 0x1 };

/**
 * Trailer section data.
 */
struct vrt_trailer {
    /** Field presence indicators. */
    struct vrt_trailer_indicators has;
    /**
     * True if the timestamp is calibrated to an external reference. Activate by setting has.calibrated_time to true.
     */
    bool calibrated_time;
    /** True if the packet data is valid. Activate by setting has.valid_data to true. */
    bool valid_data;
    /** True if the reference is phase locked and stable. Activate by setting has.reference_lock to true. */
    bool reference_lock;
    /** Whether AGC or MGC is active. Activate by setting has.agc_or_mgc to true. */
    enum vrt_agc_or_mgc agc_or_mgc;
    /** True if a signal is detected in the packet. Activate by setting has.detected_signal to true. */
    bool detected_signal;
    /** True if the signal has an inverted spectrum. Activate by setting has.spectral_inversion to true. */
    bool spectral_inversion;
    /**
     * True if at least one data sample in the packet exceeds the range of a sample. Activate by setting has.over_range
     * to true.
     */
    bool over_range;
    /** Contains at least one sample discontinuity. Activate by setting has.sample_loss to true. */
    bool sample_loss;
    /** Bit 11 in the trailer word. Defined by the user. Activate by setting has.user_defined11 to true. */
    bool user_defined11;
    /** Bit 10 in the trailer word. Defined by the user. Activate by setting has.user_defined10 to true. */
    bool user_defined10;
    /** Bit 9 in the trailer word. Defined by the user. Activate by setting has.user_defined9 to true. */
    bool user_defined9;
    /** Bit 8 in the trailer word. Defined by the user. Activate by setting has.user_defined8 to true. */
    bool user_defined8;
    /**
     * Number of context packets associated to this packet. Activate by setting has.associated_context_packet_count to
     * True.
     *
     * \note Only the 7 least significant bits are used.
     */
    uint8_t associated_context_packet_count;
};

/**
 * Indicates presence of context fields.
 */
struct vrt_context_indicators {
    bool reference_point_identifier;     /**< True if it has the Reference point identifier field. */
    bool bandwidth;                      /**< True if it has the Bandwidth field. */
    bool if_reference_frequency;         /**< True if it has the IF reference frequency field. */
    bool rf_reference_frequency;         /**< True if it has the RF reference frequency field. */
    bool rf_reference_frequency_offset;  /**< True if it has the RF reference frequency offset field. */
    bool if_band_offset;                 /**< True if it has the IF band offset field. */
    bool reference_level;                /**< True if it has the Reference level field. */
    bool gain;                           /**< True if it has the Gain stage 1 and 2 fields.*/
    bool over_range_count;               /**< True if it has the Over-range count field. */
    bool sample_rate;                    /**< True if it has the Sample rate field. */
    bool timestamp_adjustment;           /**< True if it has the Timestamp adjustment field. */
    bool timestamp_calibration_time;     /**< True if it has the Timestamp calibration time field. */
    bool temperature;                    /**< True if it has the Temperature field. */
    bool device_identifier;              /**< True if it has the Device identifier field. */
    bool state_and_event_indicators;     /**< True if it has the State and event indicators field. */
    bool data_packet_payload_format;     /**< True if it has the Data packet payload format field. */
    bool formatted_gps_geolocation;      /**< True if it has the Formatted GPS geolocation field. */
    bool formatted_ins_geolocation;      /**< True if it has the Formatted INS geolocation field. */
    bool ecef_ephemeris;                 /**< True if it has the ECEF ephemeris field. */
    bool relative_ephemeris;             /**< True if it has the Relative ephemeris field. */
    bool ephemeris_reference_identifier; /**< True if it has the Ephemeris reference identifier field. */
    bool gps_ascii;                      /**< True if it has the GPS ASCII field. */
    bool context_association_lists;      /**< True if it has the Context association list field. */
    /*
     * CIF1 (VITA 49.2). The CIF1 word is written exactly when at least one of these is true. Other CIF1 fields, and the
     * CIF2, CIF3 and CIF7 words, are not supported.
     */
    bool health_status;       /**< True if it has the Health status field (CIF1). */
    bool v49_spec_compliance; /**< True if it has the V49 spec compliance field (CIF1). */
    bool version_build_code;  /**< True if it has the Version and build code field (CIF1). */
};

/**
 * Signal gain or attenuation.
 */
struct vrt_gain {
    float stage1; /**< Front-end or RF gain [dB]. */
    float stage2; /**< Back-end or IF gain [dB]. */
};

/**
 * Identifies manufacturer and model of device generating context stream.
 */
struct vrt_device_identifier {
    /**
     * IEEE Organizationally Unique Identifier (company identifier) of device.
     *
     * \note Only the 24 least significant bits are used.
     */
    uint32_t oui;
    /** Model identification. */
    uint16_t device_code;
};

/**
 * Indicates presence of vrt_state_and_event fields.
 */
struct vrt_state_and_event_indicators {
    bool calibrated_time;    /**< True if it has the Calibrated time indicator field. */
    bool valid_data;         /**< True if it has the Valid data indicator field. */
    bool reference_lock;     /**< True if it has the Reference lock indicator field. */
    bool agc_or_mgc;         /**< True if it has the AGC/MGC indicator field. */
    bool detected_signal;    /**< True if it has the Detected signal indicator field. */
    bool spectral_inversion; /**< True if it has the Spectral inversion indicator field. */
    bool over_range;         /**< True if it has the Over-range indicator field. */
    bool sample_loss;        /**< True if it has the Sample loss indicator field. */
};

/**
 * Binary state and event indicators.
 */
struct vrt_state_and_event {
    /** Field presence indicators. */
    struct vrt_state_and_event_indicators has;
    /**
     * True if the timestamp is calibrated to an external reference. Activate by setting has.calibrated_time to true.
     */
    bool calibrated_time;
    /** True if the packet data is valid. Activate by setting has.valid_data to true. */
    bool valid_data;
    /** True if the reference is phase locked and stable. Activate by setting has.reference_lock to true. */
    bool reference_lock;
    /** Whether AGC or MGC is active. Activate by setting has.agc_or_mgc to true. */
    enum vrt_agc_or_mgc agc_or_mgc;
    /** True if a signal is detected in the packet. Activate by setting has.detected_signal to true. */
    bool detected_signal;
    /** True if a signal in the data has an inverted spectrum. Activate by setting has.spectral_inversion to true. */
    bool spectral_inversion;
    /**
     * True if at least one data sample in the packet exceeds the range of a sample. Activate by setting has.over_range
     * to true. The definition of this fields changes depending on the TSM field.
     */
    bool over_range;
    /**
     * Contains at least one sample discontinuity. Activate by setting has.sample_loss to true.
     */
    bool sample_loss;
    /**
     * State and event bits 7-0. Defined by the user.
     */
    uint8_t user_defined;
};

/**
 * Packing method.
 */
enum vrt_packing_method { VRT_PM_PROCESSING_EFFICIENT = 0x0, VRT_PM_LINK_EFFICIENT = 0x1 };

/**
 * Indicates data sample type.
 */
enum vrt_real_complex {
    VRT_ROC_REAL              = 0x0, /**< Real number. */
    VRT_ROC_COMPLEX_CARTESIAN = 0x1, /**< Cartesian complex number. */
    VRT_ROC_COMPLEX_POLAR     = 0x2  /**< Polar complex number. */
};

/**
 * Sample format type.
 */
enum vrt_data_item_format {
    VRT_DIF_SIGNED_FIXED_POINT                       = 0x00,
    VRT_DIF_SIGNED_VRT_1_BIT_EXPONENT                = 0x01,
    VRT_DIF_SIGNED_VRT_2_BIT_EXPONENT                = 0x02,
    VRT_DIF_SIGNED_VRT_3_BIT_EXPONENT                = 0x03,
    VRT_DIF_SIGNED_VRT_4_BIT_EXPONENT                = 0x04,
    VRT_DIF_SIGNED_VRT_5_BIT_EXPONENT                = 0x05,
    VRT_DIF_SIGNED_VRT_6_BIT_EXPONENT                = 0x06,
    VRT_DIF_IEEE_754_SINGLE_PRECISION_FLOATING_POINT = 0x0E,
    VRT_DIF_IEEE_754_DOUBLE_PRECISION_FLOATING_POINT = 0x0F,
    VRT_DIF_UNSIGNED_FIXED_POINT                     = 0x10,
    VRT_DIF_UNSIGNED_VRT_1_BIT_EXPONENT              = 0x11,
    VRT_DIF_UNSIGNED_VRT_2_BIT_EXPONENT              = 0x12,
    VRT_DIF_UNSIGNED_VRT_3_BIT_EXPONENT              = 0x13,
    VRT_DIF_UNSIGNED_VRT_4_BIT_EXPONENT              = 0x14,
    VRT_DIF_UNSIGNED_VRT_5_BIT_EXPONENT              = 0x15,
    VRT_DIF_UNSIGNED_VRT_6_BIT_EXPONENT              = 0x16
};

/**
 * Format of data packet payload.
 *
 * \note Some fields regarding size contain a number that is one less than the actual size, while some contain a number
 *       equal to the actual size.
 */
struct vrt_data_packet_payload_format {
    /** If link-efficient or processing-efficient packing is used. */
    enum vrt_packing_method packing_method;
    /** Data sample type. */
    enum vrt_real_complex real_or_complex;
    /** Data sample format. */
    enum vrt_data_item_format data_item_format;
    /** True when sample component repeating is used. */
    bool sample_component_repeat;
    /**
     * Equal to event tag size in paired packet data stream.
     *
     * \note Only the 3 least significant bits are used.
     */
    uint8_t event_tag_size;
    /**
     * Equal to channel tag size in paired channel data stream.
     *
     * \note Only the 4 least significant bits are used.
     */
    uint8_t channel_tag_size;
    /**
     * One less than item packing fields size in paired channel data stream.
     *
     * \note Only the 6 least significant bits are used.
     */
    uint8_t item_packing_field_size;
    /**
     * One less than data item size in paired channel data stream.
     *
     * \note Only the 3 least significant bits are used.
     */
    uint8_t data_item_size;
    /** One less than repeat count in paired channel data stream. */
    uint16_t repeat_count;
    /** One less than vector size in paired channel data stream. */
    uint16_t vector_size;
};

/**
 * Formatted GPS/INS geolocation indicators.
 */
struct vrt_formatted_geolocation_indicators {
    bool latitude;           /**< True if it has the Latitude field. */
    bool longitude;          /**< True if it has the Longitude field. */
    bool altitude;           /**< True if it has the Altitude field */
    bool speed_over_ground;  /**< True if it has the Speed over ground field. */
    bool heading_angle;      /**< True if it has the heading angle field. */
    bool track_angle;        /**< True if it has the Track angle field. */
    bool magnetic_variation; /**< True if it has the Magnetic variation field. */
};

/**
 * Formatted GPS/INS geolocation data.
 */
struct vrt_formatted_geolocation {
    /** Type of integer second timestamp. */
    enum vrt_tsi tsi;
    /** Type of fractional second timestamp. */
    enum vrt_tsf tsf;
    /**
     * IEEE Organizationally Unique Identifier of GPS/INS manufacturer.
     *
     * \note Only the 24 least significant bits are used.
     */
    uint32_t oui;
    /** Integer second timestamp of position fix as specified by tsi. */
    uint32_t integer_second_timestamp;
    /** Fractional second timestamp of position fix as specified by tsf. */
    uint64_t fractional_second_timestamp;
    /** Field presence indicators. */
    struct vrt_formatted_geolocation_indicators has;
    /** Latitude [degrees]. Activate by setting has.latitude to true. */
    double latitude;
    /** Longitude [degrees]. Activate by setting has.longitude to true. */
    double longitude;
    /** Altitude [m]. Activate by setting has.altitude to true. */
    double altitude;
    /** Speed over ground [m/s]. Activate by setting has.speed_over_ground to true. */
    double speed_over_ground;
    /** Orientation with respect to true north [degrees]. Activate by setting has.heading_angle to true. */
    double heading_angle;
    /** Travel direction with respect to true north [degrees]. Activate by setting has.track_angle to true. */
    double track_angle;
    /** Magnetic variation with respect to true north [degrees]. Activate by setting has.magnetic_variation to true. */
    double magnetic_variation;
};

/**
 * ECEF/Relative ephemeris indicators.
 */
struct vrt_ephemeris_indicators {
    bool position_x;     /**< True if it has the Position X field. */
    bool position_y;     /**< True if it has the Position Y field. */
    bool position_z;     /**< True if it has the Position Z field. */
    bool attitude_alpha; /**< True if it has the Attitude alpha field. */
    bool attitude_beta;  /**< True if it has the Attitude beta field. */
    bool attitude_phi;   /**< True if it has the Attitude phi field. */
    bool velocity_dx;    /**< True if it has the Velocity dX field. */
    bool velocity_dy;    /**< True if it has the Velocity dY field. */
    bool velocity_dz;    /**< True if it has the Velocity dZ field. */
};

/**
 * Location in a specific coordinate system.
 */
struct vrt_ephemeris {
    /** Type of integer second timestamp. */
    enum vrt_tsi tsi;
    /** Type of fractional second timestamp. */
    enum vrt_tsf tsf;
    /**
     * IEEE Organizationally Unique Identifier of GPS/INS manufacturer.
     *
     * \note Only the 24 least significant bits are valid.
     */
    uint32_t oui;
    /** Integer second timestamp of position fix as specified by tsi. */
    uint32_t integer_second_timestamp;
    /** Fractional second timestamp of position fix  as specified by tsf. */
    uint64_t fractional_second_timestamp;
    /** Field presence indicators. */
    struct vrt_ephemeris_indicators has;
    /**
     * x-position [m]. Along earth's equator. Positive direction is intersection of equator plane and prime meridian.
     * Activate by setting has.position_x to true.
     */
    double position_x;
    /** y-position [m]. Completes a right-handed orthogonal system 90 degrees east of the x-axis. Activate by setting
     * has.position_y to true. */
    double position_y;
    /** z-position [m]. Directed along earth's rotational axis, where north is positive. Activate by setting
     * has.position_z to true. */
    double position_z;
    /** Attitude coordinate about z-axis [degrees]. Positive rotation is x to y. Activate by setting has.attitude_alpha
     * to true. */
    double attitude_alpha;
    /** Attitude coordinate about y-axis [degrees]. Positive direction is x to z. Activate by setting has.attitude_beta
     * to true. */
    double attitude_beta;
    /** Attitude coordinate about x-axis [degrees]. Positive direction is y to z. Activate by setting has.attitude_phi
     * to true. */
    double attitude_phi;
    /** x-velocity [m/s]. Defined similarly to x-position. Activate by setting has.velocity_dx to true. */
    double velocity_dx;
    /** y-velocity [m/s]. Defined similarly to y-position. Activate by setting has.velocity_dy to true. */
    double velocity_dy;
    /** z-position [m/s]. Defined similarly to z-position. Activate by setting has.velocity_dz to true. */
    double velocity_dz;
};

/**
 * GPS device ASCII.
 */
struct vrt_gps_ascii {
    /**
     * IEEE Organizationally Unique Identifier of GPS/INS manufacturer.
     *
     * \note Only the 24 least significant bits are used.
     */
    uint32_t oui;
    /** Number of 32-bit words used for the GPS ASCII. */
    uint32_t number_of_words;
    /**
     * GPS ASCII sentences. Padded with null characters so it's a multiple of 4 B. Pointer may be null when
     * not defined.
     *
     * \warning Will point into the buffer when the field is read. Deallocating the buffer will render the pointer
     *          invalid.
     */
    const char* ascii;
};

/**
 * Indicator fields for Context.
 */
struct vrt_context_association_lists_indicators {
    /** True if asynchronous_channel_tag_list is included. */
    bool asynchronous_channel_tag_list;
};

/**
 * Contains association lists.
 */
struct vrt_context_association_lists {
    /**
     * Number of Stream IDs in source context association list.
     *
     * \note Only the 9 least significant bits are used.
     */
    uint16_t source_list_size;
    /**
     * Number of Stream IDs in system context association list.
     *
     * \note Only the 9 least significant bits are used.
     */
    uint16_t system_list_size;
    /** Number of Stream IDs in vector-component association list. */
    uint16_t vector_component_list_size;
    /** Field presence indicators. */
    struct vrt_context_association_lists_indicators has;
    /**
     * Number of Stream IDs in asynchronous-channel context association list.
     *
     * \note Only the 15 least significant bits are used.
     * \note Must have as many entries as sample vector dimension.
     */
    uint16_t asynchronous_channel_list_size;
    /**
     * Producer metadata.
     *
     * \warning Will point into the buffer when the field is read. Deallocating the buffer will render the pointer
     *          invalid.
     */
    const uint32_t* source_context_association_list;
    /**
     * Additional metadata not covered by other context association lists.
     *
     * \warning Will point into the buffer when the field is read. Deallocating the buffer will render the pointer
     *          invalid.
     */
    const uint32_t* system_context_association_list;
    /**
     * Holds metadata for data packet stream sample vectors.
     *
     * \warning Will point into the buffer when the field is read. Deallocating the buffer will render the pointer
     *          invalid.
     */
    const uint32_t* vector_component_context_association_list;
    /**
     * Associates a stream with each channel when channel tags are used in a data packet stream.
     *
     * \warning Will point into the buffer when the field is read. Deallocating the buffer will render the pointer
     *          invalid.
     */
    const uint32_t* asynchronous_channel_context_association_list;
    /**
     * All channel tags used in paired data packet stream.
     *
     * \warning Will point into the buffer when the field is read. Deallocating the buffer will render the pointer
     *          invalid.
     */
    const uint32_t* asynchronous_channel_tag_list;
};

/**
 * VITA 49 standard version that a packet stream complies with (CIF1 V49 spec compliance field, VITA 49.2).
 *
 * \note Remaining values are reserved.
 */
enum vrt_v49_spec_compliance {
    VRT_V49_0 = 0x1, /**< ANSI/VITA 49.0. */
    VRT_V49_1 = 0x2, /**< ANSI/VITA 49.1. */
    VRT_V49_A = 0x3, /**< ANSI/VITA 49A. */
    VRT_V49_2 = 0x4  /**< ANSI/VITA 49.2. */
};

/**
 * Version and build code (CIF1, VITA 49.2).
 */
struct vrt_version_build_code {
    /**
     * Year code.
     *
     * \note Only the 7 least significant bits are used.
     */
    uint8_t year;
    /**
     * Day of year.
     *
     * \note Only the 9 least significant bits are used.
     */
    uint16_t day;
    /**
     * Revision.
     *
     * \note Only the 6 least significant bits are used.
     */
    uint8_t revision;
    /**
     * User defined.
     *
     * \note Only the 10 least significant bits are used.
     */
    uint16_t user_defined;
};

/**
 * Context section data.
 *
 * \note Also used for the control fields of control and query-state acknowledge packets (VITA 49.2), which encode
 *       them the same way, and for the indicator bits of cancellation packets, which have no fields.
 */
struct vrt_if_context {
    /**
     * True if this packet contains new context information.
     *
     * \note In the standard this is sometimes called 'indicator' and sometimes 'identifier'.
     * \note Since over-range count isn't a persistent field, and individual user-defined fields may not be persistent,
     *       do not mark a packet as changed when only those field changes.
     */
    bool context_field_change_indicator;
    /** Field presence indicators. */
    struct vrt_context_indicators has;
    /** Stream ID of for which the context packet applies. */
    uint32_t reference_point_identifier;
    /**
     * Bandwidth [Hz].
     *
     * \note Negative values are not valid.
     * \warning This may lead to loss of precision, since the underlying VRT fixed point format has 64 bits while a
     *          double only has 53 bits of precision.
     */
    double bandwidth;
    /**
     * Center frequency [Hz].
     *
     * \warning This may lead to loss of precision, since the underlying VRT fixed point format has 64 bits while a
     *          double only has 53 bits of precision.
     */
    double if_reference_frequency;
    /**
     * Original center frequency before translation [Hz].
     *
     * \warning This may lead to loss of precision, since the underlying VRT fixed point format has 64 bits while a
     *          double only has 53 bits of precision.
     */
    double rf_reference_frequency;
    /**
     * Original frequency is the sum of this and rf_reference_frequency [Hz].
     *
     * \warning This may lead to loss of precision, since the underlying VRT fixed point format has 64 bits while a
     *          double only has 53 bits of precision.
     */
    double rf_reference_frequency_offset;
    /**
     * Offset from IF reference frequency to center of the band [Hz]. Band center is the sum of this and
     * if_reference_frequency.
     *
     * \warning This may lead to loss of precision, since the underlying VRT fixed point format has 64 bits while a
     *          double only has 53 bits of precision.
     */
    double if_band_offset;
    /** Physical signal amplitude at the Reference Point [dBm]. */
    float reference_level;
    /** Gain. */
    struct vrt_gain gain;
    /**
     * Number of over-range samples, i.e. samples with too high amplitude.
     */
    uint32_t over_range_count;
    /**
     * Sample rate [Hz].
     *
     * \warning This may lead to loss of precision, since the underlying VRT fixed point format has 64 bits while a
     *          double only has 53 bits of precision.
     */
    double sample_rate;
    /** System signal processing time [ps]. Offset from reference point time. */
    uint64_t timestamp_adjustment;
    /**
     * Time when timestamp was known to be correct, i.e. calibrated [s]. Represented in the same way as the TSI field.
     */
    uint32_t timestamp_calibration_time;
    /** Temperature of some component in the signal chain that may affect the signal. */
    float temperature;
    /** Identifies device carrying a context packet stream. */
    struct vrt_device_identifier device_identifier;
    /** Conveys state information. */
    struct vrt_state_and_event state_and_event_indicators;
    /** Describes formatting of data stream in data packets. */
    struct vrt_data_packet_payload_format data_packet_payload_format;
    /** GPS information. */
    struct vrt_formatted_geolocation formatted_gps_geolocation;
    /** INS information. */
    struct vrt_formatted_geolocation formatted_ins_geolocation;
    /** ECEF (earth-centered, earth-fixed) coordinates. */
    struct vrt_ephemeris ecef_ephemeris;
    /** Coordinates with user defined point of reference. */
    struct vrt_ephemeris relative_ephemeris;
    /** Stream ID of relative ephemeris origin. */
    uint32_t ephemeris_reference_identifier;
    /** GPS ASCII positional information. */
    struct vrt_gps_ascii gps_ascii;
    /**
     * Describes which data packets are associated to this context packet.
     *
     * \note In the standard this is sometimes called 'lists' and sometimes 'list'.
     */
    struct vrt_context_association_lists context_association_lists;
    /** Health status (CIF1). Raw 32-bit word. */
    uint32_t health_status;
    /** V49 spec compliance (CIF1). */
    enum vrt_v49_spec_compliance v49_spec_compliance;
    /** Version and build code (CIF1). */
    struct vrt_version_build_code version_build_code;
};

/**
 * Format of a controllee or controller identifier (VITA 49.2).
 */
enum vrt_identifier_format {
    VRT_IDF_WORD = 0x0, /**< 32-bit identifier. */
    VRT_IDF_UUID = 0x1  /**< 128-bit UUID. */
};

/**
 * Control action mode (VITA 49.2).
 *
 * \note Value 3 is reserved.
 */
enum vrt_action_mode {
    VRT_AM_NO_ACTION = 0x0, /**< Take no action. */
    VRT_AM_DRY_RUN   = 0x1, /**< Do not apply controls, but acknowledge as if they were applied. */
    VRT_AM_EXECUTE   = 0x2  /**< Apply controls. */
};

/**
 * Timestamp control mode (VITA 49.2).
 *
 * \note Values 5 and 6 are reserved.
 */
enum vrt_timing_control {
    VRT_TC_IGNORE        = 0x0, /**< Execute with no timestamp constraints. */
    VRT_TC_DEVICE        = 0x1, /**< Execute within the device timing precision window. */
    VRT_TC_LATE          = 0x2, /**< Execute on time, or late within the application timing window. */
    VRT_TC_EARLY         = 0x3, /**< Execute on time, or early within the application timing window. */
    VRT_TC_EARLY_LATE    = 0x4, /**< Execute within the application timing window. */
    VRT_TC_TIMING_ISSUES = 0x7  /**< Some controls were not executed at the specified time. Acknowledge only. */
};

/**
 * Control/Acknowledge mode (CAM) word of command packets (VITA 49.2).
 *
 * \note In control packets, validation, execution, query_state, warnings and errors request acknowledge packets. In
 *       acknowledge packets they state the kind of acknowledge, and whether warning and error indicator words follow.
 */
struct vrt_cam {
    bool                       controllee_enable;     /**< True if the Controllee identifier is included. */
    enum vrt_identifier_format controllee_format;     /**< Format of the Controllee identifier. */
    bool                       controller_enable;     /**< True if the Controller identifier is included. */
    enum vrt_identifier_format controller_format;     /**< Format of the Controller identifier. */
    bool                       permit_partial;        /**< True if partial execution is permitted. */
    bool                       permit_warnings;       /**< True if execution is permitted despite warnings. */
    bool                       permit_errors;         /**< True if execution is permitted despite errors. */
    enum vrt_action_mode       action_mode;           /**< Action mode. */
    bool                       nack_only;             /**< True if acknowledges are only sent on warnings or errors. */
    bool                       validation;            /**< Validation acknowledge (V). */
    bool                       execution;             /**< Execution acknowledge (X). */
    bool                       query_state;           /**< Query-state acknowledge (S). */
    bool                       warnings;              /**< Warnings requested or included (W). */
    bool                       errors;                /**< Errors requested or included (Er). */
    enum vrt_timing_control    timing_control;        /**< Timestamp control mode. */
    bool                       partial_action;        /**< True if only part of the action was taken. Ack only. */
    bool                       scheduled_or_executed; /**< True if the action was scheduled or executed. Ack only. */
};

/**
 * Bit masks for the warning and error response words of acknowledge packets (VITA 49.2). Bits 12 to 1 are user
 * defined.
 */
static const uint32_t VRT_WEF_FIELD_NOT_EXECUTED              = 0x80000000U; /**< Field not executed. */
static const uint32_t VRT_WEF_DEVICE_FAILURE                  = 0x40000000U; /**< Device failure. */
static const uint32_t VRT_WEF_ERRONEOUS_FIELD                 = 0x20000000U; /**< Field not accepted by device. */
static const uint32_t VRT_WEF_PARAMETER_OUT_OF_RANGE          = 0x10000000U; /**< Parameter out of range. */
static const uint32_t VRT_WEF_PARAMETER_UNSUPPORTED_PRECISION = 0x08000000U; /**< Unsupported precision. */
static const uint32_t VRT_WEF_FIELD_VALUE_INVALID             = 0x04000000U; /**< Field value invalid. */
static const uint32_t VRT_WEF_TIMESTAMP_PROBLEM               = 0x02000000U; /**< Timestamp problem. */
static const uint32_t VRT_WEF_HAZARDOUS_POWER_LEVELS          = 0x01000000U; /**< Hazardous power levels. */
static const uint32_t VRT_WEF_DISTORTION                      = 0x00800000U; /**< Distortion. */
static const uint32_t VRT_WEF_IN_BAND_POWER_COMPLIANCE        = 0x00400000U; /**< In-band power compliance. */
static const uint32_t VRT_WEF_OUT_OF_BAND_POWER_COMPLIANCE    = 0x00200000U; /**< Out-of-band power compliance. */
static const uint32_t VRT_WEF_COSITE_INTERFERENCE             = 0x00100000U; /**< Co-site interference. */
static const uint32_t VRT_WEF_REGIONAL_INTERFERENCE           = 0x00080000U; /**< Regional interference. */

/**
 * Warning or error indicator words (WIF0/WIF1 or EIF0/EIF1) and their response words, one 32-bit word per indicated
 * field regardless of the size of the field itself, in acknowledge packets (VITA 49.2). Response words are bitwise ORs
 * of VRT_WEF_* masks.
 */
struct vrt_ack_responses {
    /** Fields with a response word. Same bit layout as CIF0 and CIF1. */
    struct vrt_context_indicators has;
    uint32_t                      reference_point_identifier;     /**< Response for Reference point identifier. */
    uint32_t                      bandwidth;                      /**< Response for Bandwidth. */
    uint32_t                      if_reference_frequency;         /**< Response for IF reference frequency. */
    uint32_t                      rf_reference_frequency;         /**< Response for RF reference frequency. */
    uint32_t                      rf_reference_frequency_offset;  /**< Response for RF reference frequency offset. */
    uint32_t                      if_band_offset;                 /**< Response for IF band offset. */
    uint32_t                      reference_level;                /**< Response for Reference level. */
    uint32_t                      gain;                           /**< Response for Gain. */
    uint32_t                      over_range_count;               /**< Response for Over-range count. */
    uint32_t                      sample_rate;                    /**< Response for Sample rate. */
    uint32_t                      timestamp_adjustment;           /**< Response for Timestamp adjustment. */
    uint32_t                      timestamp_calibration_time;     /**< Response for Timestamp calibration time. */
    uint32_t                      temperature;                    /**< Response for Temperature. */
    uint32_t                      device_identifier;              /**< Response for Device identifier. */
    uint32_t                      state_and_event_indicators;     /**< Response for State and event indicators. */
    uint32_t                      data_packet_payload_format;     /**< Response for Data packet payload format. */
    uint32_t                      formatted_gps_geolocation;      /**< Response for Formatted GPS geolocation. */
    uint32_t                      formatted_ins_geolocation;      /**< Response for Formatted INS geolocation. */
    uint32_t                      ecef_ephemeris;                 /**< Response for ECEF ephemeris. */
    uint32_t                      relative_ephemeris;             /**< Response for Relative ephemeris. */
    uint32_t                      ephemeris_reference_identifier; /**< Response for Ephemeris reference identifier. */
    uint32_t                      gps_ascii;                      /**< Response for GPS ASCII. */
    uint32_t                      context_association_lists;      /**< Response for Context association lists. */
    uint32_t                      health_status;                  /**< Response for Health status (CIF1). */
    uint32_t                      v49_spec_compliance;            /**< Response for V49 spec compliance (CIF1). */
    uint32_t                      version_build_code;             /**< Response for Version and build code (CIF1). */
};

/**
 * Command section of command and Ext command packets (VITA 49.2): CAM, message ID and identifiers, directly following
 * the fields section. For validation and execution acknowledge packets it also holds the warning and error responses.
 *
 * \note The control fields of control and query-state acknowledge packets, and the indicator bits of cancellation
 *       packets, are in vrt_packet.if_context. The custom section of Ext command packets is vrt_packet.body.
 */
struct vrt_command {
    /** Control/Acknowledge mode. */
    struct vrt_cam cam;
    /** Message ID. Pairs acknowledges with controls. */
    uint32_t message_id;
    /** Controllee identifier. Activate by setting cam.controllee_enable and cam.controllee_format = VRT_IDF_WORD. */
    uint32_t controllee_id;
    /**
     * Controllee UUID. Activate by setting cam.controllee_enable and cam.controllee_format = VRT_IDF_UUID.
     *
     * \note Word 0 is most significant, i.e. the first 4 bytes of the UUID once the packet is in big endian.
     */
    uint32_t controllee_uuid[4];
    /** Controller identifier. Activate by setting cam.controller_enable and cam.controller_format = VRT_IDF_WORD. */
    uint32_t controller_id;
    /**
     * Controller UUID. Activate by setting cam.controller_enable and cam.controller_format = VRT_IDF_UUID.
     *
     * \note Word 0 is most significant, i.e. the first 4 bytes of the UUID once the packet is in big endian.
     */
    uint32_t controller_uuid[4];
    /** Warnings (WIF). Only in validation and execution acknowledge packets with cam.warnings set. */
    struct vrt_ack_responses warnings;
    /** Errors (EIF). Only in validation and execution acknowledge packets with cam.errors set. */
    struct vrt_ack_responses errors;
};

/**
 * VRT packet data.
 */
struct vrt_packet {
    struct vrt_header header; /**< Header. */
    struct vrt_fields fields; /**< Fields. */
    /** Data payload for data packets, or custom section for Ext context and Ext command packets. */
    void*              body;
    int32_t            words_body; /**< Number of 32-bit words used for body. */
    struct vrt_trailer trailer;    /**< Trailer. */
    /**
     * IF context. Also the control fields of control and query-state acknowledge packets, and the indicator bits of
     * cancellation packets.
     */
    struct vrt_if_context if_context;
    /**
     * Command section of command and Ext command packets. Must point to valid storage when reading or writing such
     * packets, and may be NULL otherwise.
     *
     * \note A pointer, so that packets without a command section do not pay for its size.
     */
    struct vrt_command* command;
};

/**
 * Timestamp in whole and fractional seconds.
 *
 * \note ps is always nonnegative. So -3.5 is represented as s = -4, ps = 5e11.
 */
struct vrt_time {
    int32_t  s;  /**< Seconds. */
    uint64_t ps; /**< Picoseconds. Always nonnegative. */
};

/**
 * Human comprehensible calendar date/time format. Similar to C standard library struct tm, but with an extra field for
 * picoseconds.
 */
struct vrt_calendar_time {
    int32_t  year; /**< Number of years since 1900. */
    int32_t  mday; /**< Day of month [1, 31]. */
    int32_t  yday; /**< Day in year [0, 365]. */
    int32_t  sec;  /**< Seconds [0, 59]. */
    int32_t  min;  /**< Minutes [0, 59]. */
    int32_t  hour; /**< Hours [0, 23]. */
    int32_t  mon;  /**< Month [0, 11]. */
    int32_t  wday; /**< Day of week, starting with Sunday [0, 6]. */
    uint64_t ps;   /**< Picoseconds [0, 999999999999]. */
};

#ifdef __cplusplus
}
#endif

#endif
