// Numeric test of vr_head_math.h against independent ground truth (rotation matrices built from
// axis/angle, and the mirror M = diag(1,1,-1) applied explicitly). Exit 0 = pass.
#include "../src/vr_head_math.h"
#include <cstdio>
#include <cstdlib>
using namespace c2vr;
static int fails = 0;
static void Check(const char* what, Vec3 a, Vec3 b) {
  const float d = std::fabs(a.x - b.x) + std::fabs(a.y - b.y) + std::fabs(a.z - b.z);
  if (d > 1e-4f) { ++fails; std::printf("FAIL %s: (%f %f %f) vs (%f %f %f)\n", what, a.x, a.y, a.z, b.x, b.y, b.z); }
}
static float R() { return float(std::rand()) / RAND_MAX * 2.0f - 1.0f; }
// Rodrigues: rotate v about unit axis k by angle a (right-hand rule).
static Vec3 AxisAngle(Vec3 k, float a, Vec3 v) {
  const float c = std::cos(a), s = std::sin(a), d = k.x * v.x + k.y * v.y + k.z * v.z;
  const Vec3 x{k.y * v.z - k.z * v.y, k.z * v.x - k.x * v.z, k.x * v.y - k.y * v.x};
  return {v.x * c + x.x * s + k.x * d * (1 - c), v.y * c + x.y * s + k.y * d * (1 - c), v.z * c + x.z * s + k.z * d * (1 - c)};
}
static Vec3 M(Vec3 v) { return {v.x, v.y, -v.z}; }
int main() {
  std::srand(7);
  // 1. Rotate() agrees with Rodrigues for random axis/angle.
  for (int i = 0; i < 1000; ++i) {
    Vec3 k{R(), R(), R()}; const float n = std::sqrt(k.x * k.x + k.y * k.y + k.z * k.z); k = {k.x / n, k.y / n, k.z / n};
    const float a = R() * 3.1f; const Vec3 v{R(), R(), R()};
    const Quat q{k.x * std::sin(a / 2), k.y * std::sin(a / 2), k.z * std::sin(a / 2), std::cos(a / 2)};
    Check("rotate", Rotate(q, v), AxisAngle(k, a, v));
    // 2. Converted rotation acts on mirrored vectors as the mirror of the original: R' M v = M R v.
    Check("mirror", Rotate(XrToLithQuat(q), M(v)), M(Rotate(q, v)));
    // 3. Composition: forward of game*head = game applied to head's (converted) forward.
    Vec3 k2{R(), R(), R()}; const float n2 = std::sqrt(k2.x * k2.x + k2.y * k2.y + k2.z * k2.z); k2 = {k2.x / n2, k2.y / n2, k2.z / n2};
    const float b = R() * 3.1f;
    const Quat g{k2.x * std::sin(b / 2), k2.y * std::sin(b / 2), k2.z * std::sin(b / 2), std::cos(b / 2)};
    Check("compose", Rotate(CameraWithHead(g, q), v), Rotate(g, Rotate(XrToLithQuat(q), v)));
  }
  // 4. Named cases. OpenXR yaw +30 deg about +Y turns the view LEFT; in the game, left is -X.
  const float h = 3.14159265f / 12;  // half of 30 deg
  const Quat yaw_left{0, std::sin(h), 0, std::cos(h)};
  const Vec3 fwd = Rotate(XrToLithQuat(yaw_left), {0, 0, 1});
  Check("yaw-left forward", fwd, {-0.5f, 0, 0.8660254f});
  // Pitch +20 deg about +X in OpenXR looks UP; game forward must gain +Y.
  const float p = 3.14159265f / 18;
  const Vec3 up = Rotate(XrToLithQuat(Quat{std::sin(p), 0, 0, std::cos(p)}), {0, 0, 1});
  Check("pitch-up forward", up, {0, 0.3420201f, 0.9396926f});
  // Roll +15 deg about -Z (OpenXR forward axis is -Z, so +angle about +Z tilts head LEFT: right axis gains +Y).
  const float r = 3.14159265f / 24;
  const Vec3 right = Rotate(XrToLithQuat(Quat{0, 0, std::sin(r), std::cos(r)}), {1, 0, 0});
  Check("roll-left right axis", right, {0.9659258f, 0.2588190f, 0});
  std::printf("%s (%d failures, 3003 checks)\n", fails ? "FAIL" : "PASS", fails);
  return fails ? 1 : 0;
}
