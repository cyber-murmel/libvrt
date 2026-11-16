#include <gtest/gtest.h>

#include <array>
#include <cstdint>

#include <vrt/vrt_error_code.h>
#include <vrt/vrt_init.h>
#include <vrt/vrt_read.h>
#include <vrt/vrt_types.h>
#include <vrt/vrt_words.h>
#include <vrt/vrt_write.h>

#include "hex.h"
#include "init_garbage.h"

/*
 * Command packets (VITA 49.2 section 8): header, fields (stream ID always present), CAM, message ID, optional
 * controllee and controller identifiers, then CIF words and fields (control, query-state acknowledge), CIF words only
 * (cancellation), or warning and error words (validation and execution acknowledge). Same in vita49-rs
 * (src/command.rs, src/command_payload.rs, src/cancellation.rs, src/query_ack.rs, src/ack.rs) and SoapyVITA49
 * (src/Control.cpp: Session::setField, Session::query, parseAck).
 */

class CommandPacketTest : public ::testing::Test {
   protected:
    void SetUp() override {
        vrt_init_packet(&w_);
        vrt_init_command(&wc_);
        w_.command = &wc_;
        init_garbage_packet(&r_);
        init_garbage_command(&rc_);
        r_.command = &rc_;
        buf_.fill(0xBAADF00D);
    }

    /** Write w_ with validation and compare the words with expected. Then read it back into r_. */
    template <size_t N>
    void expect_words(const std::array<uint32_t, N>& expected) {
        ASSERT_EQ(vrt_words_packet(&w_), static_cast<int32_t>(N));
        ASSERT_EQ(vrt_write_packet(&w_, buf_.data(), static_cast<int32_t>(N) - 1, true), VRT_ERR_BUFFER_SIZE);
        ASSERT_EQ(vrt_write_packet(&w_, buf_.data(), static_cast<int32_t>(buf_.size()), true), static_cast<int32_t>(N));
        for (size_t i = 0; i < N; ++i) {
            ASSERT_EQ(Hex(buf_[i]), Hex(expected[i])) << i;
        }
        ASSERT_EQ(Hex(buf_[N]), Hex(0xBAADF00D));
        ASSERT_EQ(vrt_read_packet(buf_.data(), static_cast<int32_t>(N) - 1, &r_, true), VRT_ERR_BUFFER_SIZE);
        ASSERT_EQ(vrt_read_packet(buf_.data(), static_cast<int32_t>(N), &r_, true), static_cast<int32_t>(N));
    }

    vrt_packet               w_{};
    vrt_packet               r_{};
    vrt_command              wc_{};
    vrt_command              rc_{};
    std::array<uint32_t, 64> buf_{};
};

/* vita49-rs tests/command_packet.json5, a control packet setting bandwidth, RF frequency and sample rate */
static const std::array<uint32_t, 19> VITA49_RS_COMMAND{0x62600013, 0x00000001, 0x67AE38BD, 0x00000023, 0x33109C70,
                                                        0xE10B0000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
                                                        0x00000000, 0x00000004, 0xA8200000, 0x00005F5E, 0x10000000,
                                                        0x00035A4E, 0x90000000, 0x00005F5E, 0x10000000};

/* vita49-rs tests/ack_packet.json5, an execution acknowledge packet */
static const std::array<uint32_t, 12> VITA49_RS_ACK{0x6460000C, 0x00000001, 0x00000000, 0x00000000,
                                                    0x00000000, 0xBC080000, 0x00000000, 0x00000000,
                                                    0x00000000, 0x00000000, 0x00000000, 0x00000000};

