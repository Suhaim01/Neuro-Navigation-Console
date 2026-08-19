#include "app/RegistrationBootstrap.h"
#include "app/SceneModel.h"
#include "reg/FiducialLoader.h"
#include "reg/PairedPointRegistration.h"
#include "reg/ToolComposition.h"

#include <QObject>
#include <QtTest>

#include <cmath>
#include <cstdlib>
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

nnc::Mat4 toolInImageAlongPlan(float phase01)
{
  const float t = phase01 - std::floor(phase01);
  nnc::Mat4 pose = nnc::Mat4::identity();
  pose(2, 3) = 80.f * t;
  return pose;
}

bool nearlyEqual(float a, float b, float eps = 1e-2f)
{
  return std::fabs(a - b) <= eps;
}

bool bootstrapFromFixturePath(nnc::SceneModel *sceneModel, const std::string &path)
{
  std::vector<nnc::FiducialPair> pairs;
  std::string err;
  if (!nnc::FiducialLoader::load(path, &pairs, &err))
  {
    return false;
  }
  nnc::RegistrationResult result{};
  if (!nnc::PairedPointRegistration::solve(pairs, &result, &err))
  {
    return false;
  }
  if (!result.ok)
  {
    return false;
  }
  sceneModel->setRegistration(result);
  return true;
}

} // namespace test_detail
} // namespace nnc

class TstREQ_REG_002_RegistrationBootstrap : public QObject
{
  Q_OBJECT

private slots:
  void bootstrapSetsSceneModelRegistration();
  void composedNavsimPoseLiesOnPlanAxis();
  void bootstrapRegistrationFailsWhenFileMissing();
};

void TstREQ_REG_002_RegistrationBootstrap::bootstrapSetsSceneModelRegistration()
{
  nnc::SceneModel sceneModel;
  const std::string path = nnc::test_detail::fixturePath("fiducials_navsim.txt");
  QVERIFY(nnc::test_detail::bootstrapFromFixturePath(&sceneModel, path));
  QVERIFY(sceneModel.hasRegistration());
  QVERIFY(sceneModel.freMm() < 1e-3f);
  QCOMPARE(static_cast<int>(sceneModel.residualMm().size()), 4);

  const nnc::Mat4 imageToTracker = nnc::test_detail::makeImageToTrackerGroundTruth();
  nnc::Mat4 trackerToImage = nnc::Mat4::identity();
  QVERIFY(imageToTracker.inverted(trackerToImage));
  for (int row = 0; row < 4; ++row)
  {
    for (int col = 0; col < 4; ++col)
    {
      QVERIFY(nnc::test_detail::nearlyEqual(
        sceneModel.trackerToImage()(row, col), trackerToImage(row, col), 1e-3f));
    }
  }
}

void TstREQ_REG_002_RegistrationBootstrap::composedNavsimPoseLiesOnPlanAxis()
{
  nnc::SceneModel sceneModel;
  const std::string path = nnc::test_detail::fixturePath("fiducials_navsim.txt");
  QVERIFY(nnc::test_detail::bootstrapFromFixturePath(&sceneModel, path));

  const nnc::Mat4 imageToTracker = nnc::test_detail::makeImageToTrackerGroundTruth();
  const nnc::Mat4 toolToTracker =
    imageToTracker * nnc::test_detail::toolInImageAlongPlan(0.35f);

  nnc::Vec3 tipImageMm{};
  QVERIFY(nnc::ToolComposition::composeToolTipInImage(
    sceneModel.trackerToImage(), toolToTracker, nullptr, &tipImageMm));

  QVERIFY(nnc::test_detail::nearlyEqual(tipImageMm.x, 0.f, 1e-2f));
  QVERIFY(nnc::test_detail::nearlyEqual(tipImageMm.y, 0.f, 1e-2f));
  QVERIFY(nnc::test_detail::nearlyEqual(tipImageMm.z, 28.f, 1e-1f));
}

void TstREQ_REG_002_RegistrationBootstrap::bootstrapRegistrationFailsWhenFileMissing()
{
  const char *previous = std::getenv("NNC_FIDUCIALS");
  const std::string restore = previous != nullptr ? std::string(previous) : std::string();
#ifdef _GNU_SOURCE
  ::setenv("NNC_FIDUCIALS", "/no/such/fiducials.txt", 1);
#else
  ::putenv(const_cast<char *>("NNC_FIDUCIALS=/no/such/fiducials.txt"));
#endif

  nnc::SceneModel sceneModel;
  std::string err;
  QVERIFY(!nnc::bootstrapRegistration(&sceneModel, &err));
  QVERIFY(!sceneModel.hasRegistration());
  QVERIFY(!err.empty());

  if (!restore.empty())
  {
#ifdef _GNU_SOURCE
    ::setenv("NNC_FIDUCIALS", restore.c_str(), 1);
#else
    const std::string envLine = "NNC_FIDUCIALS=" + restore;
    ::putenv(const_cast<char *>(envLine.c_str()));
#endif
  }
  else
  {
#ifdef _GNU_SOURCE
    ::unsetenv("NNC_FIDUCIALS");
#endif
  }
}

QTEST_APPLESS_MAIN(TstREQ_REG_002_RegistrationBootstrap)
#include "tst_registration_bootstrap.moc"
