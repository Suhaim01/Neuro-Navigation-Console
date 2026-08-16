#pragma once

#include "io/NiftiLoader.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace nnc {

// OpenIGTLink v2-compatible header size (no extended header in our messages).
constexpr std::size_t kIgtlHeaderSize = 58;
constexpr std::size_t kIgtlTransformBodySize = 48;  // 12 × float32

struct IgtlHeader {
  std::uint16_t version = 1;
  char type[13]{};        // NUL-terminated copy of the 12-byte type field
  char deviceName[21]{};  // NUL-terminated copy of the 20-byte device field
  std::uint64_t timeStamp = 0;
  std::uint64_t bodySize = 0;
  std::uint64_t crc = 0;
};

// OpenIGTLink framing/body helpers for TRANSFORM (first slice of Task 1).
// CRC64-ECMA over the body is verified on parse and filled on pack.
class IgtlParser {
public:
  static bool parseHeader(const std::uint8_t* data,
                           std::size_t size,
                           IgtlHeader& out,
                           std::string* error = nullptr);

  static bool verifyBodyCrc(const std::uint8_t* body,
                             std::size_t bodySize,
                             std::uint64_t expectedCrc);

  static bool parseTransformBody(const std::uint8_t* body,
                                 std::size_t size,
                                 Mat4& out,
                                 std::string* error = nullptr);

  // Expects a complete message: 58-byte header + TRANSFORM body. Verifies CRC.
  static bool parseTransformMessage(const std::uint8_t* data,
                                     std::size_t size,
                                     Mat4& out,
                                     IgtlHeader* headerOut = nullptr,
                                     std::string* error = nullptr);

  // Builds header + TRANSFORM body with a correct body CRC (for tests / navsim).
  static bool packTransformMessage(const Mat4& toolToTracker,
                                  const char* deviceName,
                                  std::uint64_t timeStamp,
                                  std::vector<std::uint8_t>& out,
                                  std::string* error = nullptr);
};

}  // namespace nnc
