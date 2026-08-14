#include "io/NiftiLoader.h"

#include <QObject>
#include <QtTest>

#include <cmath>
#include <cstdlib>
#include <string>

using nnc::NiftiLoader;
using nnc::NiftiVolume;

namespace {

std::string fixturePath(const char* name)
{
  const char* root = std::getenv("NNC_FIXTURES");
  if (root && root[0] != '\0') {
    return std::string(root) + "/" + name;
  }
#ifdef NNC_FIXTURES_DIR
  return std::string(NNC_FIXTURES_DIR) + "/" + name;
#else
  return std::string("tests/fixtures/") + name;
#endif
}

bool nearlyEqual(float a, float b, float eps = 1e-4f)
{
  return std::fabs(a - b) <= eps;
}

}  // namespace

class TstREQ_FRAME_001_NiftiSformMatchesKnownMatrix : public QObject {
  Q_OBJECT
private slots:
  void loadsTinySformFixture();
  void loadsTinyQformFixture();
  void loadsMRHeadWhenPresent();
};

void TstREQ_FRAME_001_NiftiSformMatchesKnownMatrix::loadsTinySformFixture()
{
  NiftiVolume vol;
  std::string err;
  const auto path = fixturePath("tiny_sform.nii");
  QVERIFY2(NiftiLoader::load(path, vol, &err), err.c_str());

  QCOMPARE(vol.nx, 4);
  QCOMPARE(vol.ny, 4);
  QCOMPARE(vol.nz, 2);
  QVERIFY(vol.sformCode > 0);

  // sform: diag(2,3,4), origin (10,20,30)
  QCOMPARE(vol.voxelToImage(0, 0), 2.f);
  QCOMPARE(vol.voxelToImage(1, 1), 3.f);
  QCOMPARE(vol.voxelToImage(2, 2), 4.f);
  QCOMPARE(vol.voxelToImage(0, 3), 10.f);
  QCOMPARE(vol.voxelToImage(1, 3), 20.f);
  QCOMPARE(vol.voxelToImage(2, 3), 30.f);

  float x = 0, y = 0, z = 0;
  vol.voxelToImage.transformPoint(1.f, 2.f, 1.f, x, y, z);
  QVERIFY(nearlyEqual(x, 12.f));
  QVERIFY(nearlyEqual(y, 26.f));
  QVERIFY(nearlyEqual(z, 34.f));

  QCOMPARE(vol.voxel(1, 2, 1), 121.f);
}

void TstREQ_FRAME_001_NiftiSformMatchesKnownMatrix::loadsTinyQformFixture()
{
  NiftiVolume vol;
  std::string err;
  QVERIFY2(NiftiLoader::load(fixturePath("tiny_qform.nii"), vol, &err), err.c_str());
  QVERIFY(vol.qformCode > 0);
  QCOMPARE(vol.sformCode, 0);

  float x = 0, y = 0, z = 0;
  vol.voxelToImage.transformPoint(1.f, 0.f, 1.f, x, y, z);
  QVERIFY(nearlyEqual(x, 3.f));
  QVERIFY(nearlyEqual(y, 2.f));
  QVERIFY(nearlyEqual(z, 7.f));
}

void TstREQ_FRAME_001_NiftiSformMatchesKnownMatrix::loadsMRHeadWhenPresent()
{
  const char* dataRoot = std::getenv("NNC_DATA");
  std::string path = dataRoot && dataRoot[0] != '\0'
                         ? std::string(dataRoot) + "/MRHead.nii"
                         : std::string("data/MRHead.nii");

  NiftiVolume vol;
  std::string err;
  if (!NiftiLoader::load(path, vol, &err)) {
    QSKIP("data/MRHead.nii not available (expected on local Day 1 machines)");
  }

  QCOMPARE(vol.nx, 256);
  QCOMPARE(vol.ny, 256);
  QCOMPARE(vol.nz, 130);
  QVERIFY(vol.sformCode > 0);
  QCOMPARE(static_cast<int>(vol.voxels.size()), 256 * 256 * 130);

  // srow from header probe: x = 1.299995*k - 86.6449, y = -i + 133.9286, z = -j + 116.7857
  float x = 0, y = 0, z = 0;
  vol.voxelToImage.transformPoint(0.f, 0.f, 0.f, x, y, z);
  QVERIFY(nearlyEqual(x, -86.6448974609375f));
  QVERIFY(nearlyEqual(y, 133.92860412597656f));
  QVERIFY(nearlyEqual(z, 116.78569793701172f));

  vol.voxelToImage.transformPoint(10.f, 20.f, 5.f, x, y, z);
  QVERIFY(nearlyEqual(x, 5.f * 1.2999954223632812f - 86.6448974609375f));
  QVERIFY(nearlyEqual(y, -10.f + 133.92860412597656f));
  QVERIFY(nearlyEqual(z, -20.f + 116.78569793701172f));
}

QTEST_APPLESS_MAIN(TstREQ_FRAME_001_NiftiSformMatchesKnownMatrix)
#include "tst_nifti_loader.moc"
