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
#include "read_assertions.h"

/*
 * CIF1 (VITA 49.2). CIF0 bit 1 enables CIF1, which directly follows CIF0. CIF1 fields follow all CIF0 fields. Bit
 * positions agree in vrtgen (include/vrtgen/packing/indicator_fields.hpp: Health Status 4, V49 Spec Compliance 3,
 * Version Information 2) and vita49-rs (src/cif1.rs: health_status 4, v49_spec_compliance 3, version_and_build_code 2).
 * Version and build code subfields are from vrtgen (include/vrtgen/packing/cif1.hpp: year 31..25, day 24..16, revision
 * 15..10, user defined 9..0); vita49-rs keeps it as a raw word.
 */

class Cif1Test : public ::testing::Test {
   protected:
    void SetUp() override {
        init_garbage_if_context(&r_);
        vrt_init_if_context(&w_);
        buf_.fill(0xBAADF00D);
    }

    vrt_if_context           r_{};
    vrt_if_context           w_{};
    std::array<uint32_t, 16> buf_{};
};

TEST_F(Cif1Test, ReadV49SpecCompliance) {
    buf_[0] = 0x00000002;
    buf_[1] = 0x00000008;
    buf_[2] = 0x00000004;
    ASSERT_EQ(vrt_read_if_context(buf_.data(), 3, &r_, true), 3);
    SCOPED_TRACE(::testing::UnitTest::GetInstance()->current_test_info()->name());
    assert_if_context(r_, {{"has.v49_spec_compliance", true}, {"v49_spec_compliance", VRT_V49_2}});
}

TEST_F(Cif1Test, ReadV49SpecComplianceInvalid) {
    buf_[0] = 0x00000002;
    buf_[1] = 0x00000008;
    for (uint32_t v : {0U, 5U, 7U}) {
        buf_[2] = v;
        ASSERT_EQ(vrt_read_if_context(buf_.data(), 3, &r_, true), VRT_ERR_INVALID_V49_SPEC_COMPLIANCE);
        ASSERT_EQ(vrt_read_if_context(buf_.data(), 3, &r_, false), 3);
        ASSERT_EQ(Hex(r_.v49_spec_compliance), Hex(v));
    }
}

TEST_F(Cif1Test, ReadHealthStatus) {
    buf_[0] = 0x00000002;
    buf_[1] = 0x00000010;
    buf_[2] = 0x12345678;
    ASSERT_EQ(vrt_read_if_context(buf_.data(), 3, &r_, true), 3);
    SCOPED_TRACE(::testing::UnitTest::GetInstance()->current_test_info()->name());
    assert_if_context(r_, {{"has.health_status", true}, {"health_status", static_cast<uint32_t>(0x12345678)}});
}

TEST_F(Cif1Test, ReadVersionBuildCode) {
    buf_[0] = 0x00000002;
    buf_[1] = 0x00000004;
    buf_[2] = 0x33160C2A;
    ASSERT_EQ(vrt_read_if_context(buf_.data(), 3, &r_, true), 3);
    SCOPED_TRACE(::testing::UnitTest::GetInstance()->current_test_info()->name());
    assert_if_context(r_, {{"has.version_build_code", true},
                           {"version_build_code.year", static_cast<uint8_t>(25)},
                           {"version_build_code.day", static_cast<uint16_t>(278)},
                           {"version_build_code.revision", static_cast<uint8_t>(3)},
                           {"version_build_code.user_defined", static_cast<uint16_t>(0x2A)}});
}

TEST_F(Cif1Test, ReadEmptyCif1) {
    buf_[0] = 0x00000002;
    buf_[1] = 0x00000000;
    ASSERT_EQ(vrt_read_if_context(buf_.data(), 2, &r_, true), 2);
    SCOPED_TRACE(::testing::UnitTest::GetInstance()->current_test_info()->name());
    assert_if_context(r_, {});
}

