// Condemned 2 VR: "same moment" stereo (C2_EYE_SYNC=1). Shared between vr_camera_probe.cpp
// (set view + FlipScreen) and vr_eye_sync.cpp (frame, clocks, swaps). Reader draft, 2026-10-12.
#pragma once

#include <cstdint>

namespace c2vr {

// True when C2_EYE_SYNC=1 and C2_CAM_EYE is set (otherwise the old per-flip behaviour runs).
bool SyncOn();

// Sign of the eye offset for a view being built right now: -1 left eye, +1 right eye
// (C2_EYE_SWAP=1 swaps them). Counts views built off the frame thread.
float EyeSignForView();

// Called by the FlipScreen hook around the original: Begin before, End after, with whether
// FlipScreen reported a present (its r3 != 0).
void OnFlipBegin();
void OnFlipEnd(bool presented);

// One block of [C2VR] SYNC / FREEZE lines (called every N presents by the FlipScreen hook).
void ReportSync();

}  // namespace c2vr
