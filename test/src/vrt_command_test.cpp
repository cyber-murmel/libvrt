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
 * Control/Acknowledge mode (CAM) word (VITA 49.2 section 8.3.1). Bit positions agree in vrtgen
 * (include/vrtgen/packing/command.hpp, where positions are the msb of a field), vita49-rs (src/control_ack_mode.rs) and
 * SoapyVITA49 (src/Control.hpp, CAM_* constants):
 *   31 controllee enable, 30 controllee UUID, 29 controller enable, 28 controller UUID, 27 permit partial,
 *   26 permit warnings, 25 permit errors, 24..23 action mode, 22 NACK only, 20 V, 19 X, 18 S, 17 W, 16 Er,
 *   14..12 timing control, 11 partial action (ack), 10 scheduled or executed (ack).
 */

class CommandTest : public ::testing::Test {
   protected:
    void SetUp() override {
        vrt_init_header(&h_);
        h_.packet_type = VRT_PT_COMMAND;
        vrt_init_command(&w_);
        init_garbage_command(&r_);
        buf_.fill(0xBAADF00D);
    }

    /** Write w_ with validation and expect a single CAM word plus message ID. */
    void expect_cam(uint32_t cam) {
        ASSERT_EQ(vrt_words_command(&h_, &w_), 2);
        ASSERT_EQ(vrt_write_command(&h_, &w_, buf_.data(), 2, true), 2);
        ASSERT_EQ(Hex(buf_[0]), Hex(cam));
        ASSERT_EQ(Hex(buf_[1]), Hex(0x00000000));
        ASSERT_EQ(Hex(buf_[2]), Hex(0xBAADF00D));
        ASSERT_EQ(vrt_read_command(&h_, buf_.data(), 2, &r_, true), 2);
    }

    vrt_header               h_{};
    vrt_command              w_{};
    vrt_command              r_{};
    std::array<uint32_t, 64> buf_{};
};

TEST_F(CommandTest, Empty) {
    expect_cam(0x00000000);
    ASSERT_FALSE(r_.cam.controllee_enable);
    ASSERT_EQ(r_.cam.action_mode, VRT_AM_NO_ACTION);
    ASSERT_EQ(r_.cam.timing_control, VRT_TC_IGNORE);
    ASSERT_EQ(r_.message_id, 0U);
    ASSERT_EQ(r_.controllee_id, 0U);
    ASSERT_EQ(r_.controller_uuid[3], 0U);
    ASSERT_FALSE(r_.warnings.has.bandwidth);
    ASSERT_EQ(r_.errors.bandwidth, 0U);
}

TEST_F(CommandTest, PermitPartial) {
    w_.cam.permit_partial = true;
    expect_cam(0x08000000);
    ASSERT_TRUE(r_.cam.permit_partial);
}

TEST_F(CommandTest, PermitWarnings) {
    w_.cam.permit_warnings = true;
    expect_cam(0x04000000);
    ASSERT_TRUE(r_.cam.permit_warnings);
}

TEST_F(CommandTest, PermitErrors) {
    w_.cam.permit_errors = true;
    expect_cam(0x02000000);
    ASSERT_TRUE(r_.cam.permit_errors);
}

TEST_F(CommandTest, ActionDryRun) {
    w_.cam.action_mode = VRT_AM_DRY_RUN;
    expect_cam(0x00800000);
    ASSERT_EQ(r_.cam.action_mode, VRT_AM_DRY_RUN);
}

TEST_F(CommandTest, ActionExecute) {
    w_.cam.action_mode = VRT_AM_EXECUTE;
    expect_cam(0x01000000);
    ASSERT_EQ(r_.cam.action_mode, VRT_AM_EXECUTE);
}

TEST_F(CommandTest, NackOnly) {
    w_.cam.nack_only = true;
    expect_cam(0x00400000);
    ASSERT_TRUE(r_.cam.nack_only);
}

TEST_F(CommandTest, RequestAcknowledges) {
    w_.cam.validation = true;
    expect_cam(0x00100000);
    ASSERT_TRUE(r_.cam.validation);
    w_.cam.validation = false;
    w_.cam.execution  = true;
    expect_cam(0x00080000);
    ASSERT_TRUE(r_.cam.execution);
    w_.cam.execution   = false;
    w_.cam.query_state = true;
    expect_cam(0x00040000);
    ASSERT_TRUE(r_.cam.query_state);
    w_.cam.query_state = false;
    w_.cam.warnings    = true;
    expect_cam(0x00020000);
    ASSERT_TRUE(r_.cam.warnings);
    w_.cam.warnings = false;
    w_.cam.errors   = true;
    expect_cam(0x00010000);
    ASSERT_TRUE(r_.cam.errors);
}

