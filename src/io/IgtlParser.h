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
constexpr std::size_t kIgtlTdataElementSize = 70;   // name[20] + type + reserved + 12 floats
constexpr std::size_t kIgtlTrajectoryElementSize = 150;  // OpenIGTLink TRAJ element

// OpenIGTLink TDATA instrument type: 6D / regular instrument.
constexpr std::uint8_t kIgtlTdataType6D = 2;
// OpenIGTLink trajectory type: entry + target.
constexpr std::uint8_t kIgtlTrajectoryTypeEntryTarget = 3;

struct IgtlHeader {
  std::uint16_t version = 1;
  char type[13]{};        // NUL-terminated copy of the 12-byte type field
  char deviceName[21]{};  // NUL-terminated copy of the 20-byte device field
  std::uint64_t timeStamp = 0;
  std::uint64_t bodySize = 0;
  std::uint64_t crc = 0;
};

struct Vec3 {
  float x = 0.f;
  float y = 0.f;
  float z = 0.f;
};

// OpenIGTLink framing/body helpers. CRC64-ECMA over the body is verified on
// parse and filled on pack. Trajectory messages use the OpenIGTLink wire type
// "TRAJ"; docs call these TRAJECTORY messages.
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

  // One-tool TDATA element → Mat4 (matrix portion matches TRANSFORM).
  static bool parseTdataBodyOneTool(const std::uint8_t* body,
                                    std::size_t size,
                                    Mat4& out,
                                    char* toolNameOut = nullptr,
                                    std::size_t toolNameCap = 0,
                                    std::string* error = nullptr);

  static bool parseTdataMessage(const std::uint8_t* data,
                                std::size_t size,
                                Mat4& out,
                                IgtlHeader* headerOut = nullptr,
                                char* toolNameOut = nullptr,
                                std::size_t toolNameCap = 0,
                                std::string* error = nullptr);

  static bool packTdataMessage(const Mat4& toolToTracker,
                               const char* toolName,
                               const char* deviceName,
                               std::uint64_t timeStamp,
                               std::vector<std::uint8_t>& out,
                               std::string* error = nullptr);

  // One TRAJ element with type entry+target → entry and target points.
  static bool parseTrajectoryBody(const std::uint8_t* body,
                                  std::size_t size,
                                  Vec3& entryOut,
                                  Vec3& targetOut,
                                  std::string* error = nullptr);

  static bool parseTrajectoryMessage(const std::uint8_t* data,
                                     std::size_t size,
                                     Vec3& entryOut,
                                     Vec3& targetOut,
                                     IgtlHeader* headerOut = nullptr,
                                     std::string* error = nullptr);

  static bool packTrajectoryMessage(const Vec3& entry,
                                    const Vec3& target,
                                    const char* deviceName,
                                    std::uint64_t timeStamp,
                                    std::vector<std::uint8_t>& out,
                                    std::string* error = nullptr);
};

// Accumulates partial TCP reads and yields complete OpenIGTLink messages
// (58-byte header + bodySize bytes). No sockets — caller feeds bytes.
class IgtlStreamReassembler {
public:
  // Reject headers that claim a body larger than this (corrupt / hostile stream).
  static constexpr std::uint64_t kMaxBodySize = 1024ULL * 1024ULL;

  void append(const std::uint8_t* data, std::size_t size);
  void clear();
  std::size_t bufferedBytes() const { return this->buffer_.size(); }

  // true  → messageOut is one complete framed message (header+body).
  // false → need more bytes (error empty), or framing error (error set; buffer cleared).
  bool tryExtractMessage(std::vector<std::uint8_t>& messageOut,
                         std::string* error = nullptr);

private:
  std::vector<std::uint8_t> buffer_;
};

}  // namespace nnc