TEST_F(Cif1Test, ReadCif1AfterVariableFields) {
    /* CIF1 fields follow GPS ASCII and Context association lists, the variable size CIF0 fields */
    buf_[0]  = 0x20000302;
    buf_[1]  = 0x0000001C;
    buf_[2]  = 0x00000000;
    buf_[3]  = 0x00100000;
    buf_[4]  = 0x00ABCDEF;
    buf_[5]  = 0x00000001;
    buf_[6]  = 0x41424344;
    buf_[7]  = 0x00000000;
    buf_[8]  = 0x00000000;
    buf_[9]  = 0xCAFEBABE;
    buf_[10] = 0x00000001;
    buf_[11] = 0xFFFFFFFF;
    ASSERT_EQ(vrt_read_if_context(buf_.data(), 12, &r_, true), 12);
    SCOPED_TRACE(::testing::UnitTest::GetInstance()->current_test_info()->name());
    assert_if_context(r_, {{"has.bandwidth", true},
                           {"bandwidth", 1.0},
                           {"has.gps_ascii", true},
                           {"gps_ascii.oui", static_cast<uint32_t>(0x00ABCDEF)},
                           {"gps_ascii.number_of_words", static_cast<uint32_t>(1)},
                           {"gps_ascii.ascii", reinterpret_cast<const char*>(buf_.data() + 6)},
                           {"has.context_association_lists", true},
                           {"has.health_status", true},
                           {"health_status", static_cast<uint32_t>(0xCAFEBABE)},
                           {"has.v49_spec_compliance", true},
                           {"v49_spec_compliance", VRT_V49_0},
                           {"has.version_build_code", true},
                           {"version_build_code.year", static_cast<uint8_t>(0x7F)},
                           {"version_build_code.day", static_cast<uint16_t>(0x1FF)},
                           {"version_build_code.revision", static_cast<uint8_t>(0x3F)},
                           {"version_build_code.user_defined", static_cast<uint16_t>(0x3FF)}});
    /* One word short */
    ASSERT_EQ(vrt_read_if_context(buf_.data(), 11, &r_, true), VRT_ERR_BUFFER_SIZE);
}

TEST_F(Cif1Test, ReadCif1BufferSize) {
    buf_[0] = 0x00000002;
    buf_[1] = 0x00000008;
    buf_[2] = 0x00000004;
    ASSERT_EQ(vrt_read_if_context(buf_.data(), 1, &r_, true), VRT_ERR_BUFFER_SIZE);
    ASSERT_EQ(vrt_read_if_context(buf_.data(), 2, &r_, true), VRT_ERR_BUFFER_SIZE);
    ASSERT_EQ(vrt_read_if_context(buf_.data(), 2, &r_, false), VRT_ERR_BUFFER_SIZE);
}

TEST_F(Cif1Test, ReadGpsAsciiBufferSize) {
    /* Number of words beyond the buffer */
    buf_[0] = 0x00000200;
    buf_[1] = 0x00000000;
    buf_[2] = 0x00000004;
    ASSERT_EQ(vrt_read_if_context(buf_.data(), 6, &r_, false), VRT_ERR_BUFFER_SIZE);
    ASSERT_EQ(vrt_read_if_context(buf_.data(), 7, &r_, true), 7);
    buf_[2] = 0xFFFFFFFF;
    ASSERT_EQ(vrt_read_if_context(buf_.data(), 16, &r_, false), VRT_ERR_BUFFER_SIZE);
}

TEST_F(Cif1Test, ReadUnsupportedIndicatorWords) {
    /* CIF2, CIF3 and CIF7 enable bits */
    for (uint32_t bit : {0x00000004U, 0x00000008U, 0x00000080U}) {
        buf_[0] = bit;
        ASSERT_EQ(vrt_read_if_context(buf_.data(), 16, &r_, true), VRT_ERR_UNSUPPORTED_FIELD) << Hex(bit);
        ASSERT_EQ(vrt_read_if_context(buf_.data(), 16, &r_, false), VRT_ERR_UNSUPPORTED_FIELD) << Hex(bit);
    }
}

TEST_F(Cif1Test, ReadUnsupportedCif1Fields) {
    buf_[0] = 0x00000002;
    for (uint32_t i = 0; i < 32; ++i) {
        uint32_t bit = 1U << i;
        buf_[1]      = bit;
        int32_t rv   = vrt_read_if_context(buf_.data(), 16, &r_, false);
        if (bit == 0x10 || bit == 0x08 || bit == 0x04) {
            ASSERT_EQ(rv, 3) << Hex(bit);
        } else if ((bit & 0x00E01101U) != 0) {
            /* Reserved */
            ASSERT_EQ(rv, 2) << Hex(bit);
            ASSERT_EQ(vrt_read_if_context(buf_.data(), 16, &r_, true), VRT_ERR_RESERVED) << Hex(bit);
        } else {
            ASSERT_EQ(rv, VRT_ERR_UNSUPPORTED_FIELD) << Hex(bit);
        }
    }
}

TEST_F(Cif1Test, ReadVita49RsContextVector) {
    /*
     * vita49-rs tests/context_packet.json5: CIF0 = 673316866 (0x28220002), CIF1 = 1032 (0x408). CIF1 bit 10 is the
     * Spectrum field, which is not supported.
     */
    buf_[0] = 0x28220002;
    buf_[1] = 0x00000408;
    ASSERT_EQ(vrt_read_if_context(buf_.data(), 16, &r_, true), VRT_ERR_UNSUPPORTED_FIELD);
}

