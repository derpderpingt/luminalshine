/**
 * @file tests/unit/test_pyrowave_framing.cpp
 * @brief Unit tests for the PyroWave frame payload layout and bitrate cap.
 *
 * The framing is the wire contract with PyroWave-aware clients, so it is
 * pinned byte for byte here. These tests do not need libpyrowave and run in
 * every build.
 */
#include "../tests_common.h"
#include "src/pyrowave_framing.h"

#include <array>

namespace pw = video::pyrowave;

namespace {
  std::uint32_t read_u32(const std::vector<std::uint8_t> &buf, std::size_t offset) {
    return static_cast<std::uint32_t>(buf[offset]) |
           (static_cast<std::uint32_t>(buf[offset + 1]) << 8) |
           (static_cast<std::uint32_t>(buf[offset + 2]) << 16) |
           (static_cast<std::uint32_t>(buf[offset + 3]) << 24);
  }
}  // namespace

TEST(PyroWaveFraming, SinglePacketLayout) {
  const std::array<std::uint8_t, 6> bitstream {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
  const std::array<pw::packet_ref_t, 1> packets {{{1, 3}}};

  auto framed = pw::frame_packets(bitstream, packets);
  ASSERT_TRUE(framed);
  ASSERT_EQ(framed->size(), 8u + 4u + 3u);
  EXPECT_EQ(read_u32(*framed, 0), pw::payload_magic);
  EXPECT_EQ((*framed)[0], 'P');
  EXPECT_EQ((*framed)[1], 'Y');
  EXPECT_EQ((*framed)[2], 'W');
  EXPECT_EQ((*framed)[3], '1');
  EXPECT_EQ(read_u32(*framed, 4), 1u);
  EXPECT_EQ(read_u32(*framed, 8), 3u);
  EXPECT_EQ((*framed)[12], 0xBB);
  EXPECT_EQ((*framed)[13], 0xCC);
  EXPECT_EQ((*framed)[14], 0xDD);
}

TEST(PyroWaveFraming, MultiplePacketsKeepOrderAndSizes) {
  const std::array<std::uint8_t, 8> bitstream {1, 2, 3, 4, 5, 6, 7, 8};
  // Out-of-order offsets: payload order must follow the packet list, not offsets.
  const std::array<pw::packet_ref_t, 3> packets {{{6, 2}, {0, 1}, {3, 0}}};

  auto framed = pw::frame_packets(bitstream, packets);
  ASSERT_TRUE(framed);
  ASSERT_EQ(framed->size(), 8u + 3u * 4u + 3u);
  EXPECT_EQ(read_u32(*framed, 4), 3u);
  EXPECT_EQ(read_u32(*framed, 8), 2u);
  EXPECT_EQ(read_u32(*framed, 12), 1u);
  EXPECT_EQ(read_u32(*framed, 16), 0u);
  EXPECT_EQ((*framed)[20], 7);
  EXPECT_EQ((*framed)[21], 8);
  EXPECT_EQ((*framed)[22], 1);
}

TEST(PyroWaveFraming, RejectsEmptyAndOutOfRange) {
  const std::array<std::uint8_t, 4> bitstream {};
  EXPECT_FALSE(pw::frame_packets(bitstream, std::span<const pw::packet_ref_t> {}));

  const std::array<pw::packet_ref_t, 1> past_end {{{2, 3}}};
  EXPECT_FALSE(pw::frame_packets(bitstream, past_end));

  const std::array<pw::packet_ref_t, 1> bad_offset {{{5, 0}}};
  EXPECT_FALSE(pw::frame_packets(bitstream, bad_offset));

  // Offset + size overflow must not wrap into range.
  const std::array<pw::packet_ref_t, 1> overflow {{{1, SIZE_MAX}}};
  EXPECT_FALSE(pw::frame_packets(bitstream, overflow));
}

TEST(PyroWaveFraming, RejectsTooManyPackets) {
  const std::array<std::uint8_t, 1> bitstream {};
  std::vector<pw::packet_ref_t> packets(pw::max_packets_per_frame + 1, {0, 0});
  EXPECT_FALSE(pw::frame_packets(bitstream, packets));
}

TEST(PyroWaveFraming, FrameBudgetFromBitrate) {
  // 300 Mbps at 60 fps -> 625000 bytes per frame.
  EXPECT_EQ(pw::max_frame_bytes(300000, 60), 625000u);
  // Below the floor the fallback bitrate is used: 150 Mbps at 60 fps.
  EXPECT_EQ(pw::effective_bitrate_kbps(20000), pw::fallback_bitrate_kbps);
  EXPECT_EQ(pw::max_frame_bytes(20000, 60), 312500u);
  // Absurd framerates still leave a usable per-frame budget.
  EXPECT_EQ(pw::max_frame_bytes(100000, 100000), pw::min_frame_bytes);
  // Zero/negative framerate is treated as 1 fps rather than dividing by zero.
  EXPECT_EQ(pw::max_frame_bytes(100000, 0), 12500000u);
}