TEST_F(CommandTest, TimingControl) {
    const std::array<std::pair<vrt_timing_control, uint32_t>, 5> values{{{VRT_TC_IGNORE, 0x00000000},
                                                                         {VRT_TC_DEVICE, 0x00001000},
                                                                         {VRT_TC_LATE, 0x00002000},
                                                                         {VRT_TC_EARLY, 0x00003000},
                                                                         {VRT_TC_EARLY_LATE, 0x00004000}}};
    for (const auto& v : values) {
        w_.cam.timing_control = v.first;
        expect_cam(v.second);
        ASSERT_EQ(r_.cam.timing_control, v.first);
    }
}

TEST_F(CommandTest, AcknowledgeOnlyFields) {
    h_.acknowledge               = true;
    w_.cam.execution             = true;
    w_.cam.partial_action        = true;
    w_.cam.scheduled_or_executed = true;
    w_.cam.timing_control        = VRT_TC_TIMING_ISSUES;
    expect_cam(0x00087C00);
    ASSERT_TRUE(r_.cam.partial_action);
    ASSERT_TRUE(r_.cam.scheduled_or_executed);
    ASSERT_EQ(r_.cam.timing_control, VRT_TC_TIMING_ISSUES);
}

TEST_F(CommandTest, AcknowledgeOnlyFieldsInControl) {
    w_.cam.partial_action = true;
    ASSERT_EQ(vrt_write_command(&h_, &w_, buf_.data(), 2, true), VRT_ERR_ACK_FIELD_IN_CONTROL);
    ASSERT_EQ(vrt_write_command(&h_, &w_, buf_.data(), 2, false), 2);
    ASSERT_EQ(Hex(buf_[0]), Hex(0x00000800));
    ASSERT_EQ(vrt_read_command(&h_, buf_.data(), 2, &r_, true), VRT_ERR_ACK_FIELD_IN_CONTROL);
    ASSERT_EQ(vrt_read_command(&h_, buf_.data(), 2, &r_, false), 2);
    ASSERT_TRUE(r_.cam.partial_action);

    w_.cam.partial_action        = false;
    w_.cam.scheduled_or_executed = true;
    ASSERT_EQ(vrt_write_command(&h_, &w_, buf_.data(), 2, true), VRT_ERR_ACK_FIELD_IN_CONTROL);
    w_.cam.scheduled_or_executed = false;
    w_.cam.timing_control        = VRT_TC_TIMING_ISSUES;
    ASSERT_EQ(vrt_write_command(&h_, &w_, buf_.data(), 2, true), VRT_ERR_ACK_FIELD_IN_CONTROL);
    buf_[0] = 0x00007000;
    ASSERT_EQ(vrt_read_command(&h_, buf_.data(), 2, &r_, true), VRT_ERR_ACK_FIELD_IN_CONTROL);
}

TEST_F(CommandTest, AcknowledgeSubtype) {
    h_.acknowledge = true;
    ASSERT_EQ(vrt_write_command(&h_, &w_, buf_.data(), 2, true), VRT_ERR_ACK_SUBTYPE);
    w_.cam.validation = true;
    w_.cam.execution  = true;
    ASSERT_EQ(vrt_write_command(&h_, &w_, buf_.data(), 2, true), VRT_ERR_ACK_SUBTYPE);
    w_.cam.execution = false;
    ASSERT_EQ(vrt_write_command(&h_, &w_, buf_.data(), 2, true), 2);

    buf_[0] = 0x00000000;
    ASSERT_EQ(vrt_read_command(&h_, buf_.data(), 2, &r_, true), VRT_ERR_ACK_SUBTYPE);
    buf_[0] = 0x001C0000;
    ASSERT_EQ(vrt_read_command(&h_, buf_.data(), 2, &r_, true), VRT_ERR_ACK_SUBTYPE);
    ASSERT_EQ(vrt_read_command(&h_, buf_.data(), 2, &r_, false), 2);
}

