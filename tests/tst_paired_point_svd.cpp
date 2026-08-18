#include "reg/PairedPointRegistration.h"

#include <QObject>
#include <QtTest>

#include <cmath>
#include <limits>
#include <string>
#include <vector>

namespace nnc
{
namespace test_detail
{

constexpr float kTol = 1e-3f;

nnc::FiducialPair makePair(float tx, float ty, float tz, float ix, float iy, float iz)
{
  nnc::FiducialPair pair{};
  pair.tracker.x = tx;
  pair.tracker.y = ty;
  pair.tracker.z = tz;
  pair.image.x = ix;
  pair.image.y = iy;
  pair.image.z = iz;
  return pair;
}

std::vector<nnc::FiducialPair> validTetrahedronPairs()
{
  return {
    nnc::test_detail::makePair(0.f, 0.f, 0.f, 10.f, 0.f, 0.f),
    nnc::test_detail::makePair(1.f, 0.f, 0.f, 11.f, 0.f, 0.f),
    nnc::test_detail::makePair(0.f, 1.f, 0.f, 10.f, 1.f, 0.f),
    nnc::test_detail::makePair(0.f, 0.f, 1.f, 10.f, 0.f, 1.f),
  };
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

void transformPoint(const nnc::Mat4 &m, const nnc::Vec3 &in, nnc::Vec3 *out)
{
  float x = 0.f;
  float y = 0.f;
  float z = 0.f;
  m.transformPoint(in.x, in.y, in.z, x, y, z);
  out->x = x;
  out->y = y;
  out->z = z;
}

bool nearlyEqual(float a, float b, float eps = kTol)
{
  return std::fabs(a - b) <= eps;
}

void expectMat4Near(const nnc::Mat4 &got, const nnc::Mat4 &expected, float eps = kTol)
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

class TstREQ_REG_001_PairedPointValidation : public QObject
{
  Q_OBJECT

private slots:
  void rejectsNullOutput();
  void rejectsTooFewPairs();
  void rejectsNonFiniteCoordinates();
  void rejectsCoincidentTrackerLandmarks();
  void rejectsCoincidentImageLandmarks();
  void rejectsCollinearTrackerLandmarks();
  void rejectsCollinearImageLandmarks();
  void recoversPureTranslation();
  void recoversIdentityRegistration();
  void recoversNavsimInverseRotation();
};

void TstREQ_REG_001_PairedPointValidation::rejectsNullOutput()
{
  const std::vector<nnc::FiducialPair> pairs = nnc::test_detail::validTetrahedronPairs();
  std::string err;
  QVERIFY(!nnc::PairedPointRegistration::solve(pairs, nullptr, &err));
  QVERIFY(err.find("null") != std::string::npos);
}

void TstREQ_REG_001_PairedPointValidation::rejectsTooFewPairs()
{
  const std::vector<nnc::FiducialPair> pairs = {
    nnc::test_detail::makePair(0.f, 0.f, 0.f, 0.f, 0.f, 0.f),
    nnc::test_detail::makePair(1.f, 0.f, 0.f, 1.f, 0.f, 0.f),
  };
  nnc::RegistrationResult result{};
  std::string err;
  QVERIFY(!nnc::PairedPointRegistration::solve(pairs, &result, &err));
  QVERIFY(!result.ok);
  QVERIFY(err.find("at least 3") != std::string::npos);
}

void TstREQ_REG_001_PairedPointValidation::rejectsNonFiniteCoordinates()
{
  std::vector<nnc::FiducialPair> pairs = nnc::test_detail::validTetrahedronPairs();
  pairs[1].tracker.x = std::numeric_limits<float>::quiet_NaN();
  nnc::RegistrationResult result{};
  std::string err;
  QVERIFY(!nnc::PairedPointRegistration::solve(pairs, &result, &err));
  QVERIFY(!result.ok);
  QVERIFY(err.find("non-finite") != std::string::npos);
}

void TstREQ_REG_001_PairedPointValidation::rejectsCoincidentTrackerLandmarks()
{
  const std::vector<nnc::FiducialPair> pairs = {
    nnc::test_detail::makePair(0.f, 0.f, 0.f, 0.f, 0.f, 0.f),
    nnc::test_detail::makePair(0.f, 0.f, 0.f, 1.f, 0.f, 0.f),
    nnc::test_detail::makePair(1.f, 0.f, 0.f, 0.f, 1.f, 0.f),
  };
  nnc::RegistrationResult result{};
  std::string err;
  QVERIFY(!nnc::PairedPointRegistration::solve(pairs, &result, &err));
  QVERIFY(!result.ok);
  QVERIFY(err.find("coincident tracker") != std::string::npos);
}

void TstREQ_REG_001_PairedPointValidation::rejectsCoincidentImageLandmarks()
{
  const std::vector<nnc::FiducialPair> pairs = {
    nnc::test_detail::makePair(0.f, 0.f, 0.f, 0.f, 0.f, 0.f),
    nnc::test_detail::makePair(1.f, 0.f, 0.f, 0.f, 0.f, 0.f),
    nnc::test_detail::makePair(0.f, 1.f, 0.f, 1.f, 0.f, 0.f),
  };
  nnc::RegistrationResult result{};
  std::string err;
  QVERIFY(!nnc::PairedPointRegistration::solve(pairs, &result, &err));
  QVERIFY(!result.ok);
  QVERIFY(err.find("coincident image") != std::string::npos);
}

void TstREQ_REG_001_PairedPointValidation::rejectsCollinearTrackerLandmarks()
{
  const std::vector<nnc::FiducialPair> pairs = {
    nnc::test_detail::makePair(0.f, 0.f, 0.f, 0.f, 0.f, 0.f),
    nnc::test_detail::makePair(1.f, 0.f, 0.f, 1.f, 0.f, 0.f),
    nnc::test_detail::makePair(2.f, 0.f, 0.f, 0.f, 1.f, 0.f),
  };
  nnc::RegistrationResult result{};
  std::string err;
  QVERIFY(!nnc::PairedPointRegistration::solve(pairs, &result, &err));
  QVERIFY(!result.ok);
  QVERIFY(err.find("collinear tracker") != std::string::npos);
}

void TstREQ_REG_001_PairedPointValidation::rejectsCollinearImageLandmarks()
{
  const std::vector<nnc::FiducialPair> pairs = {
    nnc::test_detail::makePair(0.f, 0.f, 0.f, 0.f, 0.f, 0.f),
    nnc::test_detail::makePair(1.f, 0.f, 0.f, 1.f, 0.f, 0.f),
    nnc::test_detail::makePair(0.f, 1.f, 0.f, 2.f, 0.f, 0.f),
  };
  nnc::RegistrationResult result{};
  std::string err;
  QVERIFY(!nnc::PairedPointRegistration::solve(pairs, &result, &err));
  QVERIFY(!result.ok);
  QVERIFY(err.find("collinear image") != std::string::npos);
}

void TstREQ_REG_001_PairedPointValidation::recoversPureTranslation()
{
  const std::vector<nnc::FiducialPair> pairs = nnc::test_detail::validTetrahedronPairs();
  nnc::RegistrationResult result{};
  std::string err;
  QVERIFY2(nnc::PairedPointRegistration::solve(pairs, &result, &err), err.c_str());
  QVERIFY(result.ok);

  nnc::Mat4 expected = nnc::Mat4::identity();
  expected(0, 3) = 10.f;
  nnc::test_detail::expectMat4Near(result.trackerToImage, expected);
}

void TstREQ_REG_001_PairedPointValidation::recoversIdentityRegistration()
{
  const std::vector<nnc::FiducialPair> pairs = {
    nnc::test_detail::makePair(0.f, 0.f, 0.f, 0.f, 0.f, 0.f),
    nnc::test_detail::makePair(1.f, 0.f, 0.f, 1.f, 0.f, 0.f),
    nnc::test_detail::makePair(0.f, 1.f, 0.f, 0.f, 1.f, 0.f),
    nnc::test_detail::makePair(0.f, 0.f, 1.f, 0.f, 0.f, 1.f),
  };
  nnc::RegistrationResult result{};
  std::string err;
  QVERIFY2(nnc::PairedPointRegistration::solve(pairs, &result, &err), err.c_str());
  QVERIFY(result.ok);
  nnc::test_detail::expectMat4Near(result.trackerToImage, nnc::Mat4::identity());
}

void TstREQ_REG_001_PairedPointValidation::recoversNavsimInverseRotation()
{
  const nnc::Mat4 imageToTracker = nnc::test_detail::makeImageToTrackerGroundTruth();
  nnc::Mat4 trackerToImage = nnc::Mat4::identity();
  QVERIFY(imageToTracker.inverted(trackerToImage));

  const nnc::Vec3 trackerPts[] = {
    {0.f, 0.f, 0.f},
    {20.f, -5.f, 10.f},
    {-8.f, 12.f, 30.f},
    {5.f, 5.f, -15.f},
  };

  std::vector<nnc::FiducialPair> pairs;
  pairs.reserve(4);
  for (const nnc::Vec3 &trackerPt : trackerPts)
  {
    nnc::Vec3 imagePt{};
    nnc::test_detail::transformPoint(trackerToImage, trackerPt, &imagePt);
    nnc::FiducialPair pair{};
    pair.tracker = trackerPt;
    pair.image = imagePt;
    pairs.push_back(pair);
  }

  nnc::RegistrationResult result{};
  std::string err;
  QVERIFY2(nnc::PairedPointRegistration::solve(pairs, &result, &err), err.c_str());
  QVERIFY(result.ok);
  nnc::test_detail::expectMat4Near(result.trackerToImage, trackerToImage);
}

QTEST_APPLESS_MAIN(TstREQ_REG_001_PairedPointValidation)
#include "tst_paired_point_svd.moc"