TEST_F(CommandPacketTest, ReadVita49RsCommand) {
    std::array<uint32_t, 19> b = VITA49_RS_COMMAND;
    ASSERT_EQ(vrt_read_packet(b.data(), static_cast<int32_t>(b.size()), &r_, true), 19);
    ASSERT_EQ(r_.header.packet_type, VRT_PT_COMMAND);
    ASSERT_TRUE(r_.header.nd0); /* Reserved bit set by the source of this vector */
    ASSERT_FALSE(r_.header.acknowledge);
    ASSERT_FALSE(r_.header.cancellation);
    ASSERT_EQ(r_.header.tsi, VRT_TSI_UTC);
    ASSERT_EQ(r_.header.tsf, VRT_TSF_REAL_TIME);
    ASSERT_EQ(r_.fields.stream_id, 1U);
    ASSERT_EQ(r_.fields.integer_seconds_timestamp, 1739471037U);
    ASSERT_EQ(r_.fields.fractional_seconds_timestamp, 151180582000U);
    ASSERT_TRUE(rc_.cam.controllee_enable);
    ASSERT_EQ(rc_.cam.controllee_format, VRT_IDF_UUID);
    ASSERT_TRUE(rc_.cam.controller_enable);
    ASSERT_EQ(rc_.cam.controller_format, VRT_IDF_WORD);
    ASSERT_EQ(rc_.cam.action_mode, VRT_AM_EXECUTE);
    ASSERT_FALSE(rc_.cam.validation);
    ASSERT_TRUE(rc_.cam.execution);
    ASSERT_FALSE(rc_.cam.query_state);
    ASSERT_TRUE(rc_.cam.warnings);
    ASSERT_TRUE(rc_.cam.errors);
    ASSERT_EQ(rc_.cam.timing_control, VRT_TC_IGNORE);
    ASSERT_EQ(rc_.message_id, 0U);
    ASSERT_EQ(rc_.controllee_uuid[0], 0U);
    ASSERT_EQ(rc_.controller_id, 4U);
    ASSERT_TRUE(r_.if_context.context_field_change_indicator);
    ASSERT_TRUE(r_.if_context.has.bandwidth);
    ASSERT_TRUE(r_.if_context.has.rf_reference_frequency);
    ASSERT_TRUE(r_.if_context.has.sample_rate);
    ASSERT_FALSE(r_.if_context.has.gain);
    ASSERT_DOUBLE_EQ(r_.if_context.bandwidth, 100e6);
    ASSERT_DOUBLE_EQ(r_.if_context.rf_reference_frequency, 900e6);
    ASSERT_DOUBLE_EQ(r_.if_context.sample_rate, 100e6);
    ASSERT_EQ(r_.words_body, 0);
    ASSERT_EQ(r_.body, nullptr);

    /* Bit exact write back. Bit 25 is reserved in command packets, so validation rejects it. */
    ASSERT_EQ(vrt_write_packet(&r_, buf_.data(), static_cast<int32_t>(buf_.size()), true),
              VRT_ERR_INDICATOR_IN_COMMAND);
    ASSERT_EQ(vrt_write_packet(&r_, buf_.data(), static_cast<int32_t>(buf_.size()), false), 19);
    for (size_t i = 0; i < b.size(); ++i) {
        ASSERT_EQ(Hex(buf_[i]), Hex(b[i])) << i;
    }
    r_.header.nd0 = false;
    ASSERT_EQ(vrt_write_packet(&r_, buf_.data(), static_cast<int32_t>(buf_.size()), true), 19);
    ASSERT_EQ(Hex(buf_[0]), Hex(0x60600013));
}

