# Third-party licences

Adrenoline itself is GPL-3.0-or-later. Components below keep their own terms.

| Component | Licence | Use |
| --- | --- | --- |
| Mesa 3D — NIR, GLSL compiler, ir3 backend | MIT | shader compilation |
| freedreno (Rob Clark and contributors) | MIT | IR3 provenance, hardware facts |
| LLVM / Clang | Apache-2.0 WITH LLVM-exception | AIR parsing |
| Apple XNU / IOKit headers | APSL-2.0 | kernel-side interfaces, if linked |
| EDK2 | BSD-2-Clause-Patent | boot, if linked |
| Qualcomm published kernel sources | GPL-2.0-only | **reference only, not linked** |

## Notes

- GPL-2.0-only material is incompatible with GPL-3.0 and is deliberately kept
  outside this repository. See [docs/LEGAL.md](docs/LEGAL.md).
- Apple binaries are never distributed here. The user supplies them from their
  own device or IPSW.
- When code is taken from a permissively licensed source, its copyright notice
  must be preserved in the file header and repeated here.
