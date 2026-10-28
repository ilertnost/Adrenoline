# Mesa host toolchain — what works and what does not

Recorded 2026-10-29 against Mesa 26.2.6 on x86-64.
Reproduce with `tools/build-mesa-host.sh`.

## Configuration

```
-Dbuildtype=release -Dwerror=false
-Dgallium-drivers=freedreno -Dvulkan-drivers=
-Dglx=disabled -Degl=disabled -Dgles1=disabled -Dgles2=disabled
-Dtools=glsl,nir,freedreno,drm-shim
-Dllvm=disabled -Dzstd=disabled
```

Desktop GLSL, not GLES. The AIR and MSL lowering path targets **GLSL 430**, so
the desktop frontend is the one that matters. The GLES frontend is not needed
and `glsl_compiler --es` is not even compiled in with these flags.

Host requirements beyond a C/C++ toolchain: `mako`, `packaging`, `pyyaml` in a
Python environment. Without `mako` the build stops at configuration; without
`packaging` it stops one step later.

## Verified

| Step | Result |
| --- | --- |
| Mesa 26.2.6 configures and builds on GCC 16 / Clang 23 | ok |
| `glsl_compiler` builds | ok, 14 MB |
| GLSL 430 -> NIR text | ok, see `tests/shaders/ca_like.frag` |
| `freedreno_noop_drm_shim.so` builds | ok |
| Shim recognises Adreno 540 | ok, `gpu_id=540 chip_id=0x5040002` |
| Mesa still contains the a5xx Gallium driver | ok, `src/gallium/drivers/freedreno/a5xx/` |
| IR3 still targets a4xx/a5xx | ok, generation checks present |

## The gap that matters

```
$ FD_GPU_ID=540 LD_PRELOAD=.../libfreedreno_noop_drm_shim.so \
    ./computerator -d -f examples/simple.asm
computerator: unsupported gpu generation: a5xx: Invalid argument
got gpu: FD540
```

`computerator` has `a4xx.cc` and `a6xx.cc` and no a5xx submission path. So
**Mesa's own host harness cannot execute anything on an emulated a5xx**, even
though its compiler can generate a5xx code.

Consequence for us: there is no off-the-shelf way to run a5xx machine code on a
host. We build our own offline harness (`tools/ir3a5xx`): open the shim device,
create an IR3 compiler for FD540, compile a NIR module, disassemble the result.

That harness is also the natural place to put the backend-agnostic shader tests
for the NIR seam, so it is not throwaway work.

## Not verified yet

- NIR -> IR3 for a5xx outside a running GL context.
- Any Gallium draw on a5xx. The noop shim stubs submission, so nothing executes.
- Anything on real hardware.
