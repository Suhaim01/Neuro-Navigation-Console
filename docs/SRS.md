# Software Requirements Specification (first draft)

**Product:** Neuro-Navigation-Console  
**Version:** 0.1.0-alpha.1  
**Status:** Research software — not a medical device, not for clinical use.  
**Document type:** First-draft SRS / build filter. Requirement IDs are stable; wording may tighten as tests land.

Related: [DISCLAIMER.md](DISCLAIMER.md).

---

## 1. Purpose

Specify what the console must do so implementation and tests can be traced to requirement IDs. The console is an OR-side neuronavigation UI prototype: display a brain MRI, receive a planned trajectory and live tool poses, register tracker space to image space, and refuse to present stale or ungated guidance as live navigation.

This document does not claim clinical validation or regulatory clearance.

---

## 2. Coordinate frames and transforms

All geometry used for display and guidance is defined here. Names are normative for code, tests, and later docs.

### 2.1 Spaces

| Space | Units | Description |
| --- | --- | --- |
| Voxel index | integer `(i, j, k)` | Indices into the loaded volume array |
| Image world | millimetres | Patient/image anatomy space; all on-screen geometry is expressed here |
| Tracker | millimetres | Optical tracker / machine frame as reported by the stream |
| Tool | millimetres | Tip at the origin; shaft along `+Z` |

### 2.2 Transforms

| Name | Maps | Source |
| --- | --- | --- |
| `voxelToImage` | voxel index → image world | NIfTI-1 `sform` / `qform` header |
| `toolToTracker` | tool → tracker | Streamed pose (TRANSFORM / TDATA) |
| `trackerToImage` | tracker → image world | Paired-point registration (SVD), not hard-coded identity |

Displayed tool tip / shaft in image world:

`tool_in_image = trackerToImage × toolToTracker`

Each of `voxelToImage`, `toolToTracker`, and `trackerToImage` shall be independently unit-testable.

### 2.3 Frame-related requirements

| ID | Requirement |
| --- | --- |
| REQ-FRAME-001 | The system shall derive `voxelToImage` from the loaded NIfTI-1 `sform`/`qform` and use it for all voxel↔mm conversions used in display. |
| REQ-FRAME-002 | The system shall treat streamed tool poses as `toolToTracker` with tip-at-origin and shaft along `+Z`. |
| REQ-FRAME-003 | The system shall obtain `trackerToImage` from paired-point registration over corresponding fiducials; navigation display shall not assume identity `trackerToImage`. |
| REQ-FRAME-004 | The system shall compose `trackerToImage × toolToTracker` to place the tool in image world for overlays and guidance. |

---

## 3. Functional requirements

Priority: **Must** = ship even if later days slip. **Should** = ship if time allows. **Cut** = explicitly out of scope.

### 3.1 Volume and MPR

| ID | Pri | Requirement |
| --- | --- | --- |
| REQ-VOL-001 | Must | The system shall load a NIfTI-1 volume (prototype input: `data/MRHead.nii`). |
| REQ-VOL-002 | Must | The system shall upload volume samples as a GL 3D texture (`sampler3D`). |
| REQ-VOL-003 | Must | The system shall present three orthogonal MPR views (axial, coronal, sagittal). |
| REQ-VOL-004 | Must | The three MPR views shall share a linked crosshair in image world. |
| REQ-VOL-005 | Must | Each MPR view shall support zoom and pan. |
| REQ-VOL-006 | Must | Intensity mapping shall use fixed contrast (no interactive window/level controls). |

### 3.2 Plan and tracking (OpenIGTLink)

| ID | Pri | Requirement |
| --- | --- | --- |
| REQ-IGTL-001 | Must | The system shall receive TRAJECTORY messages (entry + target) over OpenIGTLink. |
| REQ-IGTL-002 | Must | The system shall receive tool poses (`toolToTracker`) over OpenIGTLink at approximately 30–60 Hz under nominal simulation. |
| REQ-IGTL-003 | Must | OpenIGTLink receive shall run off the GUI thread. |
| REQ-IGTL-004 | Must | Poses shall cross to the render path via a triple buffer so the render loop does not block or allocate on receive. |
| REQ-IGTL-005 | Must | A companion `navsim` process shall publish TRAJECTORY on connect and then stream poses, with configurable jitter, latency, and dropout for demo/test. |
| REQ-IGTL-006 | Must | The console shall own no planning authoring UI; the plan arrives over the wire. |

