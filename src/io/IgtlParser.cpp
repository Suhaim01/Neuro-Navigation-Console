#include "io/IgtlParser.h"

#include <algorithm>
#include <cstring>

namespace nnc {
namespace igtl_detail {

// CRC64-ECMA table from OpenIGTLink igtl_util.c (public protocol reference).
const std::uint64_t kCrc64Table[256] = {
    0x0000000000000000ULL, 0x42F0E1EBA9EA3693ULL, 0x85E1C3D753D46D26ULL,
    0xC711223CFA3E5BB5ULL, 0x493366450E42ECDFULL, 0x0BC387AEA7A8DA4CULL,
    0xCCD2A5925D9681F9ULL, 0x8E224479F47CB76AULL, 0x9266CC8A1C85D9BEULL,
    0xD0962D61B56FEF2DULL, 0x17870F5D4F51B498ULL, 0x5577EEB6E6BB820BULL,
    0xDB55AACF12C73561ULL, 0x99A54B24BB2D03F2ULL, 0x5EB4691841135847ULL,
    0x1C4488F3E8F96ED4ULL, 0x663D78FF90E185EFULL, 0x24CD9914390BB37CULL,
    0xE3DCBB28C335E8C9ULL, 0xA12C5AC36ADFDE5AULL, 0x2F0E1EBA9EA36930ULL,
    0x6DFEFF5137495FA3ULL, 0xAAEFDD6DCD770416ULL, 0xE81F3C86649D3285ULL,
    0xF45BB4758C645C51ULL, 0xB6AB559E258E6AC2ULL, 0x71BA77A2DFB03177ULL,
    0x334A9649765A07E4ULL, 0xBD68D2308226B08EULL, 0xFF9833DB2BCC861DULL,
    0x388911E7D1F2DDA8ULL, 0x7A79F00C7818EB3BULL, 0xCC7AF1FF21C30BDEULL,
    0x8E8A101488293D4DULL, 0x499B3228721766F8ULL, 0x0B6BD3C3DBFD506BULL,
    0x854997BA2F81E701ULL, 0xC7B97651866BD192ULL, 0x00A8546D7C558A27ULL,
    0x4258B586D5BFBCB4ULL, 0x5E1C3D753D46D260ULL, 0x1CECDC9E94ACE4F3ULL,
    0xDBFDFEA26E92BF46ULL, 0x990D1F49C77889D5ULL, 0x172F5B3033043EBFULL,
    0x55DFBADB9AEE082CULL, 0x92CE98E760D05399ULL, 0xD03E790CC93A650AULL,
    0xAA478900B1228E31ULL, 0xE8B768EB18C8B8A2ULL, 0x2FA64AD7E2F6E317ULL,
    0x6D56AB3C4B1CD584ULL, 0xE374EF45BF6062EEULL, 0xA1840EAE168A547DULL,
    0x66952C92ECB40FC8ULL, 0x2465CD79455E395BULL, 0x3821458AADA7578FULL,
    0x7AD1A461044D611CULL, 0xBDC0865DFE733AA9ULL, 0xFF3067B657990C3AULL,
    0x711223CFA3E5BB50ULL, 0x33E2C2240A0F8DC3ULL, 0xF4F3E018F031D676ULL,
    0xB60301F359DBE0E5ULL, 0xDA050215EA6C212FULL, 0x98F5E3FE438617BCULL,
    0x5FE4C1C2B9B84C09ULL, 0x1D14202910527A9AULL, 0x93366450E42ECDF0ULL,
    0xD1C685BB4DC4FB63ULL, 0x16D7A787B7FAA0D6ULL, 0x5427466C1E109645ULL,
    0x4863CE9FF6E9F891ULL, 0x0A932F745F03CE02ULL, 0xCD820D48A53D95B7ULL,
    0x8F72ECA30CD7A324ULL, 0x0150A8DAF8AB144EULL, 0x43A04931514122DDULL,
    0x84B16B0DAB7F7968ULL, 0xC6418AE602954FFBULL, 0xBC387AEA7A8DA4C0ULL,
    0xFEC89B01D3679253ULL, 0x39D9B93D2959C9E6ULL, 0x7B2958D680B3FF75ULL,
    0xF50B1CAF74CF481FULL, 0xB7FBFD44DD257E8CULL, 0x70EADF78271B2539ULL,
    0x321A3E938EF113AAULL, 0x2E5EB66066087D7EULL, 0x6CAE578BCFE24BEDULL,
    0xABBF75B735DC1058ULL, 0xE94F945C9C3626CBULL, 0x676DD025684A91A1ULL,
    0x259D31CEC1A0A732ULL, 0xE28C13F23B9EFC87ULL, 0xA07CF2199274CA14ULL,
    0x167FF3EACBAF2AF1ULL, 0x548F120162451C62ULL, 0x939E303D987B47D7ULL,
    0xD16ED1D631917144ULL, 0x5F4C95AFC5EDC62EULL, 0x1DBC74446C07F0BDULL,
    0xDAAD56789639AB08ULL, 0x985DB7933FD39D9BULL, 0x84193F60D72AF34FULL,
    0xC6E9DE8B7EC0C5DCULL, 0x01F8FCB784FE9E69ULL, 0x43081D5C2D14A8FAULL,
    0xCD2A5925D9681F90ULL, 0x8FDAB8CE70822903ULL, 0x48CB9AF28ABC72B6ULL,
    0x0A3B7B1923564425ULL, 0x70428B155B4EAF1EULL, 0x32B26AFEF2A4998DULL,
    0xF5A348C2089AC238ULL, 0xB753A929A170F4ABULL, 0x3971ED50550C43C1ULL,
    0x7B810CBBFCE67552ULL, 0xBC902E8706D82EE7ULL, 0xFE60CF6CAF321874ULL,
    0xE224479F47CB76A0ULL, 0xA0D4A674EE214033ULL, 0x67C58448141F1B86ULL,
    0x253565A3BDF52D15ULL, 0xAB1721DA49899A7FULL, 0xE9E7C031E063ACECULL,
    0x2EF6E20D1A5DF759ULL, 0x6C0603E6B3B7C1CAULL, 0xF6FAE5C07D3274CDULL,
    0xB40A042BD4D8425EULL, 0x731B26172EE619EBULL, 0x31EBC7FC870C2F78ULL,
    0xBFC9838573709812ULL, 0xFD39626EDA9AAE81ULL, 0x3A28405220A4F534ULL,
    0x78D8A1B9894EC3A7ULL, 0x649C294A61B7AD73ULL, 0x266CC8A1C85D9BE0ULL,
    0xE17DEA9D3263C055ULL, 0xA38D0B769B89F6C6ULL, 0x2DAF4F0F6FF541ACULL,
    0x6F5FAEE4C61F773FULL, 0xA84E8CD83C212C8AULL, 0xEABE6D3395CB1A19ULL,
    0x90C79D3FEDD3F122ULL, 0xD2377CD44439C7B1ULL, 0x15265EE8BE079C04ULL,
    0x57D6BF0317EDAA97ULL, 0xD9F4FB7AE3911DFDULL, 0x9B041A914A7B2B6EULL,
    0x5C1538ADB04570DBULL, 0x1EE5D94619AF4648ULL, 0x02A151B5F156289CULL,
    0x4051B05E58BC1E0FULL, 0x87409262A28245BAULL, 0xC5B073890B687329ULL,
    0x4B9237F0FF14C443ULL, 0x0962D61B56FEF2D0ULL, 0xCE73F427ACC0A965ULL,
    0x8C8315CC052A9FF6ULL, 0x3A80143F5CF17F13ULL, 0x7870F5D4F51B4980ULL,
    0xBF61D7E80F251235ULL, 0xFD913603A6CF24A6ULL, 0x73B3727A52B393CCULL,
    0x31439391FB59A55FULL, 0xF652B1AD0167FEEAULL, 0xB4A25046A88DC879ULL,
    0xA8E6D8B54074A6ADULL, 0xEA16395EE99E903EULL, 0x2D071B6213A0CB8BULL,
    0x6FF7FA89BA4AFD18ULL, 0xE1D5BEF04E364A72ULL, 0xA3255F1BE7DC7CE1ULL,
    0x64347D271DE22754ULL, 0x26C49CCCB40811C7ULL, 0x5CBD6CC0CC10FAFCULL,
    0x1E4D8D2B65FACC6FULL, 0xD95CAF179FC497DAULL, 0x9BAC4EFC362EA149ULL,
    0x158E0A85C2521623ULL, 0x577EEB6E6BB820B0ULL, 0x906FC95291867B05ULL,
    0xD29F28B9386C4D96ULL, 0xCEDBA04AD0952342ULL, 0x8C2B41A1797F15D1ULL,
    0x4B3A639D83414E64ULL, 0x09CA82762AAB78F7ULL, 0x87E8C60FDED7CF9DULL,
    0xC51827E4773DF90EULL, 0x020905D88D03A2BBULL, 0x40F9E43324E99428ULL,
    0x2CFFE7D5975E55E2ULL, 0x6E0F063E3EB46371ULL, 0xA91E2402C48A38C4ULL,
    0xEBEEC5E96D600E57ULL, 0x65CC8190991CB93DULL, 0x273C607B30F68FAEULL,
    0xE02D4247CAC8D41BULL, 0xA2DDA3AC6322E288ULL, 0xBE992B5F8BDB8C5CULL,
    0xFC69CAB42231BACFULL, 0x3B78E888D80FE17AULL, 0x7988096371E5D7E9ULL,
    0xF7AA4D1A85996083ULL, 0xB55AACF12C735610ULL, 0x724B8ECDD64D0DA5ULL,
    0x30BB6F267FA73B36ULL, 0x4AC29F2A07BFD00DULL, 0x08327EC1AE55E69EULL,
    0xCF235CFD546BBD2BULL, 0x8DD3BD16FD818BB8ULL, 0x03F1F96F09FD3CD2ULL,
    0x41011884A0170A41ULL, 0x86103AB85A2951F4ULL, 0xC4E0DB53F3C36767ULL,
    0xD8A453A01B3A09B3ULL, 0x9A54B24BB2D03F20ULL, 0x5D45907748EE6495ULL,
    0x1FB5719CE1045206ULL, 0x919735E51578E56CULL, 0xD367D40EBC92D3FFULL,
    0x1476F63246AC884AULL, 0x568617D9EF46BED9ULL, 0xE085162AB69D5E3CULL,
    0xA275F7C11F7768AFULL, 0x6564D5FDE549331AULL, 0x279434164CA30589ULL,
    0xA9B6706FB8DFB2E3ULL, 0xEB46918411358470ULL, 0x2C57B3B8EB0BDFC5ULL,
    0x6EA7525342E1E956ULL, 0x72E3DAA0AA188782ULL, 0x30133B4B03F2B111ULL,
    0xF7021977F9CCEAA4ULL, 0xB5F2F89C5026DC37ULL, 0x3BD0BCE5A45A6B5DULL,
    0x79205D0E0DB05DCEULL, 0xBE317F32F78E067BULL, 0xFCC19ED95E6430E8ULL,
    0x86B86ED5267CDBD3ULL, 0xC4488F3E8F96ED40ULL, 0x0359AD0275A8B6F5ULL,
    0x41A94CE9DC428066ULL, 0xCF8B0890283E370CULL, 0x8D7BE97B81D4019FULL,
    0x4A6ACB477BEA5A2AULL, 0x089A2AACD2006CB9ULL, 0x14DEA25F3AF9026DULL,
    0x562E43B4931334FEULL, 0x913F6188692D6F4BULL, 0xD3CF8063C0C759D8ULL,
    0x5DEDC41A34BBEEB2ULL, 0x1F1D25F19D51D821ULL, 0xD80C07CD676F8394ULL,
    0x9AFCE626CE85B507ULL,
};

bool hostIsLittleEndian()
{
  const std::uint16_t one = 1;
  return *reinterpret_cast<const std::uint8_t*>(&one) == 1;
}

std::uint16_t readBe16(const std::uint8_t* p)
{
  return static_cast<std::uint16_t>((static_cast<std::uint16_t>(p[0]) << 8) |
                                    static_cast<std::uint16_t>(p[1]));
}

std::uint64_t readBe64(const std::uint8_t* p)
{
  std::uint64_t v = 0;
  for (int i = 0; i < 8; ++i) {
    v = (v << 8) | static_cast<std::uint64_t>(p[i]);
  }
  return v;
}

void writeBe16(std::uint8_t* p, std::uint16_t v)
{
  p[0] = static_cast<std::uint8_t>((v >> 8) & 0xff);
  p[1] = static_cast<std::uint8_t>(v & 0xff);
}

void writeBe64(std::uint8_t* p, std::uint64_t v)
{
  for (int i = 7; i >= 0; --i) {
    p[i] = static_cast<std::uint8_t>(v & 0xff);
    v >>= 8;
  }
}

float readBeFloat(const std::uint8_t* p)
{
  std::uint8_t bytes[4];
  if (nnc::igtl_detail::hostIsLittleEndian()) {
    bytes[0] = p[3];
    bytes[1] = p[2];
    bytes[2] = p[1];
    bytes[3] = p[0];
  } else {
    std::memcpy(bytes, p, 4);
  }
  float out = 0.f;
  std::memcpy(&out, bytes, 4);
  return out;
}

void writeBeFloat(std::uint8_t* p, float v)
{
  std::uint8_t bytes[4];
  std::memcpy(bytes, &v, 4);
  if (nnc::igtl_detail::hostIsLittleEndian()) {
    p[0] = bytes[3];
    p[1] = bytes[2];
    p[2] = bytes[1];
    p[3] = bytes[0];
  } else {
    std::memcpy(p, bytes, 4);
  }
}

std::uint64_t crc64(const std::uint8_t* data, std::size_t len, std::uint64_t crc)
{
  const std::uint8_t* p = data;
  std::size_t remaining = len;
  while (remaining > 0) {
    const std::uint8_t idx =
        static_cast<std::uint8_t>(*p ^ static_cast<std::uint8_t>(crc >> 56));
    crc = nnc::igtl_detail::kCrc64Table[idx] ^ (crc << 8);
    ++p;
    --remaining;
  }
  return crc;
}

void copyPaddedField(char* dest, std::size_t destCap, const std::uint8_t* src, std::size_t srcLen)
{
  const std::size_t n = std::min(srcLen, destCap - 1);
  std::memcpy(dest, src, n);
  dest[n] = '\0';
  // Trim trailing NULs already handled; also stop at first embedded NUL via strnlen path:
  for (std::size_t i = 0; i < n; ++i) {
    if (dest[i] == '\0') {
      break;
    }
  }
}

bool typeEquals(const std::uint8_t* typeField, const char* expected)
{
  std::uint8_t padded[12];
  std::memset(padded, 0, sizeof(padded));
  const std::size_t n = std::min(std::strlen(expected), sizeof(padded));
  std::memcpy(padded, expected, n);
  return std::memcmp(typeField, padded, 12) == 0;
}

void setError(std::string* error, const char* message)
{
  if (error) {
    *error = message;
  }
}

void writePaddedField(std::uint8_t* dest, std::size_t destLen, const char* src)
{
  std::memset(dest, 0, destLen);
  if (src == nullptr) {
    return;
  }
  const std::size_t n = std::min(std::strlen(src), destLen);
  std::memcpy(dest, src, n);
}

void writeTransform12(std::uint8_t* dest, const Mat4& m)
{
  nnc::igtl_detail::writeBeFloat(dest + 0, m(0, 0));
  nnc::igtl_detail::writeBeFloat(dest + 4, m(1, 0));
  nnc::igtl_detail::writeBeFloat(dest + 8, m(2, 0));
  nnc::igtl_detail::writeBeFloat(dest + 12, m(0, 1));
  nnc::igtl_detail::writeBeFloat(dest + 16, m(1, 1));
  nnc::igtl_detail::writeBeFloat(dest + 20, m(2, 1));
  nnc::igtl_detail::writeBeFloat(dest + 24, m(0, 2));
  nnc::igtl_detail::writeBeFloat(dest + 28, m(1, 2));
  nnc::igtl_detail::writeBeFloat(dest + 32, m(2, 2));
  nnc::igtl_detail::writeBeFloat(dest + 36, m(0, 3));
  nnc::igtl_detail::writeBeFloat(dest + 40, m(1, 3));
  nnc::igtl_detail::writeBeFloat(dest + 44, m(2, 3));
}

void writeHeader(std::uint8_t* out,
                 const char* type,
                 const char* deviceName,
                 std::uint64_t timeStamp,
                 std::uint64_t bodySize)
{
  nnc::igtl_detail::writeBe16(out + 0, 1);
  nnc::igtl_detail::writePaddedField(out + 2, 12, type);
  nnc::igtl_detail::writePaddedField(out + 14, 20, deviceName);
  nnc::igtl_detail::writeBe64(out + 34, timeStamp);
  nnc::igtl_detail::writeBe64(out + 42, bodySize);
}

bool finishMessageParse(const std::uint8_t* data,
                        std::size_t size,
                        const char* expectedType,
                        std::uint64_t expectedBodySize,
                        IgtlHeader& header,
                        const std::uint8_t*& bodyOut,
                        std::string* error,
                        const char* typeMismatchMsg,
                        const char* sizeMismatchMsg,
                        const char* crcMismatchMsg)
{
  if (!nnc::IgtlParser::parseHeader(data, size, header, error)) {
    return false;
  }
  if (!nnc::igtl_detail::typeEquals(data + 2, expectedType)) {
    nnc::igtl_detail::setError(error, typeMismatchMsg);
    return false;
  }
  if (header.bodySize != expectedBodySize) {
    nnc::igtl_detail::setError(error, sizeMismatchMsg);
    return false;
  }
  if (size < nnc::kIgtlHeaderSize + header.bodySize) {
    nnc::igtl_detail::setError(error, "message shorter than header + body");
    return false;
  }
  bodyOut = data + nnc::kIgtlHeaderSize;
  if (!nnc::IgtlParser::verifyBodyCrc(bodyOut, static_cast<std::size_t>(header.bodySize),
                                 header.crc)) {
    nnc::igtl_detail::setError(error, crcMismatchMsg);
    return false;
  }
  return true;
}

}  // namespace igtl_detail

bool IgtlParser::parseHeader(const std::uint8_t* data,
                             std::size_t size,
                             IgtlHeader& out,
                             std::string* error)
{
  if (data == nullptr || size < nnc::kIgtlHeaderSize) {
    nnc::igtl_detail::setError(error, "header buffer shorter than 58 bytes");
    return false;
  }

  out.version = nnc::igtl_detail::readBe16(data + 0);
  nnc::igtl_detail::copyPaddedField(out.type, sizeof(out.type), data + 2, 12);
  nnc::igtl_detail::copyPaddedField(out.deviceName, sizeof(out.deviceName), data + 14, 20);
  out.timeStamp = nnc::igtl_detail::readBe64(data + 34);
  out.bodySize = nnc::igtl_detail::readBe64(data + 42);
  out.crc = nnc::igtl_detail::readBe64(data + 50);
  return true;
}

bool IgtlParser::verifyBodyCrc(const std::uint8_t* body,
                               std::size_t bodySize,
                               std::uint64_t expectedCrc)
{
  if (body == nullptr && bodySize > 0) {
    return false;
  }
  const std::uint64_t actual =
      nnc::igtl_detail::crc64(body, bodySize, 0ULL);
  return actual == expectedCrc;
}

bool IgtlParser::parseTransformBody(const std::uint8_t* body,
                                   std::size_t size,
                                   Mat4& out,
                                   std::string* error)
{
  if (body == nullptr || size < nnc::kIgtlTransformBodySize) {
    nnc::igtl_detail::setError(error, "TRANSFORM body shorter than 48 bytes");
    return false;
  }

  // OpenIGTLink order: R11,R21,R31, R12,R22,R32, R13,R23,R33, TX,TY,TZ
  const float r11 = nnc::igtl_detail::readBeFloat(body + 0);
  const float r21 = nnc::igtl_detail::readBeFloat(body + 4);
  const float r31 = nnc::igtl_detail::readBeFloat(body + 8);
  const float r12 = nnc::igtl_detail::readBeFloat(body + 12);
  const float r22 = nnc::igtl_detail::readBeFloat(body + 16);
  const float r32 = nnc::igtl_detail::readBeFloat(body + 20);
  const float r13 = nnc::igtl_detail::readBeFloat(body + 24);
  const float r23 = nnc::igtl_detail::readBeFloat(body + 28);
  const float r33 = nnc::igtl_detail::readBeFloat(body + 32);
  const float tx = nnc::igtl_detail::readBeFloat(body + 36);
  const float ty = nnc::igtl_detail::readBeFloat(body + 40);
  const float tz = nnc::igtl_detail::readBeFloat(body + 44);

  out = nnc::Mat4::identity();
  out(0, 0) = r11;
  out(1, 0) = r21;
  out(2, 0) = r31;
  out(0, 1) = r12;
  out(1, 1) = r22;
  out(2, 1) = r32;
  out(0, 2) = r13;
  out(1, 2) = r23;
  out(2, 2) = r33;
  out(0, 3) = tx;
  out(1, 3) = ty;
  out(2, 3) = tz;
  return true;
}

bool IgtlParser::parseTransformMessage(const std::uint8_t* data,
                                      std::size_t size,
                                      Mat4& out,
                                      IgtlHeader* headerOut,
                                      std::string* error)
{
  nnc::IgtlHeader header;
  const std::uint8_t* body = nullptr;
  if (!nnc::igtl_detail::finishMessageParse(
          data, size, "TRANSFORM", nnc::kIgtlTransformBodySize, header, body, error,
          "message type is not TRANSFORM", "unexpected TRANSFORM body size",
          "TRANSFORM body CRC mismatch")) {
    return false;
  }
  if (!nnc::IgtlParser::parseTransformBody(body, static_cast<std::size_t>(header.bodySize),
                                         out, error)) {
    return false;
  }
  if (headerOut) {
    *headerOut = header;
  }
  return true;
}

bool IgtlParser::packTransformMessage(const Mat4& toolToTracker,
                                      const char* deviceName,
                                      std::uint64_t timeStamp,
                                      std::vector<std::uint8_t>& out,
                                      std::string* error)
{
  if (deviceName == nullptr) {
    nnc::igtl_detail::setError(error, "deviceName is null");
    return false;
  }

  out.assign(nnc::kIgtlHeaderSize + nnc::kIgtlTransformBodySize, 0);
  nnc::igtl_detail::writeHeader(out.data(), "TRANSFORM", deviceName, timeStamp,
                                nnc::kIgtlTransformBodySize);

  std::uint8_t* body = out.data() + nnc::kIgtlHeaderSize;
  nnc::igtl_detail::writeTransform12(body, toolToTracker);

  const std::uint64_t crc =
      nnc::igtl_detail::crc64(body, nnc::kIgtlTransformBodySize, 0ULL);
  nnc::igtl_detail::writeBe64(out.data() + 50, crc);
  return true;
}

bool IgtlParser::parseTdataBodyOneTool(const std::uint8_t* body,
                                       std::size_t size,
                                       Mat4& out,
                                       char* toolNameOut,
                                       std::size_t toolNameCap,
                                       std::string* error)
{
  if (body == nullptr || size < nnc::kIgtlTdataElementSize) {
    nnc::igtl_detail::setError(error, "TDATA body shorter than one 70-byte element");
    return false;
  }
  if (size != nnc::kIgtlTdataElementSize) {
    nnc::igtl_detail::setError(error, "TDATA body must contain exactly one tool element");
    return false;
  }

  if (toolNameOut != nullptr && toolNameCap > 0) {
    nnc::igtl_detail::copyPaddedField(toolNameOut, toolNameCap, body, 20);
  }

  // Matrix starts at offset 22 (name[20] + type + reserved).
  return nnc::IgtlParser::parseTransformBody(body + 22, size - 22, out, error);
}

bool IgtlParser::parseTdataMessage(const std::uint8_t* data,
                                   std::size_t size,
                                   Mat4& out,
                                   IgtlHeader* headerOut,
                                   char* toolNameOut,
                                   std::size_t toolNameCap,
                                   std::string* error)
{
  nnc::IgtlHeader header;
  const std::uint8_t* body = nullptr;
  if (!nnc::igtl_detail::finishMessageParse(
          data, size, "TDATA", nnc::kIgtlTdataElementSize, header, body, error,
          "message type is not TDATA", "unexpected TDATA body size (one tool required)",
          "TDATA body CRC mismatch")) {
    return false;
  }
  if (!nnc::IgtlParser::parseTdataBodyOneTool(body, static_cast<std::size_t>(header.bodySize),
                                             out, toolNameOut, toolNameCap, error)) {
    return false;
  }
  if (headerOut) {
    *headerOut = header;
  }
  return true;
}

bool IgtlParser::packTdataMessage(const Mat4& toolToTracker,
                                  const char* toolName,
                                  const char* deviceName,
                                  std::uint64_t timeStamp,
                                  std::vector<std::uint8_t>& out,
                                  std::string* error)
{
  if (toolName == nullptr) {
    nnc::igtl_detail::setError(error, "toolName is null");
    return false;
  }
  if (deviceName == nullptr) {
    nnc::igtl_detail::setError(error, "deviceName is null");
    return false;
  }

  out.assign(nnc::kIgtlHeaderSize + nnc::kIgtlTdataElementSize, 0);
  nnc::igtl_detail::writeHeader(out.data(), "TDATA", deviceName, timeStamp,
                                nnc::kIgtlTdataElementSize);

  std::uint8_t* body = out.data() + nnc::kIgtlHeaderSize;
  nnc::igtl_detail::writePaddedField(body, 20, toolName);
  body[20] = nnc::kIgtlTdataType6D;
  body[21] = 0;
  nnc::igtl_detail::writeTransform12(body + 22, toolToTracker);

  const std::uint64_t crc =
      nnc::igtl_detail::crc64(body, nnc::kIgtlTdataElementSize, 0ULL);
  nnc::igtl_detail::writeBe64(out.data() + 50, crc);
  return true;
}

bool IgtlParser::parseTrajectoryBody(const std::uint8_t* body,
                                     std::size_t size,
                                     Vec3& entryOut,
                                     Vec3& targetOut,
                                     std::string* error)
{
  if (body == nullptr || size < nnc::kIgtlTrajectoryElementSize) {
    nnc::igtl_detail::setError(error, "TRAJ body shorter than one 150-byte element");
    return false;
  }
  if (size != nnc::kIgtlTrajectoryElementSize) {
    nnc::igtl_detail::setError(error, "TRAJ body must contain exactly one element");
    return false;
  }

  // Layout: name[64], group[32], type, reserved, rgba[4], entry[3], target[3], ...
  const std::uint8_t trajType = body[96];
  if (trajType != nnc::kIgtlTrajectoryTypeEntryTarget) {
    nnc::igtl_detail::setError(error, "TRAJ type is not entry+target");
    return false;
  }

  const std::size_t entryOff = 102;  // 64 + 32 + 1 + 1 + 4
  entryOut.x = nnc::igtl_detail::readBeFloat(body + entryOff + 0);
  entryOut.y = nnc::igtl_detail::readBeFloat(body + entryOff + 4);
  entryOut.z = nnc::igtl_detail::readBeFloat(body + entryOff + 8);
  targetOut.x = nnc::igtl_detail::readBeFloat(body + entryOff + 12);
  targetOut.y = nnc::igtl_detail::readBeFloat(body + entryOff + 16);
  targetOut.z = nnc::igtl_detail::readBeFloat(body + entryOff + 20);
  return true;
}

bool IgtlParser::parseTrajectoryMessage(const std::uint8_t* data,
                                        std::size_t size,
                                        Vec3& entryOut,
                                        Vec3& targetOut,
                                        IgtlHeader* headerOut,
                                        std::string* error)
{
  nnc::IgtlHeader header;
  const std::uint8_t* body = nullptr;
  if (!nnc::igtl_detail::finishMessageParse(
          data, size, "TRAJ", nnc::kIgtlTrajectoryElementSize, header, body, error,
          "message type is not TRAJ",
          "unexpected TRAJ body size (one element required)",
          "TRAJ body CRC mismatch")) {
    return false;
  }
  if (!nnc::IgtlParser::parseTrajectoryBody(body, static_cast<std::size_t>(header.bodySize),
                                            entryOut, targetOut, error)) {
    return false;
  }
  if (headerOut) {
    *headerOut = header;
  }
  return true;
}

bool IgtlParser::packTrajectoryMessage(const Vec3& entry,
                                       const Vec3& target,
                                       const char* deviceName,
                                       std::uint64_t timeStamp,
                                       std::vector<std::uint8_t>& out,
                                       std::string* error)
{
  if (deviceName == nullptr) {
    nnc::igtl_detail::setError(error, "deviceName is null");
    return false;
  }

  out.assign(nnc::kIgtlHeaderSize + nnc::kIgtlTrajectoryElementSize, 0);
  nnc::igtl_detail::writeHeader(out.data(), "TRAJ", deviceName, timeStamp,
                                nnc::kIgtlTrajectoryElementSize);

  std::uint8_t* body = out.data() + nnc::kIgtlHeaderSize;
  nnc::igtl_detail::writePaddedField(body + 0, 64, "Plan");
  nnc::igtl_detail::writePaddedField(body + 64, 32, "Trajectory");
  body[96] = nnc::kIgtlTrajectoryTypeEntryTarget;
  body[97] = 0;
  body[98] = 255;  // R
  body[99] = 0;    // G
  body[100] = 0;   // B
  body[101] = 255; // A
  nnc::igtl_detail::writeBeFloat(body + 102, entry.x);
  nnc::igtl_detail::writeBeFloat(body + 106, entry.y);
  nnc::igtl_detail::writeBeFloat(body + 110, entry.z);
  nnc::igtl_detail::writeBeFloat(body + 114, target.x);
  nnc::igtl_detail::writeBeFloat(body + 118, target.y);
  nnc::igtl_detail::writeBeFloat(body + 122, target.z);
  nnc::igtl_detail::writeBeFloat(body + 126, 0.f);  // diameter / radius
  // owner_name[20] at 130 already zeroed

  const std::uint64_t crc =
      nnc::igtl_detail::crc64(body, nnc::kIgtlTrajectoryElementSize, 0ULL);
  nnc::igtl_detail::writeBe64(out.data() + 50, crc);
  return true;
}

}  // namespace nnc