TEST_F(CommandTest, InvalidValues) {
    w_.cam.action_mode = static_cast<vrt_action_mode>(3);
    ASSERT_EQ(vrt_write_command(&h_, &w_, buf_.data(), 2, true), VRT_ERR_INVALID_ACTION_MODE);
    ASSERT_EQ(vrt_write_command(&h_, &w_, buf_.data(), 2, false), 2);
    ASSERT_EQ(Hex(buf_[0]), Hex(0x01800000));
    ASSERT_EQ(vrt_read_command(&h_, buf_.data(), 2, &r_, true), VRT_ERR_INVALID_ACTION_MODE);
    vrt_init_command(&w_);

    for (uint32_t tc : {5U, 6U}) {
        w_.cam.timing_control = static_cast<vrt_timing_control>(tc);
        ASSERT_EQ(vrt_write_command(&h_, &w_, buf_.data(), 2, true), VRT_ERR_INVALID_TIMING_CONTROL);
        buf_[0] = tc << 12U;
        ASSERT_EQ(vrt_read_command(&h_, buf_.data(), 2, &r_, true), VRT_ERR_INVALID_TIMING_CONTROL);
    }
    vrt_init_command(&w_);

    w_.cam.controllee_format = static_cast<vrt_identifier_format>(2);
    ASSERT_EQ(vrt_write_command(&h_, &w_, buf_.data(), 2, true), VRT_ERR_INVALID_IDENTIFIER_FORMAT);
    w_.cam.controllee_format = VRT_IDF_WORD;
    w_.cam.controller_format = static_cast<vrt_identifier_format>(-1);
    ASSERT_EQ(vrt_write_command(&h_, &w_, buf_.data(), 2, true), VRT_ERR_INVALID_IDENTIFIER_FORMAT);
}

TEST_F(CommandTest, Reserved) {
    for (uint32_t i = 0; i < 32; ++i) {
        uint32_t bit = 1U << i;
        if ((bit & 0x002083FFU) == 0) {
            continue;
        }
        buf_[0] = bit;
        ASSERT_EQ(vrt_read_command(&h_, buf_.data(), 2, &r_, true), VRT_ERR_RESERVED) << Hex(bit);
        ASSERT_EQ(vrt_read_command(&h_, buf_.data(), 2, &r_, false), 2) << Hex(bit);
    }
}

TEST_F(CommandTest, Identifiers) {
    w_.message_id            = 0x12345678;
    w_.cam.controllee_enable = true;
    w_.controllee_id         = 0xAAAAAAAA;
    w_.cam.controller_enable = true;
    w_.cam.controller_format = VRT_IDF_UUID;
    w_.controller_uuid[0]    = 0x00112233;
    w_.controller_uuid[1]    = 0x44556677;
    w_.controller_uuid[2]    = 0x8899AABB;
    w_.controller_uuid[3]    = 0xCCDDEEFF;

    ASSERT_EQ(vrt_words_command(&h_, &w_), 7);
    ASSERT_EQ(vrt_write_command(&h_, &w_, buf_.data(), 6, true), VRT_ERR_BUFFER_SIZE);
    ASSERT_EQ(vrt_write_command(&h_, &w_, buf_.data(), 7, true), 7);
    const std::array<uint32_t, 8> expected{0xB0000000, 0x12345678, 0xAAAAAAAA, 0x00112233,
                                           0x44556677, 0x8899AABB, 0xCCDDEEFF, 0xBAADF00D};
    for (size_t i = 0; i < expected.size(); ++i) {
        ASSERT_EQ(Hex(buf_[i]), Hex(expected[i])) << i;
    }

    ASSERT_EQ(vrt_read_command(&h_, buf_.data(), 6, &r_, true), VRT_ERR_BUFFER_SIZE);
    ASSERT_EQ(vrt_read_command(&h_, buf_.data(), 7, &r_, true), 7);
    ASSERT_TRUE(r_.cam.controllee_enable);
    ASSERT_EQ(r_.cam.controllee_format, VRT_IDF_WORD);
    ASSERT_TRUE(r_.cam.controller_enable);
    ASSERT_EQ(r_.cam.controller_format, VRT_IDF_UUID);
    ASSERT_EQ(r_.message_id, 0x12345678U);
    ASSERT_EQ(r_.controllee_id, 0xAAAAAAAAU);
    ASSERT_EQ(r_.controllee_uuid[0], 0U);
    ASSERT_EQ(r_.controller_id, 0U);
    ASSERT_EQ(r_.controller_uuid[0], 0x00112233U);
    ASSERT_EQ(r_.controller_uuid[3], 0xCCDDEEFFU);
}