### 3.3 Registration

| ID | Pri | Requirement |
| --- | --- | --- |
| REQ-REG-001 | Must | The system shall compute a rigid `trackerToImage` using paired-point SVD over corresponding fiducial pairs. |
| REQ-REG-002 | Must | Prototype fiducial inputs may be canned (shipped pairs); interactive landmark picking is out of scope. |
| REQ-REG-003 | Must | The system shall compute FRE (fiducial registration error) and per-landmark residuals (mm miss per fiducial after transform). |
| REQ-REG-004 | Must | The system shall display FRE and per-landmark residuals. |
| REQ-REG-005 | Must | Navigation mode (live guidance overlays treated as active navigation) shall be enabled only when FRE is below a configured threshold. |

### 3.4 Tool overlay and guidance

| ID | Pri | Requirement |
| --- | --- | --- |
| REQ-GUI-001 | Must | The system shall overlay tool tip and shaft on all three MPR views. |
| REQ-GUI-002 | Must | The system shall display distance-to-target relative to the streamed TRAJECTORY target. |
| REQ-GUI-003 | Must | The system shall display angular deviation between the tool shaft and the streamed TRAJECTORY axis (entry → target). |
| REQ-GUI-004 | Should | The system shall provide a probe’s-eye view driven by the same `SceneModel` as the MPR views. |
| REQ-GUI-005 | Should | The system shall support a second-display surgeon window driven by the same `SceneModel`. |
| REQ-GUI-006 | Should | The control panel shall use touch-friendly hit targets and gesture handlers. |

### 3.5 Safety and telemetry

| ID | Pri | Requirement |
| --- | --- | --- |
| REQ-SAFE-001 | Must | If no tool pose arrives within approximately 100 ms, the system shall grey the tool overlay and raise an alert rather than leave a frozen pose appearing live. |
| REQ-SAFE-002 | Must | The system shall present a distinct no-trajectory-received / no-plan state when TRAJECTORY has not been received. |
| REQ-SAFE-003 | Must | The system shall show an alert banner for safety degradations in REQ-SAFE-001 and REQ-SAFE-002. |
| REQ-SAFE-004 | Must | The status bar shall show tracker state, line-of-sight (as provided by the stream/sim), FRE, end-to-end pose-to-display latency, frame time, and dropped frames. |

### 3.6 Product framing and build

| ID | Pri | Requirement |
| --- | --- | --- |
| REQ-META-001 | Must | The application shall present a clear not-a-medical-device / not-for-clinical-use disclaimer at startup. |
| REQ-META-002 | Must | Continuous integration shall build the project on x86_64 and on real aarch64 (`ubuntu-latest` and `ubuntu-24.04-arm`). |
| REQ-META-003 | Should | Unit tests and an offscreen render test shall run in CI; interactive frame-rate claims shall state measurement host (desktop GL vs CI software GL). |

---

## 4. Explicitly out of scope (Cut)

| ID | Item |
| --- | --- |
| CUT-001 | Interactive window/level controls |
| CUT-002 | GPU raycast / full 3D volume view |
| CUT-003 | Interactive landmark picking / pointer-based fiducial capture UI |
| CUT-004 | DICOM reader |
| CUT-005 | Entry corridor / vessel-mask safety checking |
| CUT-006 | Planning station features (trajectory authoring inside this app) |

---

## 5. System context (informative)

```text
navsim  --OpenIGTLink (TRAJECTORY, toolToTracker)-->  IgtlReceiver  -->  SceneModel
canned fiducials  -->  PairedPointRegistration  -->  trackerToImage + FRE  -->  SceneModel
NIfTI  -->  voxelToImage + VolumeTexture  -->  MprView x3 / ProbeEyeView
SceneModel  -->  StatusBar + AlertBanner
```

`SceneModel` is the single mutable source of truth; views render it only.

---

## 6. Traceability (placeholder)

Later: a generated requirements-to-tests matrix keyed by these IDs (e.g. test names containing `REQ-…`). Until then, new tests should name or document the ID they cover.
