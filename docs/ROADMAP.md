# Roadmap

Ordered by what unblocks the most work per unit of effort.

## M-1 — boot chain (moved)

The boot chain is a separate project: **unixnu-sagit**
(<https://github.com/ilertnost/unixnu-sagit>) — EDK2 firmware pieces, payload,
ADT, device tree and the XNU build for sagit. This repository only consumes the
resulting XNU.

## M0 — seam and contract

- [x] Public backend vtable and device caps
- [x] NIR handoff representation
- [x] Licence and provenance policy
- [x] Stub backend compiles clean on x86-64
- [x] Host test: registry, capability negotiation, teardown
- [ ] Stub backend compiles clean on aarch64

## M1a — host shader toolchain

- [x] Mesa 26.2.6 builds on host with a documented flag set (`docs/MESA-HOST.md`)
- [x] GLSL 430 -> NIR verified (`tests/shaders/ca_like.frag`)
- [x] Adreno 540 recognised by the freedreno noop DRM shim (`chip_id=0x5040002`)
- [x] Confirmed `computerator` has no a5xx submission path
- [ ] `tools/ir3a5xx`: NIR -> IR3 -> disassembly for FD540, offline

## M1b — Gallium reference backend

- [ ] Build Mesa freedreno for a5xx as a host library
- [ ] NIR -> `pipe_context` translation for the compositor's pipeline shapes
- [ ] Offscreen render to an IOSurface-shaped buffer
- [ ] Pixel-exact reference images checked into `tests/ref/`

## M2 — Metal frontend (metal2nir)

- [x] AIR container parse (metallib -> per-function modules): `tools/mtlbinfo.py`
- [x] Scope fixed by the real workload: 141 functions in QuartzCore's
      `default.metallib` (`docs/CA-SHADER-INVENTORY.md`)
- [ ] Function constants and specialisation
- [ ] AIR -> NIR lowering for the compositor's known pipelines
- [ ] MSL source path for development shaders
- [ ] AOT catalog: compile once, cache by pipeline key

## M3 — native PM4 backend

- [ ] SMMU 4 KiB context, command ring, MAU/fence
- [ ] State packets for the pipeline shapes from M2
- [ ] Draw and blit paths
- [ ] Diff against M1 reference images, byte for byte
- [ ] Switch default backend when the diff is clean

## M4 — integration

- [ ] Backend selection at runtime
- [ ] Fence and residency semantics matching Metal expectations
- [ ] Frame pacing: measure the CPU side, which is the actual limiter

## Non-goals

- Vulkan
- SPIR-V
- Turnip
- Any dependency on unlicensed third-party source
