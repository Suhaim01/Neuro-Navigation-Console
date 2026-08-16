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

nnc::Mat4 samplePose()
{
  nnc::Mat4 sent = nnc::Mat4::identity();
  sent(0, 3) = 10.f;
  sent(1, 3) = 20.f;
  sent(2, 3) = 30.f;
  sent(0, 0) = 0.f;
  sent(0, 1) = -1.f;
  sent(1, 0) = 1.f;
  sent(1, 1) = 0.f;
  return sent;
}

void expectMatEqual(const nnc::Mat4& got, const nnc::Mat4& sent)
{
  for (int r = 0; r < 4; ++r) {
    for (int c = 0; c < 4; ++c) {
      QVERIFY2(nearlyEqual(got(r, c), sent(r, c)),
               qPrintable(QStringLiteral("mismatch at (%1,%2)").arg(r).arg(c)));
    }
  }
}

}  // namespace test_detail
}  // namespace nnc

class TstREQ_IGTL_TransformParse : public QObject {
  Q_OBJECT
private slots:
  void packThenParseRoundTrip();
  void rejectsBadCrc();
  void rejectsShortBuffer();
  void tdataPackThenParseRoundTrip();
  void trajectoryPackThenParseRoundTrip();
  void trajectoryRejectsWrongType();
};

void TstREQ_IGTL_TransformParse::packThenParseRoundTrip()
{
  const nnc::Mat4 sent = nnc::test_detail::samplePose();

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
  nnc::test_detail::expectMatEqual(got, sent);
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

void TstREQ_IGTL_TransformParse::tdataPackThenParseRoundTrip()
{
  const nnc::Mat4 sent = nnc::test_detail::samplePose();

  std::vector<std::uint8_t> bytes;
  std::string err;
  QVERIFY2(nnc::IgtlParser::packTdataMessage(sent, "Probe", "Tracker", 0, bytes, &err),
           err.c_str());
  QCOMPARE(static_cast<int>(bytes.size()),
           static_cast<int>(nnc::kIgtlHeaderSize + nnc::kIgtlTdataElementSize));

  nnc::IgtlHeader header;
  nnc::Mat4 got = nnc::Mat4::identity();
  char toolName[21] = {};
  QVERIFY2(nnc::IgtlParser::parseTdataMessage(bytes.data(), bytes.size(), got, &header, toolName,
                                              sizeof(toolName), &err),
           err.c_str());

  QCOMPARE(std::string(header.type), std::string("TDATA"));
  QCOMPARE(std::string(header.deviceName), std::string("Tracker"));
  QCOMPARE(std::string(toolName), std::string("Probe"));
  QCOMPARE(header.bodySize, static_cast<std::uint64_t>(nnc::kIgtlTdataElementSize));
  nnc::test_detail::expectMatEqual(got, sent);
}

void TstREQ_IGTL_TransformParse::trajectoryPackThenParseRoundTrip()
{
  const nnc::Vec3 entry{1.f, 2.f, 3.f};
  const nnc::Vec3 target{10.f, 20.f, 30.f};

  std::vector<std::uint8_t> bytes;
  std::string err;
  QVERIFY2(nnc::IgtlParser::packTrajectoryMessage(entry, target, "Plan", 0, bytes, &err),
           err.c_str());
  QCOMPARE(static_cast<int>(bytes.size()),
           static_cast<int>(nnc::kIgtlHeaderSize + nnc::kIgtlTrajectoryElementSize));

  nnc::IgtlHeader header;
  nnc::Vec3 gotEntry{};
  nnc::Vec3 gotTarget{};
  QVERIFY2(nnc::IgtlParser::parseTrajectoryMessage(bytes.data(), bytes.size(), gotEntry, gotTarget,
                                                   &header, &err),
           err.c_str());

  QCOMPARE(std::string(header.type), std::string("TRAJ"));
  QCOMPARE(header.bodySize, static_cast<std::uint64_t>(nnc::kIgtlTrajectoryElementSize));
  QVERIFY(nnc::test_detail::nearlyEqual(gotEntry.x, entry.x));
  QVERIFY(nnc::test_detail::nearlyEqual(gotEntry.y, entry.y));
  QVERIFY(nnc::test_detail::nearlyEqual(gotEntry.z, entry.z));
  QVERIFY(nnc::test_detail::nearlyEqual(gotTarget.x, target.x));
  QVERIFY(nnc::test_detail::nearlyEqual(gotTarget.y, target.y));
  QVERIFY(nnc::test_detail::nearlyEqual(gotTarget.z, target.z));
}

void TstREQ_IGTL_TransformParse::trajectoryRejectsWrongType()
{
  std::uint8_t body[nnc::kIgtlTrajectoryElementSize] = {};
  body[96] = 1;  // entry-only, not entry+target
  nnc::Vec3 entry{};
  nnc::Vec3 target{};
  std::string err;
  QVERIFY(!nnc::IgtlParser::parseTrajectoryBody(body, sizeof(body), entry, target, &err));
  QVERIFY(err.find("entry+target") != std::string::npos);
}

QTEST_APPLESS_MAIN(TstREQ_IGTL_TransformParse)
#include "tst_igtl_parse.moc"
