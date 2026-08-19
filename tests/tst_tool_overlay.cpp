#include "reg/ToolOverlay.h"

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

nnc::Mat4 translationMat4(float tx, float ty, float tz)
{
  nnc::Mat4 m = nnc::Mat4::identity();
  m(0, 3) = tx;
  m(1, 3) = ty;
  m(2, 3) = tz;
  return m;
}

nnc::Mat4 pitchYMm(float pitchRad, float tx, float ty, float tz)
{
  const float c = std::cos(pitchRad);
  const float s = std::sin(pitchRad);
  nnc::Mat4 m = nnc::Mat4::identity();
  m(0, 0) = c;
  m(0, 2) = s;
  m(2, 0) = -s;
  m(2, 2) = c;
  m(0, 3) = tx;
  m(1, 3) = ty;
  m(2, 3) = tz;
  return m;
}

} // namespace test_detail
} // namespace nnc

class TstREQ_GUI_001_ToolOverlayImage : public QObject
{
  Q_OBJECT

private slots:
  void rejectsNullOutput();
  void identityPoseYieldsOriginAndPlusZShaft();
  void translationOnlyMovesTipNotShaftDir();
  void rotatedPoseRotatesShaftDirAndEndpoint();
};

void TstREQ_GUI_001_ToolOverlayImage::rejectsNullOutput()
{
  const nnc::Mat4 identity = nnc::Mat4::identity();
  QVERIFY(!nnc::ToolOverlay::toolGeometryInImage(identity, nullptr));
}

void TstREQ_GUI_001_ToolOverlayImage::identityPoseYieldsOriginAndPlusZShaft()
{
  const nnc::Mat4 toolInImage = nnc::Mat4::identity();
  nnc::ToolOverlayImage overlay{};
  QVERIFY(nnc::ToolOverlay::toolGeometryInImage(toolInImage, &overlay));

  nnc::test_detail::expectVec3Near(overlay.tipMm, nnc::Vec3{0.f, 0.f, 0.f});
  nnc::test_detail::expectVec3Near(overlay.shaftDirMm, nnc::Vec3{0.f, 0.f, 1.f});
  nnc::test_detail::expectVec3Near(
    overlay.shaftEndMm, nnc::Vec3{0.f, 0.f, nnc::kShaftDisplayLengthMm});
}

void TstREQ_GUI_001_ToolOverlayImage::translationOnlyMovesTipNotShaftDir()
{
  const nnc::Mat4 toolInImage = nnc::test_detail::translationMat4(12.f, -3.f, 7.f);
  nnc::ToolOverlayImage overlay{};
  QVERIFY(nnc::ToolOverlay::toolGeometryInImage(toolInImage, &overlay));

  nnc::test_detail::expectVec3Near(overlay.tipMm, nnc::Vec3{12.f, -3.f, 7.f});
  nnc::test_detail::expectVec3Near(overlay.shaftDirMm, nnc::Vec3{0.f, 0.f, 1.f});
  nnc::test_detail::expectVec3Near(
    overlay.shaftEndMm,
    nnc::Vec3{12.f, -3.f, 7.f + nnc::kShaftDisplayLengthMm});
}

void TstREQ_GUI_001_ToolOverlayImage::rotatedPoseRotatesShaftDirAndEndpoint()
{
  const float pitchRad = 30.f * 0.017453292519943295f;
  const nnc::Mat4 toolInImage = nnc::test_detail::pitchYMm(pitchRad, 40.f, -25.f, 15.f);
  nnc::ToolOverlayImage overlay{};
  QVERIFY(nnc::ToolOverlay::toolGeometryInImage(toolInImage, &overlay));

  const float c = std::cos(pitchRad);
  const float s = std::sin(pitchRad);
  const nnc::Vec3 expectedDir{s, 0.f, c};

  nnc::test_detail::expectVec3Near(overlay.tipMm, nnc::Vec3{40.f, -25.f, 15.f});
  nnc::test_detail::expectVec3Near(overlay.shaftDirMm, expectedDir);
  nnc::test_detail::expectVec3Near(
    overlay.shaftEndMm,
    nnc::Vec3{
      40.f + expectedDir.x * nnc::kShaftDisplayLengthMm,
      -25.f,
      15.f + expectedDir.z * nnc::kShaftDisplayLengthMm});
}

QTEST_APPLESS_MAIN(TstREQ_GUI_001_ToolOverlayImage)
#include "tst_tool_overlay.moc"