TEST_F(Cif1Test, ReadReservedCif0) {
    for (uint32_t bit : {0x00000001U, 0x00000010U, 0x00000020U, 0x00000040U}) {
        buf_[0] = bit;
        ASSERT_EQ(vrt_read_if_context(buf_.data(), 1, &r_, true), VRT_ERR_RESERVED) << Hex(bit);
        ASSERT_EQ(vrt_read_if_context(buf_.data(), 1, &r_, false), 1) << Hex(bit);
    }
}

TEST_F(Cif1Test, WriteV49SpecCompliance) {
    w_.has.v49_spec_compliance = true;
    w_.v49_spec_compliance     = VRT_V49_2;
    ASSERT_EQ(vrt_words_if_context(&w_), 3);
    ASSERT_EQ(vrt_write_if_context(&w_, buf_.data(), 3, true), 3);
    ASSERT_EQ(Hex(buf_[0]), Hex(0x00000002));
    ASSERT_EQ(Hex(buf_[1]), Hex(0x00000008));
    ASSERT_EQ(Hex(buf_[2]), Hex(0x00000004));
    ASSERT_EQ(Hex(buf_[3]), Hex(0xBAADF00D));
}

TEST_F(Cif1Test, WriteV49SpecComplianceInvalid) {
    w_.has.v49_spec_compliance = true;
    w_.v49_spec_compliance     = static_cast<vrt_v49_spec_compliance>(5);
    ASSERT_EQ(vrt_write_if_context(&w_, buf_.data(), 3, true), VRT_ERR_INVALID_V49_SPEC_COMPLIANCE);
    w_.v49_spec_compliance = static_cast<vrt_v49_spec_compliance>(0);
    ASSERT_EQ(vrt_write_if_context(&w_, buf_.data(), 3, true), VRT_ERR_INVALID_V49_SPEC_COMPLIANCE);
    ASSERT_EQ(vrt_write_if_context(&w_, buf_.data(), 3, false), 3);
    ASSERT_EQ(Hex(buf_[2]), Hex(0x00000000));
}

TEST_F(Cif1Test, WriteVersionBuildCode) {
    w_.has.version_build_code          = true;
    w_.version_build_code.year         = 25;
    w_.version_build_code.day          = 278;
    w_.version_build_code.revision     = 3;
    w_.version_build_code.user_defined = 0x2A;
    ASSERT_EQ(vrt_write_if_context(&w_, buf_.data(), 3, true), 3);
    ASSERT_EQ(Hex(buf_[0]), Hex(0x00000002));
    ASSERT_EQ(Hex(buf_[1]), Hex(0x00000004));
    ASSERT_EQ(Hex(buf_[2]), Hex(0x33160C2A));
}

TEST_F(Cif1Test, WriteVersionBuildCodeBounds) {
    w_.has.version_build_code  = true;
    w_.version_build_code.year = 0x80;
    ASSERT_EQ(vrt_write_if_context(&w_, buf_.data(), 3, true), VRT_ERR_BOUNDS_VERSION_BUILD_CODE);
    w_.version_build_code.year = 0;
    w_.version_build_code.day  = 0x200;
    ASSERT_EQ(vrt_write_if_context(&w_, buf_.data(), 3, true), VRT_ERR_BOUNDS_VERSION_BUILD_CODE);
    w_.version_build_code.day      = 0;
    w_.version_build_code.revision = 0x40;
    ASSERT_EQ(vrt_write_if_context(&w_, buf_.data(), 3, true), VRT_ERR_BOUNDS_VERSION_BUILD_CODE);
    w_.version_build_code.revision     = 0;
    w_.version_build_code.user_defined = 0x400;
    ASSERT_EQ(vrt_write_if_context(&w_, buf_.data(), 3, true), VRT_ERR_BOUNDS_VERSION_BUILD_CODE);
    /* Masked without validation */
    ASSERT_EQ(vrt_write_if_context(&w_, buf_.data(), 3, false), 3);
    ASSERT_EQ(Hex(buf_[2]), Hex(0x00000000));
}

TEST_F(Cif1Test, WriteBufferSize) {
    w_.has.health_status = true;
    ASSERT_EQ(vrt_write_if_context(&w_, buf_.data(), 2, true), VRT_ERR_BUFFER_SIZE);
    ASSERT_EQ(vrt_write_if_context(&w_, buf_.data(), 2, false), VRT_ERR_BUFFER_SIZE);
}

