# ADR-0002 — Target Metal 2 by capability reporting, not by porting Apple code

- Status: accepted
- Date: 2026-10-29
- Context: iOS 16.7.16 `20H392` on iPhone 8 (`D20AP`)

## Facts

- A11 Bionic is Apple GPU family **Apple4**. Metal 2 shipped with it.
- Apple's feature-set tables put Metal 3 at **A13 or later**. A11 therefore
  gets the Metal 2 API surface, not Metal 3.
- The device tree of `20H392` for `D20AP` states it directly:

  ```
  graphics-featureset-class     = "APPLE4"
  graphics-featureset-fallbacks = "APPLE3:APPLE3v1:APPLE2:APPLE1:GLES2,0"
  ```

  So the firmware itself settles both questions: Apple4, and no Metal 3
  feature set anywhere in the fallback chain.
- The userspace frameworks are fixed binaries. We cannot present them with a
  different "Metal version"; they are built once and shipped.
- `AGXMetalA11.bundle` (`com.apple.AGXMetalA11`) is 3.3 MB of Apple GPU
  microcode and an IOGPU user client. It describes Apple's GPU, not Adreno 540.

## Decision

1. **Report a conservative feature set.** The only lever the system actually
   honours is what our `MTLDevice` answers: `supportsFamily:`,
   `supportsFeatureSet:`, and the limit properties (`argumentBuffersSupport`,
   `rasterOrderGroupsSupported`, `readWriteTextureSupport`, `maxSampleCount`,
   `maxThreadsPerThreadgroup`, `maxBufferLength`). Answer for Apple4/Metal 2 and
   the compositor takes the Metal 2 paths by itself.
2. **Do not transplant Apple driver code.** Neither decompiled nor
   re-implemented from a disassembly. Two reasons: it targets a different ISA,
   and it is the one thing that would poison a public GPLv3 repository.
3. **Derive scope from the workload, not from the API.** The compositor's shader
   set is a concrete artifact we can enumerate offline. That set is the target
   (`docs/CA-SHADER-INVENTORY.md`).
4. **Instrument rather than reverse.** Where the exact API subset is unknown,
   our own device logs what the system asks for. Observations of our own process
   are both cheaper and more accurate than reading a disassembly.

## Consequences

- No dependency on Apple binaries at runtime, and nothing Apple-derived in the
  tree.
- The shader target is finite and countable: 141 functions, 39 vertex,
  95 fragment, 7 kernel, 830 KB of AIR.
- Feature reporting becomes a tested contract: if the system takes a path we
  have not implemented, the log says so directly.