TEST_F(CommandTest, ControlleeUuidControllerWord) {
    w_.cam.controllee_enable = true;
    w_.cam.controllee_format = VRT_IDF_UUID;
    w_.controllee_uuid[0]    = 1;
    w_.controllee_uuid[3]    = 4;
    w_.cam.controller_enable = true;
    w_.controller_id         = 5;
    ASSERT_EQ(vrt_write_command(&h_, &w_, buf_.data(), 7, true), 7);
    ASSERT_EQ(Hex(buf_[0]), Hex(0xE0000000));
    ASSERT_EQ(Hex(buf_[2]), Hex(1));
    ASSERT_EQ(Hex(buf_[5]), Hex(4));
    ASSERT_EQ(Hex(buf_[6]), Hex(5));
    ASSERT_EQ(vrt_read_command(&h_, buf_.data(), 7, &r_, true), 7);
    ASSERT_EQ(r_.controllee_uuid[0], 1U);
    ASSERT_EQ(r_.controllee_uuid[3], 4U);
    ASSERT_EQ(r_.controller_id, 5U);
}

TEST_F(CommandTest, BufferSize) {
    ASSERT_EQ(vrt_write_command(&h_, &w_, buf_.data(), 1, true), VRT_ERR_BUFFER_SIZE);
    ASSERT_EQ(vrt_read_command(&h_, buf_.data(), 1, &r_, false), VRT_ERR_BUFFER_SIZE);
    ASSERT_EQ(vrt_read_command(&h_, buf_.data(), -1, &r_, false), VRT_ERR_BUFFER_SIZE);
}

TEST_F(CommandTest, WarningsAndErrorsOnlyInValidationAndExecutionAck) {
    w_.cam.warnings                      = true;
    w_.cam.errors                        = true;
    w_.warnings.has.bandwidth            = true;
    w_.errors.has.rf_reference_frequency = true;
    /* Control packet: W and Er request warnings and errors, no words follow */
    ASSERT_EQ(vrt_words_command(&h_, &w_), 2);
    /* Query-state acknowledge: context fields follow instead */
    h_.acknowledge     = true;
    w_.cam.query_state = true;
    ASSERT_EQ(vrt_words_command(&h_, &w_), 2);
    w_.cam.query_state = false;
    w_.cam.validation  = true;
    ASSERT_EQ(vrt_words_command(&h_, &w_), 2 + 2 + 2);
}

TEST_F(CommandTest, ExecutionAckWarningsErrors) {
    /*
     * Order from vita49-rs (src/ack.rs: wif0, wif1, eif0, eif1, then warning fields, then error fields) and vrtgen
     * (templates ack.cpp.jinja2: same order). One 32-bit response word per field (vita49-rs src/ack_response.rs).
     */
    h_.acknowledge                       = true;
    w_.message_id                        = 42;
    w_.cam.execution                     = true;
    w_.cam.warnings                      = true;
    w_.cam.errors                        = true;
    w_.cam.partial_action                = true;
    w_.warnings.has.bandwidth            = true;
    w_.warnings.bandwidth                = VRT_WEF_PARAMETER_OUT_OF_RANGE;
    w_.warnings.has.v49_spec_compliance  = true;
    w_.warnings.v49_spec_compliance      = VRT_WEF_FIELD_VALUE_INVALID;
    w_.errors.has.rf_reference_frequency = true;
    w_.errors.rf_reference_frequency     = VRT_WEF_FIELD_NOT_EXECUTED | VRT_WEF_DEVICE_FAILURE;
    w_.errors.has.sample_rate            = true;
    w_.errors.sample_rate                = VRT_WEF_REGIONAL_INTERFERENCE | 0x00000002U;
    /* Not in the indicator words, so not written */
    w_.errors.gain = 0xFFFFFFFF;

    const int32_t words = vrt_words_command(&h_, &w_);
    ASSERT_EQ(words, 2 + 2 + 1 + 2 + 2);
    ASSERT_EQ(vrt_write_command(&h_, &w_, buf_.data(), words - 1, true), VRT_ERR_BUFFER_SIZE);
    ASSERT_EQ(vrt_write_command(&h_, &w_, buf_.data(), words, true), words);
    const std::array<uint32_t, 10> expected{0x000B0800, 0x0000002A, 0x20000002, 0x00000008, 0x08200000,
                                            0x10000000, 0x04000000, 0xC0000000, 0x00080002, 0xBAADF00D};
    for (size_t i = 0; i < expected.size(); ++i) {
        ASSERT_EQ(Hex(buf_[i]), Hex(expected[i])) << i;
    }

    ASSERT_EQ(vrt_read_command(&h_, buf_.data(), words - 1, &r_, true), VRT_ERR_BUFFER_SIZE);
    ASSERT_EQ(vrt_read_command(&h_, buf_.data(), words, &r_, true), words);
    ASSERT_TRUE(r_.cam.execution);
    ASSERT_TRUE(r_.cam.partial_action);
    ASSERT_EQ(r_.message_id, 42U);
    ASSERT_TRUE(r_.warnings.has.bandwidth);
    ASSERT_TRUE(r_.warnings.has.v49_spec_compliance);
    ASSERT_FALSE(r_.warnings.has.rf_reference_frequency);
    ASSERT_EQ(Hex(r_.warnings.bandwidth), Hex(VRT_WEF_PARAMETER_OUT_OF_RANGE));
    ASSERT_EQ(Hex(r_.warnings.v49_spec_compliance), Hex(VRT_WEF_FIELD_VALUE_INVALID));
    ASSERT_EQ(Hex(r_.warnings.gain), Hex(0));
    ASSERT_TRUE(r_.errors.has.rf_reference_frequency);
    ASSERT_TRUE(r_.errors.has.sample_rate);
    ASSERT_FALSE(r_.errors.has.bandwidth);
    ASSERT_EQ(Hex(r_.errors.rf_reference_frequency), Hex(0xC0000000));
    ASSERT_EQ(Hex(r_.errors.sample_rate), Hex(0x00080002));
    ASSERT_EQ(Hex(r_.errors.gain), Hex(0));
}

