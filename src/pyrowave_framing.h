/**
 * @file src/pyrowave_framing.h
 * @brief Frame payload layout for the experimental PyroWave codec.
 *
 * A PyroWave frame is a set of independently decodable packets. The client
 * side (the PyroWave-aware moonlight-common-c + decoder) expects them
 * concatenated into one video frame with this little-endian header:
 *
 *   u32 magic ('PYW1')
 *   u32 packet_count
 *   u32 packet_size[packet_count]
 *   u8  packet_data[...]      (packets back to back, in order)
 *
 * Kept free of any PyroWave/Vulkan dependency so it builds and is tested
 * whether or not SUNSHINE_ENABLE_PYROWAVE is set.
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <optional>
#include <span>
#include <vector>

namespace video::pyrowave {
  constexpr std::uint32_t payload_magic = 0x31575950u;  // 'PYW1'

  /// Upper bound on packets per frame; anything above this is a codec bug.
  constexpr std::size_t max_packets_per_frame = 4096;

  struct packet_ref_t {
    std::size_t offset;
    std::size_t size;
  };

  /**
   * @brief Build the framed payload from packetized bitstream output.
   * @param bitstream Buffer the encoder packetized into.
   * @param packets Offset/size of each packet inside @p bitstream.
   * @return The framed payload, or nullopt if the packet list is empty, too
   *         long, or references bytes outside @p bitstream.
   */
  inline std::optional<std::vector<std::uint8_t>> frame_packets(std::span<const std::uint8_t> bitstream, std::span<const packet_ref_t> packets) {
    if (packets.empty() || packets.size() > max_packets_per_frame) {
      return std::nullopt;
    }

    std::size_t payload_bytes = 0;
    for (const auto &packet : packets) {
      if (packet.size > std::numeric_limits<std::uint32_t>::max() ||
          packet.offset > bitstream.size() ||
          packet.size > bitstream.size() - packet.offset) {
        return std::nullopt;
      }
      payload_bytes += packet.size;
    }

    std::vector<std::uint8_t> framed(8 + packets.size() * 4 + payload_bytes);
    auto *out = framed.data();
    auto write_u32 = [&out](std::uint32_t value) {
      out[0] = static_cast<std::uint8_t>(value);
      out[1] = static_cast<std::uint8_t>(value >> 8);
      out[2] = static_cast<std::uint8_t>(value >> 16);
      out[3] = static_cast<std::uint8_t>(value >> 24);
      out += 4;
    };

    write_u32(payload_magic);
    write_u32(static_cast<std::uint32_t>(packets.size()));
    for (const auto &packet : packets) {
      write_u32(static_cast<std::uint32_t>(packet.size));
    }
    for (const auto &packet : packets) {
      if (packet.size) {
        std::memcpy(out, bitstream.data() + packet.offset, packet.size);
        out += packet.size;
      }
    }

    return framed;
  }

  /**
   * @brief Per-frame bitstream cap derived from the stream bitrate.
   *
   * PyroWave has no rate control beyond a hard per-frame size limit, so the
   * client's bitrate is turned into bytes per frame. Below ~100 Mbps the codec
   * falls apart visually, so lower requests are raised to 150 Mbps (matching
   * the reference Sunshine fork this path was ported from).
   */
  constexpr int min_bitrate_kbps = 100000;
  constexpr int fallback_bitrate_kbps = 150000;
  constexpr std::size_t min_frame_bytes = 64 * 1024;

  constexpr int effective_bitrate_kbps(int requested_kbps) {
    return requested_kbps < min_bitrate_kbps ? fallback_bitrate_kbps : requested_kbps;
  }

  constexpr std::size_t max_frame_bytes(int requested_kbps, int framerate) {
    const auto fps = static_cast<std::size_t>(framerate > 0 ? framerate : 1);
    const auto bytes = static_cast<std::size_t>(effective_bitrate_kbps(requested_kbps)) * 1000u / 8u / fps;
    return bytes < min_frame_bytes ? min_frame_bytes : bytes;
  }
}  // namespace video::pyrowave
