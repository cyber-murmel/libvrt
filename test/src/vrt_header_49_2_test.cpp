#include <gtest/gtest.h>

#include <array>
#include <cstdint>

#include <vrt/vrt_error_code.h>
#include <vrt/vrt_init.h>
#include <vrt/vrt_read.h>
#include <vrt/vrt_types.h>
#include <vrt/vrt_util.h>
#include <vrt/vrt_write.h>

#include "hex.h"
#include "init_garbage.h"
#include "read_assertions.h"

/*
 * VITA 49.2 header bits 26 to 24. Layout agrees in vrtgen (include/vrtgen/packing/header.hpp: DataHeader,
 * ContextHeader, CommandHeader) and vita49-rs (src/packet_header.rs: Indicators):
 *   data:    26 trailer,     25 Nd0,      24 spectrum (S)
 *   context: 26 reserved,    25 Nd0,      24 TSM
 *   command: 26 acknowledge, 25 reserved, 24 cancellation
 */

class Header492Test : public ::testing::Test {
   protected:
    void SetUp() override {
        init_garbage_header(&r_);
        vrt_init_header(&w_);
        buf_.fill(0xBAADF00D);
    }

    vrt_header              r_{};
    vrt_header              w_{};
    std::array<uint32_t, 2> buf_{};
};

TEST_F(Header492Test, ReadNd0Data) {
    buf_[0] = 0x12000000;
    ASSERT_EQ(vrt_read_header(buf_.data(), 1, &r_, true), 1);
    SCOPED_TRACE(::testing::UnitTest::GetInstance()->current_test_info()->name());
    assert_header(r_, {{"packet_type", VRT_PT_IF_DATA_WITH_STREAM_ID}, {"nd0", true}});
}

TEST_F(Header492Test, ReadNd0Context) {
    buf_[0] = 0x43000000;
    ASSERT_EQ(vrt_read_header(buf_.data(), 1, &r_, true), 1);
    SCOPED_TRACE(::testing::UnitTest::GetInstance()->current_test_info()->name());
    assert_header(r_, {{"packet_type", VRT_PT_IF_CONTEXT}, {"nd0", true}, {"tsm", VRT_TSM_COARSE}});
}

TEST_F(Header492Test, ReadSpectrum) {
    buf_[0] = 0x37000000;
    ASSERT_EQ(vrt_read_header(buf_.data(), 1, &r_, true), 1);
    SCOPED_TRACE(::testing::UnitTest::GetInstance()->current_test_info()->name());
    assert_header(r_, {{"packet_type", VRT_PT_EXT_DATA_WITH_STREAM_ID},
                       {"has.trailer", true},
                       {"nd0", true},
                       {"spectrum_or_time", VRT_SOT_SPECTRUM}});
}

TEST_F(Header492Test, ReadCommand) {
    buf_[0] = 0x60000000;
    ASSERT_EQ(vrt_read_header(buf_.data(), 1, &r_, true), 1);
    SCOPED_TRACE(::testing::UnitTest::GetInstance()->current_test_info()->name());
    assert_header(r_, {{"packet_type", VRT_PT_COMMAND}});
}

TEST_F(Header492Test, ReadExtCommand) {
    buf_[0] = 0x70000000;
    ASSERT_EQ(vrt_read_header(buf_.data(), 1, &r_, true), 1);
    SCOPED_TRACE(::testing::UnitTest::GetInstance()->current_test_info()->name());
    assert_header(r_, {{"packet_type", VRT_PT_EXT_COMMAND}});
}

TEST_F(Header492Test, ReadAcknowledge) {
    buf_[0] = 0x64000000;
    ASSERT_EQ(vrt_read_header(buf_.data(), 1, &r_, true), 1);
    SCOPED_TRACE(::testing::UnitTest::GetInstance()->current_test_info()->name());
    assert_header(r_, {{"packet_type", VRT_PT_COMMAND}, {"acknowledge", true}});
}

