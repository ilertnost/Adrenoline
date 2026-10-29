# Adrenoline

Metal shader frontend and GPU runtime for Qualcomm Adreno **a5xx**
(Adreno 530 / 540, MSM8998 — Xiaomi Mi 6 `sagit`).

Adrenoline exists for one reason: the iOS/Metal graphics stack has to run on
Adreno hardware that upstream Mesa does not support. **Turnip is a6xx+ only**,
so there is no Vulkan path for a5xx. Adrenoline therefore does not use Vulkan
at all, and does not use SPIR-V.

## Scope

Adrenoline is the graphics side only: Metal frontend, NIR, the a5xx backends
and the XNU GPU driver. The device-specific boot chain that brings XNU up on
the Mi 6 (EDK2 firmware, UniXNU/DarwinBoot, ADT, payload, device tree) lives in
a separate repository: <https://github.com/ilertnost/unixnu-sagit>.

## The one decision that matters

**NIR is the interchange format. Nothing else.**

```
  Metal            AIR / MSL source
                       |
                       v
   frontend        metal2nir          (frontend/metal2nir)
                       |
                       v
   ---- seam ----      NIR            <-- stable ABI boundary
                       |
              +--------+--------+
              v                 v
   backends  PM4 a5xx       Gallium/freedreno
             (native)       (GLES 3.2 / Mesa)
              |                 |
              v                 v
           Adreno 540 command processor
```

Both backends consume the same NIR. That is deliberate: the mature Mesa
Gallium driver is used as a **pixel-exact reference oracle** for the native
PM4 path, and as the first shipping backend while PM4 is still being
stabilised.

Rationale and numbers: [docs/ADR-0001-interchange-and-backend.md](docs/ADR-0001-interchange-and-backend.md).

## Why not Vulkan

Vulkan's advantage is CPU submission overhead on workloads with thousands of
draw calls. A compositor issues hundreds. The GPU here is not the bottleneck
either — Adreno 540 is roughly 567 GFLOPS FP32 versus ~409 GFLOPS for the GPU
in an iPhone 8. The limiter is the CPU side of frame preparation.

Porting Turnip to a5xx would cost far more than the submission overhead it
could save, and would trade a mature GL driver for an unproven Vulkan one.

## Layout

| Path | Contents |
| --- | --- |
| `include/adrenoline/` | public seam: backend vtable, NIR handoff, device caps |
| `frontend/metal2nir/` | AIR / MSL -> NIR lowering |
| `backends/pm4_a5xx/` | native Adreno 540 command submission |
| `backends/gallium/` | freedreno Gallium bridge (reference oracle) |
| `docs/` | ADRs, legal policy, roadmap |
| `tools/` | offline shader compile, AOT catalog, diff harness |

## Build

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

Only the stub backend builds today. The tree is the seam and the contract, not
a working driver yet.

## Status

Pre-alpha. The seam, the licence policy and the backend contract are in place.
No backend is functional. See [docs/ROADMAP.md](docs/ROADMAP.md).

## Legal

GPL-3.0-or-later for original code in this repository. Third-party components
keep their own licences and are listed in
[THIRD_PARTY_LICENSES.md](THIRD_PARTY_LICENSES.md). Read
[docs/LEGAL.md](docs/LEGAL.md) before contributing — this project is
clean-room with respect to projects that do not carry an explicit licence.

Apple binaries (IPSW contents, dyld shared cache, Metal libraries) are **never**
part of this repository. You supply your own from your own device.
