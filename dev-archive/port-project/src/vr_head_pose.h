// Condemned 2 VR: the headset turns the game camera (C2_XR_HEAD=1). See vr_head_pose.cpp.
#pragma once

#include <cstdint>

#include "vr_head_math.h"

namespace c2vr {

// True when C2_XR_HEAD=1 and C2_EYE_SYNC is on (pairs are numbered by vr_eye_sync.cpp).
bool HeadOn();

// The head for pair `pair`, sampled once at its first call and kept for both eyes. q is in
// OpenXR's axes (vr_head_math.h converts it); offset_m is the head's movement since the first
// sample, in metres, OpenXR axes, or zero unless C2_XR_HEAD_POS=1. False when the headset gave no
// sample (q is then identity).
bool HeadForPair(uint32_t pair, Quat* q, Vec3* offset_m);

float UnitsPerMetre();

// C2_XR_FOV=1: the headset's own view, both eyes covered, as tangents left, right, up, down (all
// positive), from SDK patch 09b. False when off, or before the headset has given a sample.
bool HeadsetFov(float t[4]);

// The tangents the player view is really drawn with, for dk_xr_eye_fov (off-centre allowed).
void NotePlayerTangents(float left, float right, float up, float down);

// C2_CAM_EYE=auto: half the headset's own eye distance in game units (SDK patch 09b,
// rex_xr_eye_distance), or `fallback` until the headset has reported one.
float HalfEyeUnits(float fallback);

// The player camera's FOV tangents as set-view receives them, for dk_xr_eye_fov.
void NotePlayerFov(float tan_x, float tan_y);

}  // namespace c2vr