TEST_F(Header492Test, ReadCancellation) {
    buf_[0] = 0x61000000;
    ASSERT_EQ(vrt_read_header(buf_.data(), 1, &r_, true), 1);
    SCOPED_TRACE(::testing::UnitTest::GetInstance()->current_test_info()->name());
    assert_header(r_, {{"packet_type", VRT_PT_COMMAND}, {"cancellation", true}});
}

TEST_F(Header492Test, ReadCommandReservedBit25) {
    /* Reserved in command packets, but set by e.g. the vita49-rs test vector tests/command_packet.json5 */
    buf_[0] = 0x62600013;
    ASSERT_EQ(vrt_read_header(buf_.data(), 1, &r_, true), 1);
    SCOPED_TRACE(::testing::UnitTest::GetInstance()->current_test_info()->name());
    assert_header(r_, {{"packet_type", VRT_PT_COMMAND},
                       {"nd0", true},
                       {"tsi", VRT_TSI_UTC},
                       {"tsf", VRT_TSF_REAL_TIME},
                       {"packet_size", static_cast<uint16_t>(19)}});
}

TEST_F(Header492Test, ReadAllCommand) {
    buf_[0] = 0x7F7FFFFF;
    ASSERT_EQ(vrt_read_header(buf_.data(), 1, &r_, true), 1);
    SCOPED_TRACE(::testing::UnitTest::GetInstance()->current_test_info()->name());
    assert_header(r_, {{"packet_type", VRT_PT_EXT_COMMAND},
                       {"has.class_id", true},
                       {"acknowledge", true},
                       {"nd0", true},
                       {"cancellation", true},
                       {"tsi", VRT_TSI_UTC},
                       {"tsf", VRT_TSF_FREE_RUNNING_COUNT},
                       {"packet_count", static_cast<uint8_t>(0xF)},
                       {"packet_size", static_cast<uint16_t>(0xFFFF)}});
}

TEST_F(Header492Test, ReadContextTrailerStillInvalid) {
    buf_[0] = 0x46000000;
    ASSERT_EQ(vrt_read_header(buf_.data(), 1, &r_, true), VRT_ERR_TRAILER_IN_CONTEXT);
}

TEST_F(Header492Test, WriteNd0Data) {
    w_.packet_type = VRT_PT_IF_DATA_WITH_STREAM_ID;
    w_.nd0         = true;
    ASSERT_EQ(vrt_write_header(&w_, buf_.data(), 1, true), 1);
    ASSERT_EQ(Hex(buf_[0]), Hex(0x12000000));
    ASSERT_EQ(Hex(buf_[1]), Hex(0xBAADF00D));
}

TEST_F(Header492Test, WriteNd0Context) {
    w_.packet_type = VRT_PT_IF_CONTEXT;
    w_.nd0         = true;
    w_.tsm         = VRT_TSM_COARSE;
    ASSERT_EQ(vrt_write_header(&w_, buf_.data(), 1, true), 1);
    ASSERT_EQ(Hex(buf_[0]), Hex(0x43000000));
}

TEST_F(Header492Test, WriteSpectrum) {
    w_.packet_type      = VRT_PT_IF_DATA_WITH_STREAM_ID;
    w_.has.class_id     = true;
    w_.has.trailer      = true;
    w_.nd0              = true;
    w_.spectrum_or_time = VRT_SOT_SPECTRUM;
    w_.tsi              = VRT_TSI_UTC;
    w_.tsf              = VRT_TSF_REAL_TIME;
    w_.packet_size      = 328;
    /* vita49-rs tests/spectral_data_packet.json5 header has hword_1 = 7520 (0x1D60), with Nd0 clear */
    w_.nd0 = false;
    ASSERT_EQ(vrt_write_header(&w_, buf_.data(), 1, true), 1);
    ASSERT_EQ(Hex(buf_[0]), Hex(0x1D600148));
    w_.nd0 = true;
    ASSERT_EQ(vrt_write_header(&w_, buf_.data(), 1, true), 1);
    ASSERT_EQ(Hex(buf_[0]), Hex(0x1F600148));
}