TEST_F(CommandPacketTest, ReadVita49RsAck) {
    std::array<uint32_t, 12> b = VITA49_RS_ACK;
    ASSERT_EQ(vrt_read_packet(b.data(), static_cast<int32_t>(b.size()), &r_, true), 12);
    ASSERT_EQ(r_.header.packet_type, VRT_PT_COMMAND);
    ASSERT_TRUE(r_.header.acknowledge);
    ASSERT_FALSE(r_.header.nd0);
    ASSERT_TRUE(rc_.cam.controllee_enable);
    ASSERT_EQ(rc_.cam.controllee_format, VRT_IDF_WORD);
    ASSERT_TRUE(rc_.cam.controller_enable);
    ASSERT_EQ(rc_.cam.controller_format, VRT_IDF_UUID);
    ASSERT_TRUE(rc_.cam.permit_partial);
    ASSERT_TRUE(rc_.cam.permit_warnings);
    ASSERT_FALSE(rc_.cam.permit_errors);
    ASSERT_TRUE(rc_.cam.execution);
    ASSERT_FALSE(rc_.cam.warnings);
    ASSERT_FALSE(rc_.cam.errors);

    ASSERT_EQ(vrt_write_packet(&r_, buf_.data(), static_cast<int32_t>(buf_.size()), true), 12);
    for (size_t i = 0; i < b.size(); ++i) {
        ASSERT_EQ(Hex(buf_[i]), Hex(b[i])) << i;
    }
}

TEST_F(CommandPacketTest, SoapyVita49SetField) {
    /*
     * SoapyVITA49 src/Control.cpp Session::setField for RF frequency with acknowledge requested: CAM from
     * buildCam(false) = CAM_CONTROLLEE_ENABLE | ACTION_EXECUTE << 23 | CAM_PERMIT_WARNINGS | CAM_PERMIT_ERRORS |
     * CAM_REQ_EXECUTION_ACK | CAM_REQ_WARNINGS | CAM_REQ_ERRORS, and the stream ID as controllee identifier.
     */
    w_.header.packet_type                    = VRT_PT_COMMAND;
    w_.fields.stream_id                      = 0x00000001;
    wc_.cam.controllee_enable                = true;
    wc_.cam.action_mode                      = VRT_AM_EXECUTE;
    wc_.cam.permit_warnings                  = true;
    wc_.cam.permit_errors                    = true;
    wc_.cam.execution                        = true;
    wc_.cam.warnings                         = true;
    wc_.cam.errors                           = true;
    wc_.message_id                           = 1;
    wc_.controllee_id                        = 0x00000001;
    w_.if_context.has.rf_reference_frequency = true;
    w_.if_context.rf_reference_frequency     = 2412e6;
    expect_words(std::array<uint32_t, 8>{0x60000008, 0x00000001, 0x870B0000, 0x00000001, 0x00000001, 0x08000000,
                                         0x0008FC43, 0x30000000});
    ASSERT_DOUBLE_EQ(r_.if_context.rf_reference_frequency, 2412e6);
    ASSERT_EQ(rc_.cam.action_mode, VRT_AM_EXECUTE);
}

TEST_F(CommandPacketTest, SoapyVita49Query) {
    /*
     * SoapyVITA49 src/Control.cpp Session::query: CAM from buildCam(true) = CAM_CONTROLLEE_ENABLE |
     * CAM_REQ_QUERY_STATE, and CIF0 bits with zeroed fields.
     */
    w_.header.packet_type                    = VRT_PT_COMMAND;
    w_.header.packet_count                   = 3;
    w_.fields.stream_id                      = 0x00000007;
    wc_.cam.controllee_enable                = true;
    wc_.cam.query_state                      = true;
    wc_.message_id                           = 2;
    wc_.controllee_id                        = 0x00000007;
    w_.if_context.has.bandwidth              = true;
    w_.if_context.has.rf_reference_frequency = true;
    w_.if_context.has.gain                   = true;
    w_.if_context.has.sample_rate            = true;
    expect_words(std::array<uint32_t, 13>{0x6003000D, 0x00000007, 0x80040000, 0x00000002, 0x00000007, 0x28A00000,
                                          0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
                                          0x00000000});
}

