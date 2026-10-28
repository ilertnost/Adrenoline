# ADR-0001 — NIR as the interchange, two backends behind one seam

- Status: accepted
- Date: 2026-10-29

## Context

We want the Metal/iOS graphics stack to render on an Adreno 540 (a5xx,
MSM8998) inside a ported XNU. Three routes were on the table:

1. Metal -> SPIR-V -> Vulkan driver.
2. Metal -> NIR -> native PM4 command submission.
3. Metal -> NIR (as GLSL) -> Mesa Gallium freedreno -> GLES.

The Vulkan route has a hard blocker: **Turnip targets a6xx and newer.** Mesa
states there are no plans to support a5xx. Any Vulkan backend for a5xx would be
written from scratch, on a driver design (SQE, bindless descriptors, cluster
pipelining) that assumes a6xx-era hardware.

## Measurements that decided it

| | iPhone 8 (A11) | Mi 6 (MSM8998) |
| --- | --- | --- |
| GPU FP32 | ~409 GFLOPS | ~567 GFLOPS |
| CPU | 2x Monsoon 2.39 GHz + 4x Mistral | 4x Kryo 280 2.45 GHz + 4x 1.9 GHz |
| Effective single-thread | baseline | roughly 2x slower |

The GPU is not the bottleneck. The CPU side of frame preparation is, and so is
the 16 KiB XNU page size translating through a 4 KiB GPU SMMU.

Vulkan optimises CPU submission overhead for workloads with thousands of draw
calls per frame. A compositor issues hundreds. The optimisation does not apply;
the porting cost does.

## Decision

- **Drop Vulkan and SPIR-V entirely.** No Turnip, no Vulkan backend, no SPIR-V
  intermediate.
- **NIR is the seam.** The frontend lowers AIR and MSL to NIR. Backends consume
  NIR. The seam is the only ABI that matters.
- **Two backends, one seam.**
  - `backends/gallium` — Mesa freedreno a5xx behind GLES 3.2. Mature, stable,
    ships first, and doubles as the reference oracle.
  - `backends/pm4_a5xx` — native PM4 submission. Higher ceiling, more of the
    pipeline owned by us, enabled once it matches the oracle.

## Consequences

Positive:

- Correctness work becomes measurable: the same NIR rendered by both backends
  gives a pixel diff, so a regression has an unambiguous answer.
- We never depend on an a5xx Vulkan driver existing.
- The Gallium backend can ship a working desktop before PM4 is ready.
- Shader work (AIR lowering) is shared and backend-independent.

Negative:

- Two backends is more surface area than one.
- Gallium's stateful API does not map cleanly onto Metal render passes; load and
  store actions, fences and the memory model need explicit bridging.
- The PM4 backend must own tiling, resolves, blits and barriers, which Gallium
  would otherwise provide.

## Rejected alternatives

- **Metal -> Vulkan -> Turnip ported to a5xx.** Highest cost, unproven result,
  and Mesa has explicitly declined the target.
- **Gallium only.** Cheapest path to working pixels, but surrenders direct
  control of the command stream and caps the ceiling we care about.
- **PM4 only.** Best ceiling, but leaves no oracle, and a compositor with no
  reference implementation is very expensive to debug.