TEST_F(Header492Test, WriteAcknowledge) {
    w_.packet_type = VRT_PT_COMMAND;
    w_.acknowledge = true;
    w_.tsi         = VRT_TSI_UTC;
    w_.tsf         = VRT_TSF_REAL_TIME;
    w_.packet_size = 12;
    /* vita49-rs tests/ack_packet.json5: hword_1 = 25696 (0x6460), packet_size = 12 */
    ASSERT_EQ(vrt_write_header(&w_, buf_.data(), 1, true), 1);
    ASSERT_EQ(Hex(buf_[0]), Hex(0x6460000C));
}

TEST_F(Header492Test, WriteCancellation) {
    w_.packet_type  = VRT_PT_COMMAND;
    w_.cancellation = true;
    ASSERT_EQ(vrt_write_header(&w_, buf_.data(), 1, true), 1);
    ASSERT_EQ(Hex(buf_[0]), Hex(0x61000000));
}

TEST_F(Header492Test, WriteExtCommand) {
    w_.packet_type  = VRT_PT_EXT_COMMAND;
    w_.acknowledge  = true;
    w_.cancellation = true;
    ASSERT_EQ(vrt_write_header(&w_, buf_.data(), 1, true), 1);
    ASSERT_EQ(Hex(buf_[0]), Hex(0x75000000));
}

TEST_F(Header492Test, WriteNd0InCommand) {
    w_.packet_type = VRT_PT_COMMAND;
    w_.nd0         = true;
    ASSERT_EQ(vrt_write_header(&w_, buf_.data(), 1, true), VRT_ERR_INDICATOR_IN_COMMAND);
    ASSERT_EQ(vrt_write_header(&w_, buf_.data(), 1, false), 1);
    ASSERT_EQ(Hex(buf_[0]), Hex(0x62000000));
}

TEST_F(Header492Test, WriteTrailerInCommand) {
    w_.packet_type = VRT_PT_COMMAND;
    w_.has.trailer = true;
    ASSERT_EQ(vrt_write_header(&w_, buf_.data(), 1, true), VRT_ERR_INDICATOR_IN_COMMAND);
}

TEST_F(Header492Test, WriteTsmInCommand) {
    w_.packet_type = VRT_PT_EXT_COMMAND;
    w_.tsm         = VRT_TSM_COARSE;
    ASSERT_EQ(vrt_write_header(&w_, buf_.data(), 1, true), VRT_ERR_INDICATOR_IN_COMMAND);
}

TEST_F(Header492Test, WriteSpectrumInContext) {
    w_.packet_type      = VRT_PT_IF_CONTEXT;
    w_.spectrum_or_time = VRT_SOT_SPECTRUM;
    ASSERT_EQ(vrt_write_header(&w_, buf_.data(), 1, true), VRT_ERR_SPECTRUM_IN_NON_DATA);
    ASSERT_EQ(vrt_write_header(&w_, buf_.data(), 1, false), 1);
    ASSERT_EQ(Hex(buf_[0]), Hex(0x41000000));
}

TEST_F(Header492Test, WriteSpectrumInCommand) {
    w_.packet_type      = VRT_PT_COMMAND;
    w_.spectrum_or_time = VRT_SOT_SPECTRUM;
    ASSERT_EQ(vrt_write_header(&w_, buf_.data(), 1, true), VRT_ERR_SPECTRUM_IN_NON_DATA);
}

TEST_F(Header492Test, WriteSpectrumInvalid) {
    w_.spectrum_or_time = static_cast<vrt_spectrum_or_time>(2);
    ASSERT_EQ(vrt_write_header(&w_, buf_.data(), 1, true), VRT_ERR_INVALID_SPECTRUM_OR_TIME);
}