TEST_F(CommandPacketTest, QueryStateAckRoundTrip) {
    w_.header.packet_type                    = VRT_PT_COMMAND;
    w_.header.acknowledge                    = true;
    w_.fields.stream_id                      = 0x5D5D0001;
    wc_.cam.controllee_enable                = true;
    wc_.cam.query_state                      = true;
    wc_.message_id                           = 2;
    wc_.controllee_id                        = 0x5D5D0001;
    w_.if_context.has.rf_reference_frequency = true;
    w_.if_context.rf_reference_frequency     = 2437e6;
    w_.if_context.has.v49_spec_compliance    = true;
    w_.if_context.v49_spec_compliance        = VRT_V49_2;
    expect_words(std::array<uint32_t, 10>{0x6400000A, 0x5D5D0001, 0x80040000, 0x00000002, 0x5D5D0001, 0x08000002,
                                          0x00000008, 0x0009141A, 0xB4000000, 0x00000004});
    ASSERT_TRUE(r_.header.acknowledge);
    ASSERT_TRUE(rc_.cam.query_state);
    ASSERT_DOUBLE_EQ(r_.if_context.rf_reference_frequency, 2437e6);
    ASSERT_TRUE(r_.if_context.has.v49_spec_compliance);
}

TEST_F(CommandPacketTest, ExecutionAckWithWarningsRoundTrip) {
    w_.header.packet_type         = VRT_PT_COMMAND;
    w_.header.acknowledge         = true;
    w_.fields.stream_id           = 1;
    wc_.cam.execution             = true;
    wc_.cam.warnings              = true;
    wc_.cam.scheduled_or_executed = true;
    wc_.message_id                = 7;
    wc_.warnings.has.sample_rate  = true;
    wc_.warnings.sample_rate      = VRT_WEF_PARAMETER_OUT_OF_RANGE;
    /* Ignored: execution acknowledges have no context fields */
    w_.if_context.has.bandwidth = true;
    expect_words(std::array<uint32_t, 6>{0x64000006, 0x00000001, 0x000A0400, 0x00000007, 0x00200000, 0x10000000});
    ASSERT_TRUE(rc_.warnings.has.sample_rate);
    ASSERT_EQ(Hex(rc_.warnings.sample_rate), Hex(VRT_WEF_PARAMETER_OUT_OF_RANGE));
    ASSERT_FALSE(rc_.errors.has.sample_rate);
}

TEST_F(CommandPacketTest, CancellationRoundTrip) {
    /* Cancellation packets have indicator words only (vita49-rs src/cancellation.rs) */
    w_.header.packet_type                = VRT_PT_COMMAND;
    w_.header.cancellation               = true;
    w_.fields.stream_id                  = 1;
    wc_.message_id                       = 9;
    w_.if_context.has.sample_rate        = true;
    w_.if_context.sample_rate            = 1e6;
    w_.if_context.has.version_build_code = true;
    expect_words(std::array<uint32_t, 6>{0x61000006, 0x00000001, 0x00000000, 0x00000009, 0x00200002, 0x00000004});
    ASSERT_TRUE(r_.header.cancellation);
    ASSERT_TRUE(r_.if_context.has.sample_rate);
    ASSERT_TRUE(r_.if_context.has.version_build_code);
    ASSERT_FALSE(r_.if_context.has.bandwidth);
    ASSERT_DOUBLE_EQ(r_.if_context.sample_rate, 0.0);
}

TEST_F(CommandPacketTest, ExtCommandBodyRoundTrip) {
    uint32_t body[]           = {0x01234567, 0x89ABCDEF};
    w_.header.packet_type     = VRT_PT_EXT_COMMAND;
    w_.header.has.class_id    = true;
    w_.fields.stream_id       = 2;
    w_.fields.class_id.oui    = 0x00123456;
    wc_.cam.controller_enable = true;
    wc_.controller_id         = 0xC0FFEE;
    wc_.message_id            = 3;
    w_.body                   = body;
    w_.words_body             = 2;
    expect_words(std::array<uint32_t, 9>{0x78000009, 0x00000002, 0x00123456, 0x00000000, 0x20000000, 0x00000003,
                                         0x00C0FFEE, 0x01234567, 0x89ABCDEF});
    ASSERT_EQ(r_.words_body, 2);
    ASSERT_EQ(r_.body, buf_.data() + 7);
    ASSERT_EQ(rc_.controller_id, 0xC0FFEEU);
}