TEST_F(Cif1Test, RoundTripWithCif0) {
    w_.context_field_change_indicator                            = true;
    w_.has.rf_reference_frequency                                = true;
    w_.rf_reference_frequency                                    = 2412e6;
    w_.has.context_association_lists                             = true;
    uint32_t lists[]                                             = {0x11111111, 0x22222222};
    w_.context_association_lists.source_list_size                = 1;
    w_.context_association_lists.source_context_association_list = lists;
    w_.context_association_lists.system_list_size                = 1;
    w_.context_association_lists.system_context_association_list = lists + 1;
    w_.has.health_status                                         = true;
    w_.health_status                                             = 0xA5A5A5A5;
    w_.has.v49_spec_compliance                                   = true;
    w_.v49_spec_compliance                                       = VRT_V49_2;
    w_.has.version_build_code                                    = true;
    w_.version_build_code.year                                   = 26;
    w_.version_build_code.day                                    = 278;
    w_.version_build_code.revision                               = 1;
    w_.version_build_code.user_defined                           = 7;

    const int32_t words = vrt_words_if_context(&w_);
    ASSERT_EQ(words, 1 + 1 + 2 + 4 + 3);
    ASSERT_EQ(vrt_write_if_context(&w_, buf_.data(), words, true), words);
    ASSERT_EQ(Hex(buf_[0]), Hex(0x88000102));
    ASSERT_EQ(Hex(buf_[1]), Hex(0x0000001C));
    ASSERT_EQ(Hex(buf_[4]), Hex(0x00010001));
    ASSERT_EQ(Hex(buf_[5]), Hex(0x00000000));
    ASSERT_EQ(Hex(buf_[6]), Hex(0x11111111));
    ASSERT_EQ(Hex(buf_[7]), Hex(0x22222222));
    ASSERT_EQ(Hex(buf_[8]), Hex(0xA5A5A5A5));
    ASSERT_EQ(Hex(buf_[9]), Hex(0x00000004));
    ASSERT_EQ(Hex(buf_[10]), Hex(0x35160407));

    ASSERT_EQ(vrt_read_if_context(buf_.data(), words, &r_, true), words);
    SCOPED_TRACE(::testing::UnitTest::GetInstance()->current_test_info()->name());
    assert_if_context(
        r_,
        {{"context_field_change_indicator", true},
         {"has.rf_reference_frequency", true},
         {"rf_reference_frequency", 2412e6},
         {"has.context_association_lists", true},
         {"context_association_lists.source_list_size", static_cast<uint16_t>(1)},
         {"context_association_lists.system_list_size", static_cast<uint16_t>(1)},
         {"context_association_lists.source_context_association_list", static_cast<const uint32_t*>(buf_.data() + 6)},
         {"context_association_lists.system_context_association_list", static_cast<const uint32_t*>(buf_.data() + 7)},
         {"has.health_status", true},
         {"health_status", static_cast<uint32_t>(0xA5A5A5A5)},
         {"has.v49_spec_compliance", true},
         {"has.version_build_code", true},
         {"version_build_code.year", static_cast<uint8_t>(26)},
         {"version_build_code.day", static_cast<uint16_t>(278)},
         {"version_build_code.revision", static_cast<uint8_t>(1)},
         {"version_build_code.user_defined", static_cast<uint16_t>(7)}});
}

TEST(Cif1PacketTest, ContextPacketNd0RoundTrip) {
    vrt_packet p{};
    vrt_init_packet(&p);
    p.header.packet_type                 = VRT_PT_IF_CONTEXT;
    p.header.nd0                         = true;
    p.fields.stream_id                   = 0x5D5D0001;
    p.if_context.has.sample_rate         = true;
    p.if_context.sample_rate             = 16e6;
    p.if_context.has.v49_spec_compliance = true;
    p.if_context.v49_spec_compliance     = VRT_V49_2;

    std::array<uint32_t, 16> buf{};
    const int32_t            words = vrt_words_packet(&p);
    ASSERT_EQ(words, 1 + 1 + 2 + 2 + 1);
    ASSERT_EQ(vrt_write_packet(&p, buf.data(), static_cast<int32_t>(buf.size()), true), words);
    ASSERT_EQ(Hex(buf[0]), Hex(0x42000007));
    ASSERT_EQ(Hex(buf[2]), Hex(0x00200002));
    ASSERT_EQ(Hex(buf[3]), Hex(0x00000008));
    ASSERT_EQ(Hex(buf[6]), Hex(0x00000004));

    vrt_packet r{};
    init_garbage_packet(&r);
    ASSERT_EQ(vrt_read_packet(buf.data(), words, &r, true), words);
    ASSERT_TRUE(r.header.nd0);
    ASSERT_TRUE(r.if_context.has.v49_spec_compliance);
    ASSERT_EQ(r.if_context.v49_spec_compliance, VRT_V49_2);
    ASSERT_EQ(r.if_context.sample_rate, 16e6);
}