TEST_F(Header492Test, WriteAcknowledgeInData) {
    w_.acknowledge = true;
    ASSERT_EQ(vrt_write_header(&w_, buf_.data(), 1, true), VRT_ERR_COMMAND_INDICATOR_IN_NON_COMMAND);
    ASSERT_EQ(vrt_write_header(&w_, buf_.data(), 1, false), 1);
    ASSERT_EQ(Hex(buf_[0]), Hex(0x04000000));
}

TEST_F(Header492Test, WriteCancellationInContext) {
    w_.packet_type  = VRT_PT_EXT_CONTEXT;
    w_.cancellation = true;
    ASSERT_EQ(vrt_write_header(&w_, buf_.data(), 1, true), VRT_ERR_COMMAND_INDICATOR_IN_NON_COMMAND);
}

TEST_F(Header492Test, RoundTripAllTypes) {
    for (uint32_t t = VRT_PT_IF_DATA_WITHOUT_STREAM_ID; t <= VRT_PT_EXT_COMMAND; ++t) {
        vrt_init_header(&w_);
        w_.packet_type  = static_cast<vrt_packet_type>(t);
        w_.has.class_id = true;
        w_.tsi          = VRT_TSI_GPS;
        w_.tsf          = VRT_TSF_SAMPLE_COUNT;
        w_.packet_count = 0x9;
        w_.packet_size  = 0x1234;
        if (vrt_is_data(&w_)) {
            w_.has.trailer      = true;
            w_.nd0              = true;
            w_.spectrum_or_time = VRT_SOT_SPECTRUM;
        } else if (vrt_is_context(&w_)) {
            w_.nd0 = true;
            w_.tsm = VRT_TSM_COARSE;
        } else {
            w_.acknowledge  = true;
            w_.cancellation = true;
        }
        ASSERT_EQ(vrt_write_header(&w_, buf_.data(), 1, true), 1) << "type " << t;
        ASSERT_EQ(vrt_read_header(buf_.data(), 1, &r_, true), 1) << "type " << t;
        ASSERT_EQ(r_.packet_type, w_.packet_type);
        ASSERT_EQ(r_.has.class_id, w_.has.class_id);
        ASSERT_EQ(r_.has.trailer, w_.has.trailer);
        ASSERT_EQ(r_.tsm, w_.tsm);
        ASSERT_EQ(r_.nd0, w_.nd0);
        ASSERT_EQ(r_.spectrum_or_time, w_.spectrum_or_time);
        ASSERT_EQ(r_.acknowledge, w_.acknowledge);
        ASSERT_EQ(r_.cancellation, w_.cancellation);
        ASSERT_EQ(r_.tsi, w_.tsi);
        ASSERT_EQ(r_.tsf, w_.tsf);
        ASSERT_EQ(r_.packet_count, w_.packet_count);
        ASSERT_EQ(r_.packet_size, w_.packet_size);
    }
}

TEST(PacketTypeHelpersTest, IsDataIsContextIsCommand) {
    vrt_header h{};
    for (uint32_t t = 0; t <= 7; ++t) {
        h.packet_type = static_cast<vrt_packet_type>(t);
        ASSERT_EQ(vrt_is_data(&h), t <= 3) << t;
        ASSERT_EQ(vrt_is_context(&h), t == 4 || t == 5) << t;
        ASSERT_EQ(vrt_is_command(&h), t == 6 || t == 7) << t;
    }
}

TEST(PacketTypeHelpersTest, CommandHasStreamId) {
    vrt_header h{};
    h.packet_type = VRT_PT_COMMAND;
    ASSERT_TRUE(vrt_has_stream_id(&h));
    h.packet_type = VRT_PT_EXT_COMMAND;
    ASSERT_TRUE(vrt_has_stream_id(&h));
}
