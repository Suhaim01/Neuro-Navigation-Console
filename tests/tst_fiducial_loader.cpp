#include "reg/FiducialLoader.h"
#include "reg/PairedPointRegistration.h"

#include <QObject>
#include <QtTest>

#include <cmath>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

namespace nnc
{
namespace test_detail
{

std::string fixturePath(const char *name)
{
  const char *root = std::getenv("NNC_FIXTURES");
  if (root != nullptr && root[0] != '\0')
  {
    return std::string(root) + "/" + name;
  }
#ifdef NNC_FIXTURES_DIR
  return std::string(NNC_FIXTURES_DIR) + "/" + name;
#else
  return std::string("tests/fixtures/") + name;
#endif
}

nnc::Mat4 makeImageToTrackerGroundTruth()
{
  const float pitchRad = 30.f * 0.017453292519943295f;
  const float c = std::cos(pitchRad);
  const float s = std::sin(pitchRad);
  nnc::Mat4 m = nnc::Mat4::identity();
  m(0, 0) = c;
  m(0, 2) = s;
  m(2, 0) = -s;
  m(2, 2) = c;
  m(0, 3) = 40.f;
  m(1, 3) = -25.f;
  m(2, 3) = 15.f;
  return m;
}

bool nearlyEqual(float a, float b, float eps = 1e-3f)
{
  return std::fabs(a - b) <= eps;
}

void expectMat4Near(const nnc::Mat4 &got, const nnc::Mat4 &expected, float eps = 1e-3f)
{
  for (int row = 0; row < 4; ++row)
  {
    for (int col = 0; col < 4; ++col)
    {
      QVERIFY2(nnc::test_detail::nearlyEqual(got(row, col), expected(row, col), eps),
               qPrintable(QStringLiteral("mismatch at (%1,%2)").arg(row).arg(col)));
    }
  }
}

} // namespace test_detail
} // namespace nnc

class TstREQ_REG_002_FiducialLoader : public QObject
{
  Q_OBJECT

private slots:
  void loadsNavsimFixture();
  void rejectsMissingFile();
  void rejectsBadLine();
  void rejectsEmptyFile();
  void resolvePathDefaultsToFixtureRelativePath();
  void loadedPairsSolveToNavsimInverse();
};

void TstREQ_REG_002_FiducialLoader::loadsNavsimFixture()
{
  const std::string path = nnc::test_detail::fixturePath("fiducials_navsim.txt");
  std::vector<nnc::FiducialPair> pairs;
  std::string err;
  QVERIFY2(nnc::FiducialLoader::load(path, &pairs, &err), err.c_str());
  QCOMPARE(static_cast<int>(pairs.size()), 4);

  QCOMPARE(pairs[0].tracker.x, 0.f);
  QCOMPARE(pairs[0].tracker.y, 0.f);
  QCOMPARE(pairs[0].tracker.z, 0.f);
  QVERIFY(nnc::test_detail::nearlyEqual(pairs[0].image.x, -27.141016f));
  QVERIFY(nnc::test_detail::nearlyEqual(pairs[0].image.y, 25.f));
  QVERIFY(nnc::test_detail::nearlyEqual(pairs[0].image.z, -32.990381f));
}

void TstREQ_REG_002_FiducialLoader::rejectsMissingFile()
{
  std::vector<nnc::FiducialPair> pairs;
  std::string err;
  QVERIFY(!nnc::FiducialLoader::load("/no/such/fiducials.txt", &pairs, &err));
  QVERIFY(!err.empty());
  QVERIFY(pairs.empty());
}

void TstREQ_REG_002_FiducialLoader::rejectsBadLine()
{
  const std::string path = "/tmp/nnc_fiducials_bad_line.txt";
  {
    std::ofstream out(path);
    QVERIFY(out.is_open());
    out << "0 0 0 1 2\n";
  }

  std::vector<nnc::FiducialPair> pairs;
  std::string err;
  QVERIFY(!nnc::FiducialLoader::load(path, &pairs, &err));
  QVERIFY(err.find("expected 6 floats") != std::string::npos);
}

void TstREQ_REG_002_FiducialLoader::rejectsEmptyFile()
{
  const std::string path = "/tmp/nnc_fiducials_empty.txt";
  {
    std::ofstream out(path);
    QVERIFY(out.is_open());
    out << "# comments only\n";
  }

  std::vector<nnc::FiducialPair> pairs;
  std::string err;
  QVERIFY(!nnc::FiducialLoader::load(path, &pairs, &err));
  QVERIFY(err.find("no pairs") != std::string::npos);
}

void TstREQ_REG_002_FiducialLoader::resolvePathDefaultsToFixtureRelativePath()
{
  const std::string path = nnc::FiducialLoader::resolvePath();
  QVERIFY2(!path.empty(), "resolvePath returned empty");
  QVERIFY(path.find("fiducials_navsim.txt") != std::string::npos);
}

void TstREQ_REG_002_FiducialLoader::loadedPairsSolveToNavsimInverse()
{
  const std::string path = nnc::test_detail::fixturePath("fiducials_navsim.txt");
  std::vector<nnc::FiducialPair> pairs;
  std::string err;
  QVERIFY2(nnc::FiducialLoader::load(path, &pairs, &err), err.c_str());

  nnc::RegistrationResult result{};
  QVERIFY2(nnc::PairedPointRegistration::solve(pairs, &result, &err), err.c_str());
  QVERIFY(result.ok);
  QVERIFY(result.freMm < 1e-3f);

  const nnc::Mat4 imageToTracker = nnc::test_detail::makeImageToTrackerGroundTruth();
  nnc::Mat4 trackerToImage = nnc::Mat4::identity();
  QVERIFY(imageToTracker.inverted(trackerToImage));
  nnc::test_detail::expectMat4Near(result.trackerToImage, trackerToImage);
}

QTEST_APPLESS_MAIN(TstREQ_REG_002_FiducialLoader)
#include "tst_fiducial_loader.moc"