TEST_F(CommandTest, ValidationAckErrorsOnly) {
    h_.acknowledge      = true;
    w_.cam.validation   = true;
    w_.cam.errors       = true;
    w_.errors.has.gain  = true;
    w_.errors.gain      = VRT_WEF_PARAMETER_UNSUPPORTED_PRECISION;
    const int32_t words = vrt_words_command(&h_, &w_);
    ASSERT_EQ(words, 4);
    ASSERT_EQ(vrt_write_command(&h_, &w_, buf_.data(), words, true), words);
    ASSERT_EQ(Hex(buf_[0]), Hex(0x00110000));
    ASSERT_EQ(Hex(buf_[2]), Hex(0x00800000));
    ASSERT_EQ(Hex(buf_[3]), Hex(0x08000000));
    ASSERT_EQ(vrt_read_command(&h_, buf_.data(), words, &r_, true), words);
    ASSERT_FALSE(r_.warnings.has.gain);
    ASSERT_TRUE(r_.errors.has.gain);
    ASSERT_EQ(Hex(r_.errors.gain), Hex(0x08000000));
}

TEST_F(CommandTest, AckIndicatorWordsUnsupportedAndReserved) {
    h_.acknowledge = true;
    buf_[0]        = 0x00120000; /* V and W */
    buf_[1]        = 0;
    buf_[2]        = 0x00000004; /* WIF2 */
    ASSERT_EQ(vrt_read_command(&h_, buf_.data(), 8, &r_, false), VRT_ERR_UNSUPPORTED_FIELD);
    buf_[2] = 0x00000010;
    ASSERT_EQ(vrt_read_command(&h_, buf_.data(), 8, &r_, true), VRT_ERR_RESERVED);
    ASSERT_EQ(vrt_read_command(&h_, buf_.data(), 8, &r_, false), 3);
    /* Bit 31 is ignored */
    buf_[2] = 0x80000000;
    ASSERT_EQ(vrt_read_command(&h_, buf_.data(), 8, &r_, true), 3);
    /* WIF1 */
    buf_[2] = 0x00000002;
    buf_[3] = 0x00000010;
    buf_[4] = 0x12345678;
    ASSERT_EQ(vrt_read_command(&h_, buf_.data(), 4, &r_, true), VRT_ERR_BUFFER_SIZE);
    ASSERT_EQ(vrt_read_command(&h_, buf_.data(), 5, &r_, true), 5);
    ASSERT_TRUE(r_.warnings.has.health_status);
    ASSERT_EQ(Hex(r_.warnings.health_status), Hex(0x12345678));
}
