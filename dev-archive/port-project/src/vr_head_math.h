// Condemned 2 VR: head-pose maths shared by vr_head_pose.cpp and its test (no game headers here).
//
// OpenXR: +X right, +Y up, -Z forward, right-handed. LithTech (the game): +X right, +Y up, +Z
// forward, left-handed (vr_camera_probe.cpp: right = q*(1,0,0), forward = q*(0,0,1)). The two
// differ by a mirror of Z, so a rotation's axis keeps Z and flips X and Y.
#pragma once

#include <cmath>

namespace c2vr {

struct Quat {
  float x, y, z, w;
};
struct Vec3 {
  float x, y, z;
};

inline Quat XrToLithQuat(Quat q) { return {-q.x, -q.y, q.z, q.w}; }
inline Vec3 XrToLithVec(Vec3 v) { return {v.x, v.y, -v.z}; }

inline Quat Mul(Quat a, Quat b) {  // a then b in the camera's own frame: world = a * b * v
  return {a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
          a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
          a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
          a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z};
}

inline Quat Normalize(Quat q) {
  const float n = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
  return n > 0.0f ? Quat{q.x / n, q.y / n, q.z / n, q.w / n} : Quat{0, 0, 0, 1};
}

inline Vec3 Rotate(Quat q, Vec3 v) {  // q * v * q^-1
  const Vec3 u{q.x, q.y, q.z};
  const Vec3 t{2 * (u.y * v.z - u.z * v.y), 2 * (u.z * v.x - u.x * v.z), 2 * (u.x * v.y - u.y * v.x)};
  return {v.x + q.w * t.x + (u.y * t.z - u.z * t.y), v.y + q.w * t.y + (u.z * t.x - u.x * t.z),
          v.z + q.w * t.z + (u.x * t.y - u.y * t.x)};
}

// The game camera turned by the head: head rotation applied in the camera's own frame.
inline Quat CameraWithHead(Quat game, Quat head_xr) {
  return Normalize(Mul(Normalize(game), XrToLithQuat(Normalize(head_xr))));
}

}  // namespace c2vr
