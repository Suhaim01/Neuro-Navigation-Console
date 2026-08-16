#include "io/IgtlParser.h"

#include <QObject>
#include <QtTest>

#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace nnc {
namespace test_detail {

bool nearlyEqual(float a, float b, float eps = 1e-5f)
{
  return std::fabs(a - b) <= eps;
}

}  // namespace test_detail
}  // namespace nnc

class TstREQ_IGTL_TransformParse : public QObject {
  Q_OBJECT
private slots:
  void packThenParseRoundTrip();
  void rejectsBadCrc();
  void rejectsShortBuffer();
};

void TstREQ_IGTL_TransformParse::packThenParseRoundTrip()
{
  nnc::Mat4 sent = nnc::Mat4::identity();
  // Tip-at-origin, shaft +Z: identity already; add a translation.
  sent(0, 3) = 10.f;
  sent(1, 3) = 20.f;
  sent(2, 3) = 30.f;
  // Mild rotation about Z (approx cos/sin 90°).
  sent(0, 0) = 0.f;
  sent(0, 1) = -1.f;
  sent(1, 0) = 1.f;
  sent(1, 1) = 0.f;

  std::vector<std::uint8_t> bytes;
  std::string err;
  QVERIFY2(nnc::IgtlParser::packTransformMessage(sent, "Tool", 0, bytes, &err), err.c_str());
  QCOMPARE(static_cast<int>(bytes.size()),
           static_cast<int>(nnc::kIgtlHeaderSize + nnc::kIgtlTransformBodySize));

  nnc::IgtlHeader header;
  nnc::Mat4 got = nnc::Mat4::identity();
  QVERIFY2(nnc::IgtlParser::parseTransformMessage(bytes.data(), bytes.size(), got, &header, &err),
           err.c_str());

  QCOMPARE(std::string(header.type), std::string("TRANSFORM"));
  QCOMPARE(std::string(header.deviceName), std::string("Tool"));
  QCOMPARE(header.bodySize, static_cast<std::uint64_t>(nnc::kIgtlTransformBodySize));

  for (int r = 0; r < 4; ++r) {
    for (int c = 0; c < 4; ++c) {
      QVERIFY2(nnc::test_detail::nearlyEqual(got(r, c), sent(r, c)),
               qPrintable(QStringLiteral("mismatch at (%1,%2)").arg(r).arg(c)));
    }
  }
}

void TstREQ_IGTL_TransformParse::rejectsBadCrc()
{
  nnc::Mat4 sent = nnc::Mat4::identity();
  sent(0, 3) = 1.f;

  std::vector<std::uint8_t> bytes;
  std::string err;
  QVERIFY(nnc::IgtlParser::packTransformMessage(sent, "Tool", 0, bytes, &err));

  // Flip one body byte after CRC was computed.
  bytes[nnc::kIgtlHeaderSize + 0] ^= 0xff;

  nnc::Mat4 got;
  QVERIFY(!nnc::IgtlParser::parseTransformMessage(bytes.data(), bytes.size(), got, nullptr, &err));
  QVERIFY(err.find("CRC") != std::string::npos);
}

void TstREQ_IGTL_TransformParse::rejectsShortBuffer()
{
  std::uint8_t tiny[10] = {};
  nnc::Mat4 got;
  std::string err;
  QVERIFY(!nnc::IgtlParser::parseTransformMessage(tiny, sizeof(tiny), got, nullptr, &err));
}

QTEST_APPLESS_MAIN(TstREQ_IGTL_TransformParse)
#include "tst_igtl_parse.moc"
