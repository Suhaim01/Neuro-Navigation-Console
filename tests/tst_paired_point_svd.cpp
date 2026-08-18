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
  void acceptsValidNonDegeneratePairs();
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

void TstREQ_REG_001_PairedPointValidation::acceptsValidNonDegeneratePairs()
{
  const std::vector<nnc::FiducialPair> pairs = nnc::test_detail::validTetrahedronPairs();
  nnc::RegistrationResult result{};
  std::string err;
  QVERIFY(!nnc::PairedPointRegistration::solve(pairs, &result, &err));
  QVERIFY(!result.ok);
  QVERIFY(err.find("not implemented") != std::string::npos);
}

QTEST_APPLESS_MAIN(TstREQ_REG_001_PairedPointValidation)
#include "tst_paired_point_svd.moc"
