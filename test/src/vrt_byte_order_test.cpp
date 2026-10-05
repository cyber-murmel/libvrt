#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <cstring>

#include <vrt/vrt_error_code.h>
#include <vrt/vrt_string.h>
#include <vrt/vrt_types.h>
#include <vrt/vrt_util.h>

#include "hex.h"

TEST(ByteOrderTest, ToBigEndian) {
    std::array<uint32_t, 3> buf{0x12345678, 0xAABBCCDD, 0xBAADF00D};
    vrt_to_big_endian(buf.data(), 2);
    std::array<uint8_t, 12> bytes{};
    std::memcpy(bytes.data(), buf.data(), bytes.size());
    const std::array<uint8_t, 8> expected{0x12, 0x34, 0x56, 0x78, 0xAA, 0xBB, 0xCC, 0xDD};
    for (size_t i = 0; i < expected.size(); ++i) {
        ASSERT_EQ(Hex(bytes[i]), Hex(expected[i])) << i;
    }
    /* Words after the given count are untouched */
    ASSERT_EQ(Hex(buf[2]), Hex(0xBAADF00D));
}

TEST(ByteOrderTest, FromBigEndian) {
    const std::array<uint8_t, 8> bytes{0x64, 0x60, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x01};
    std::array<uint32_t, 2>      buf{};
    std::memcpy(buf.data(), bytes.data(), bytes.size());
    vrt_from_big_endian(buf.data(), 2);
    ASSERT_EQ(Hex(buf[0]), Hex(0x6460000C));
    ASSERT_EQ(Hex(buf[1]), Hex(0x00000001));
}

TEST(ByteOrderTest, RoundTrip) {
    std::array<uint32_t, 4> buf{0x01020304, 0xFFFFFFFF, 0x00000000, 0x80000001};
    const auto              orig = buf;
    vrt_to_big_endian(buf.data(), static_cast<int32_t>(buf.size()));
    if (vrt_is_platform_little_endian()) {
        ASSERT_EQ(Hex(buf[0]), Hex(0x04030201));
        ASSERT_EQ(Hex(buf[3]), Hex(0x01000080));
    } else {
        ASSERT_EQ(buf, orig);
    }
    vrt_from_big_endian(buf.data(), static_cast<int32_t>(buf.size()));
    ASSERT_EQ(buf, orig);
}

TEST(ByteOrderTest, NonPositiveWords) {
    std::array<uint32_t, 1> buf{0x12345678};
    vrt_to_big_endian(buf.data(), 0);
    vrt_to_big_endian(buf.data(), -1);
    vrt_from_big_endian(buf.data(), -5);
    ASSERT_EQ(Hex(buf[0]), Hex(0x12345678));
}

TEST(String492Test, Enums) {
    ASSERT_STREQ(vrt_string_packet_type(VRT_PT_COMMAND), "Command packet");
    ASSERT_STREQ(vrt_string_packet_type(VRT_PT_EXT_COMMAND), "Extension Command packet");
    ASSERT_STREQ(vrt_string_spectrum_or_time(VRT_SOT_TIME), "Time");
    ASSERT_STREQ(vrt_string_spectrum_or_time(VRT_SOT_SPECTRUM), "Spectrum");
    ASSERT_STREQ(vrt_string_spectrum_or_time(static_cast<vrt_spectrum_or_time>(2)), "Unknown");
    ASSERT_STREQ(vrt_string_v49_spec_compliance(VRT_V49_2), "V49.2");
    ASSERT_STREQ(vrt_string_v49_spec_compliance(static_cast<vrt_v49_spec_compliance>(0)), "Unknown");
    ASSERT_STREQ(vrt_string_identifier_format(VRT_IDF_UUID), "128-bit UUID");
    ASSERT_STREQ(vrt_string_action_mode(VRT_AM_EXECUTE), "Execute");
    ASSERT_STREQ(vrt_string_action_mode(static_cast<vrt_action_mode>(3)), "Unknown");
    ASSERT_STREQ(vrt_string_timing_control(VRT_TC_TIMING_ISSUES), "Timing Issues");
    ASSERT_STREQ(vrt_string_timing_control(static_cast<vrt_timing_control>(5)), "Unknown");
}

TEST(String492Test, Errors) {
    for (int32_t e = VRT_ERR_SPECTRUM_IN_NON_DATA; e >= VRT_ERR_MISSING_COMMAND; --e) {
        ASSERT_STRNE(vrt_string_error(e), "Unknown") << e;
    }
    ASSERT_STREQ(vrt_string_error(VRT_ERR_MISSING_COMMAND - 1), "Unknown");
}
