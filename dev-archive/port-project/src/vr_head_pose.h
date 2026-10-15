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

// The player camera's FOV tangents as set-view receives them, for dk_xr_eye_fov.
void NotePlayerFov(float tan_x, float tan_y);

}  // namespace c2vr