TEST_F(CommandPacketTest, MissingCommand) {
    w_.header.packet_type = VRT_PT_COMMAND;
    w_.command            = nullptr;
    ASSERT_EQ(vrt_write_packet(&w_, buf_.data(), static_cast<int32_t>(buf_.size()), true), VRT_ERR_MISSING_COMMAND);
    ASSERT_EQ(vrt_write_packet(&w_, buf_.data(), static_cast<int32_t>(buf_.size()), false), VRT_ERR_MISSING_COMMAND);
    std::array<uint32_t, 12> b = VITA49_RS_ACK;
    r_.command                 = nullptr;
    ASSERT_EQ(vrt_read_packet(b.data(), static_cast<int32_t>(b.size()), &r_, false), VRT_ERR_MISSING_COMMAND);
}

TEST_F(CommandPacketTest, PacketSizeMismatch) {
    std::array<uint32_t, 13> b{};
    std::copy(VITA49_RS_ACK.begin(), VITA49_RS_ACK.end(), b.begin());
    b[0] = 0x6460000D;
    ASSERT_EQ(vrt_read_packet(b.data(), static_cast<int32_t>(b.size()), &r_, true), VRT_ERR_MISMATCH_PACKET_SIZE);
    ASSERT_EQ(vrt_read_packet(b.data(), static_cast<int32_t>(b.size()), &r_, false), 12);
}

TEST_F(CommandPacketTest, NoTrailerInCommand) {
    /* has.trailer is ignored without validation, since bit 26 is the acknowledge indicator */
    w_.header.packet_type = VRT_PT_COMMAND;
    w_.header.has.trailer = true;
    wc_.cam.action_mode   = VRT_AM_DRY_RUN;
    ASSERT_EQ(vrt_words_packet(&w_), 5);
    ASSERT_EQ(vrt_write_packet(&w_, buf_.data(), static_cast<int32_t>(buf_.size()), true),
              VRT_ERR_INDICATOR_IN_COMMAND);
    ASSERT_EQ(vrt_write_packet(&w_, buf_.data(), static_cast<int32_t>(buf_.size()), false), 5);
    ASSERT_EQ(Hex(buf_[0]), Hex(0x64000005));
}

TEST(SignalDataPacket492Test, Nd0SpectrumRoundTrip) {
    uint32_t   body[] = {0x00010002, 0x00030004, 0x00050006};
    vrt_packet p{};
    vrt_init_packet(&p);
    p.header.packet_type      = VRT_PT_IF_DATA_WITH_STREAM_ID;
    p.header.has.trailer      = true;
    p.header.nd0              = true;
    p.header.spectrum_or_time = VRT_SOT_SPECTRUM;
    p.header.packet_count     = 5;
    p.fields.stream_id        = 0xABCD0001;
    p.body                    = body;
    p.words_body              = 3;
    p.trailer.has.valid_data  = true;
    p.trailer.valid_data      = true;

    std::array<uint32_t, 8> buf{};
    ASSERT_EQ(vrt_words_packet(&p), 6);
    ASSERT_EQ(vrt_write_packet(&p, buf.data(), static_cast<int32_t>(buf.size()), true), 6);
    ASSERT_EQ(Hex(buf[0]), Hex(0x17050006));
    ASSERT_EQ(Hex(buf[5]), Hex(0x40040000));

    vrt_packet r{};
    init_garbage_packet(&r);
    ASSERT_EQ(vrt_read_packet(buf.data(), 6, &r, true), 6);
    ASSERT_TRUE(r.header.nd0);
    ASSERT_EQ(r.header.spectrum_or_time, VRT_SOT_SPECTRUM);
    ASSERT_EQ(r.header.tsm, VRT_TSM_FINE);
    ASSERT_TRUE(r.header.has.trailer);
    ASSERT_EQ(r.words_body, 3);
    ASSERT_TRUE(r.trailer.valid_data);
}
