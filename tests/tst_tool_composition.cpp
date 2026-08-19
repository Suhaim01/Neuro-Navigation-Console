#include "reg/ToolComposition.h"

#include <QObject>
#include <QtTest>

#include <cmath>

namespace nnc
{
namespace test_detail
{

constexpr float kTol = 1e-3f;

bool nearlyEqual(float a, float b, float eps = kTol)
{
  return std::fabs(a - b) <= eps;
}

void expectVec3Near(const nnc::Vec3 &got, const nnc::Vec3 &expected, float eps = kTol)
{
  QVERIFY(nnc::test_detail::nearlyEqual(got.x, expected.x, eps));
  QVERIFY(nnc::test_detail::nearlyEqual(got.y, expected.y, eps));
  QVERIFY(nnc::test_detail::nearlyEqual(got.z, expected.z, eps));
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

nnc::Mat4 translationMat4(float tx, float ty, float tz)
{
  nnc::Mat4 m = nnc::Mat4::identity();
  m(0, 3) = tx;
  m(1, 3) = ty;
  m(2, 3) = tz;
  return m;
}

} // namespace test_detail
} // namespace nnc

class TstREQ_FRAME_004_ToolComposition : public QObject
{
  Q_OBJECT

private slots:
  void rejectsNullOutputs();
  void identityRegistrationPreservesToolToTracker();
  void navsimChainRecoversToolInImage();
  void toolTipIsOriginInToolFrame();
};

void TstREQ_FRAME_004_ToolComposition::rejectsNullOutputs()
{
  const nnc::Mat4 identity = nnc::Mat4::identity();
  QVERIFY(!nnc::ToolComposition::composeToolInImage(identity, identity, nullptr));
  QVERIFY(!nnc::ToolComposition::toolTipInImage(identity, nullptr));
}

void TstREQ_FRAME_004_ToolComposition::identityRegistrationPreservesToolToTracker()
{
  const nnc::Mat4 trackerToImage = nnc::Mat4::identity();
  const nnc::Mat4 toolToTracker = nnc::test_detail::translationMat4(12.f, -3.f, 7.f);

  nnc::Mat4 toolInImage = nnc::Mat4::identity();
  QVERIFY(nnc::ToolComposition::composeToolInImage(trackerToImage, toolToTracker, &toolInImage));

  nnc::Vec3 tip{};
  QVERIFY(nnc::ToolComposition::toolTipInImage(toolInImage, &tip));
  nnc::test_detail::expectVec3Near(tip, nnc::Vec3{12.f, -3.f, 7.f});
}

void TstREQ_FRAME_004_ToolComposition::navsimChainRecoversToolInImage()
{
  const nnc::Mat4 imageToTracker = nnc::test_detail::makeImageToTrackerGroundTruth();
  nnc::Mat4 trackerToImage = nnc::Mat4::identity();
  QVERIFY(imageToTracker.inverted(trackerToImage));

  const nnc::Mat4 expectedToolInImage = nnc::test_detail::translationMat4(5.f, 10.f, 40.f);
  const nnc::Mat4 toolToTracker = imageToTracker * expectedToolInImage;

  nnc::Mat4 toolInImage = nnc::Mat4::identity();
  nnc::Vec3 tip{};
  QVERIFY(nnc::ToolComposition::composeToolTipInImage(
    trackerToImage, toolToTracker, &toolInImage, &tip));

  nnc::Vec3 expectedTip{};
  expectedToolInImage.transformPoint(0.f, 0.f, 0.f, expectedTip.x, expectedTip.y, expectedTip.z);
  nnc::test_detail::expectVec3Near(tip, expectedTip);
  QCOMPARE(toolInImage(0, 3), expectedToolInImage(0, 3));
  QCOMPARE(toolInImage(1, 3), expectedToolInImage(1, 3));
  QCOMPARE(toolInImage(2, 3), expectedToolInImage(2, 3));
}

void TstREQ_FRAME_004_ToolComposition::toolTipIsOriginInToolFrame()
{
  const nnc::Mat4 toolInImage = nnc::test_detail::translationMat4(1.f, 2.f, 3.f);
  nnc::Vec3 tip{};
  QVERIFY(nnc::ToolComposition::toolTipInImage(toolInImage, &tip));
  nnc::test_detail::expectVec3Near(tip, nnc::Vec3{1.f, 2.f, 3.f});
}

QTEST_APPLESS_MAIN(TstREQ_FRAME_004_ToolComposition)
#include "tst_tool_composition.moc"
