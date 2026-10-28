# Legal and provenance policy

This document is a development rule, not legal advice.

## 1. Clean-room with respect to unlicensed projects

Several useful projects in this space carry per-file SPDX identifiers but **no
top-level licence**. In that situation every file without an SPDX header is
all-rights-reserved by default, and "we rewrote it ourselves after reading it"
does not make a copy non-infringing.

Rule: **we do not copy files that lack an explicit licence.** Design ideas,
hardware facts, register values taken from vendor documentation, and behaviour
observed on hardware are fair to learn from. Expression is not.

## 2. What we may reuse

Only with an explicit, compatible licence, and only with attribution retained:

| Source | Licence | Rule |
| --- | --- | --- |
| Mesa (NIR, GLSL compiler, ir3) | MIT | reuse, keep notice |
| freedreno kernel tree | MIT | reuse, keep notice |
| Apple open source (XNU, IOKit headers) | APSL-2.0 | reuse per APSL terms |
| LLVM | Apache-2.0 WITH LLVM-exception | reuse |
| UniXNU and its generic components (build driver, toolchain, `ArmDTPlatform`, `ArmCpu`, `RamBlock`, `DarwinBoot`) | BSD-2-Clause-Patent | reuse, keep notice; see ADR-0004 |
| UniXNU device repositories (`chiron`, `cheeseburger`, ...) | per-repository | **reference only** — our board files are ours |
| Qualcomm published kernel sources | GPL-2.0-only | **reference only** |

## 3. GPL-2.0-only is a hard boundary

GPL-2.0-only is **not** compatible with GPL-3.0. Linux-derived MSM/DRM sources
therefore must not be copied into this repository. They may be read, and the
hardware facts they document may be used, but no code moves across.

## 4. Apple material

Never committed, never redistributed:

- IPSW contents, dyld shared cache, `metallib`, `kernelcache`
- Apple kexts or userland binaries
- device trees extracted from a firmware image

The build fetches or accepts these from the **user's own device or own IPSW**
at build time. Nothing of Apple's is stored here.

## 5. Contributions

By opening a pull request you assert that the code is your own work and is
licensable under GPL-3.0-or-later, or that it comes from a source in the table
above with its notice preserved.
