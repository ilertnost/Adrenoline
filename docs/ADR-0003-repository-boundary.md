# ADR-0003 — The GPU stack lives in this repository, and only in this one

- Status: accepted
- Date: 2026-11-07

## Facts

- The work is split across two repositories. `unixnu-sagit` carries the boot
  chain that makes XNU run on the Mi 6: EDK2 firmware pieces, UniXNU and
  DarwinBoot, the ADT, the payload volume, the device tree and the XNU build.
  Adrenoline carries the graphics stack.
- The boot chain needs to draw *something* long before a GPU driver exists.
  The bring-up ladder, the panic screen and the firmware menu all write pixels
  into the framebuffer with no GPU involved, and they have to keep working
  when the driver is broken, which is most of the time.
- The GPU driver is not a boot-time component. It is an IOKit driver that
  needs a working kernel, memory management, interrupt delivery and a device
  tree node to attach to. It is also the part of the system that knows what
  Adreno 540 actually is, which is the whole subject of this repository.

## Decision

1. **Everything that talks to the GPU lives in Adrenoline**, including the
   XNU driver: the IOKit device, firmware loading and authentication, SMMU
   programming, ring submission for the a5xx command processor, memory and
   IOSurface objects, the Metal frontend (`metal2nir`), the NIR seam, the
   `gallium` and `pm4_a5xx` backends and the offline tooling
   (`tools/ir3a5xx`, `tools/mtlbinfo.py`).
2. **`unixnu-sagit` keeps the boot chain and, at most, basic software
   rendering.** Software rendering means writing pixels into a framebuffer:
   the bring-up ladder, console text, a blit, a simple rasteriser used to put
   something on the panel before the GPU works. It never means shader
   compilation, command submission or GPU register programming.
3. **The dependency direction is fixed.** Adrenoline consumes the kernel that
   `unixnu-sagit` builds; `unixnu-sagit` consumes the kexts that Adrenoline
   builds. Neither repository contains the other's source, and neither reaches
   into the other's tree at build time.
4. **A register poke is still the GPU stack.** When Adrenoline needs a fact
   from the hardware during bring-up, the code that uses it belongs here, even
   if it is ten lines long.

## Consequences

- The kernel-side GPU driver ships as a kext built from this tree; the payload
  builder in `unixnu-sagit` takes it as an input artifact, exactly like the
  kernel and the ADT.
- Bring-up diagnostics never depend on the GPU being functional, which is what
  keeps them useful while the driver is being written.
- A change that needs to alter both sides is two commits in two repositories,
  not one. That is the price of keeping the boot chain shippable without the
  GPU and the GPU shippable without the boot chain.
- The split also keeps the GPLv3 graphics tree free of EDK2 firmware code, and
  the boot chain free of anything derived from Apple's GPU driver.
