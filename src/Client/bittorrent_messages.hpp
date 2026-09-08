#ifndef BITTORRENT_MESSAGES
#define BITTORRENT_MESSAGES

#include "io_ring_buffer.hpp"
#include <algorithm>
#include <array>
#include <span>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace bittorrent_messages {

  namespace length {
    constexpr std::size_t handshake = 68;
  }

  namespace header {
    constexpr std::array<std::byte, 0> keepalive;
    constexpr std::array<std::byte, 0> choke;
    constexpr std::array<std::byte, 0> unchoke;
    constexpr std::array<std::byte, 0> interested;
    constexpr std::array<std::byte, 0> not_interested;
    constexpr std::array<std::byte, 0> have;
    constexpr std::array<std::byte, 0> request;
    constexpr std::array<std::byte, 0> cancel;
  };

  using handshake_t = std::array<std::byte, length::handshake>;
  inline constexpr std::uint8_t protocol_string_length = 19;
  inline constexpr std::array<char, 19> protocol_string {'B','i','t','T','o','r','r','e','n','t',' ','p','r','o','t','o','c','o','l'};

  struct encode_result {
    bool complete;
  };

  struct decode_result {
    bool complete;
    bool valid;
  };

  // frame_cursor helps to make encoding for upload easier not useful for recving since recieved message
  // type of message cannot be easily inferred from the onset of transaction without storing a lot of states
  // unike uploads where the caller should now what he should be sening
  // .cursor tells where in the current message did the previous encode stop for later resumption
  // .reset() enables the cursor to be reusable for new messages to be encoded as frames.
  struct frame_cursor {
    std::size_t cursor {0};
    void reset() { cursor = 0; }
  };

namespace handshake {

  // encoder and decoder now does not care if buffer full or empty
  // only operates with returned iovecs.
  // this functions also trust that offset is valid in the context of
  // handshakes size, so between 0 to 68

  template <std::size_t size> encode_result encode( const handshake_t& handshake, io_ring_buffer<size>& buffer, frame_cursor& frame ) {

    encode_result encode_resolve {.complete = false };
    std::size_t encode_cursor = frame.cursor;

    if (encode_cursor >= length::handshake) {
      encode_resolve.complete = true;
      return encode_resolve;
    }

    prepare_t prepare = buffer.prepare_write();
    if (prepare.empty())
      return encode_resolve;

    for (std::size_t i = 0; i < prepare.prepared_iovecs; ++i) {
      auto& io_path = prepare.iovec_array[i];
      if (io_path.iov_len == 0)
        break;

      std::size_t io_length = std::min(length::handshake - encode_cursor, io_path.iov_len);
      std::memcpy(io_path.iov_base ,&handshake[encode_cursor], io_length);
      encode_cursor += io_length;
      buffer.commit_write(io_length);

      if (encode_cursor == length::handshake) {
        encode_resolve.complete = true;
        break;
      }
    }

    frame.cursor = encode_cursor;
    return encode_resolve;
  }

  template <std::size_t size> decode_result decode( io_ring_buffer<size>& buffer, std::span<const std::byte> info_hash ) {
    (void) buffer;
    return {false, false};
  }
}

namespace keep_alive {

}

}

#endif
