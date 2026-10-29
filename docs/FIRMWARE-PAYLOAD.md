# The payload volume and how it reaches the kernel

The firmware boots, UniXNU shows a menu, DarwinBoot hands XNU a Mach-O and a
device tree. This document is the part in between: how the kernel, its device
tree and the menu configuration get into RAM when the phone has no storage
driver at all.

## Why the payload travels inside the boot image

`edk2-msm8998` has no eMMC and no UFS driver. Nothing in the firmware can read a
partition: the Android bootloader (ABL) reads `boot`, loads it, and that is the
whole of the storage story. So the payload has to arrive the same way the
firmware does, inside the boot image.

An Android boot image has three loadable parts: kernel, ramdisk and second.
This is what we use:

| part | contents | address |
|---|---|---|
| kernel | `gzip(UEFI FD) + Android-sagit.dtb` | `0x10000000` |
| ramdisk | the UniXNU payload volume (FAT16) | `0xE0000000` |
| second | unused | — |

The firmware's own copy lands at `0xD0000000` (6 MiB), so `0xE0000000` is clear
of it and still well inside the DRAM the platform describes.

`0xE0000000` matters for a second reason: it is inside a region the firmware
maps as normal memory. A ramdisk placed below `0x80000000` — the usual Android
`0x11000000` — sits outside every region the MMU knows about in DXE, and
touching it there faults. This is the kind of problem that shows up as a random
data abort minutes into bring-up, so the address is chosen deliberately.

## Who publishes it

ABL copies the ramdisk to the address in the header and writes
`linux,initrd-start` / `linux,initrd-end` into `/chosen` of the device tree it
passes in `x0`. `edk2-msm8998` keeps that pointer at `PcdFdtStoreAddress`
(`0x83300000`), and the firmware later publishes the same tree as the
`gFdtTableGuid` configuration table.

`firmware/AdrenolinePkg/Drivers/RamDiskDxe` (ours, GPL-3.0-or-later) reads those
two properties, claims the pages with an exact-address allocation so the
allocator can never hand them to somebody else, and installs a read-only
`EFI_BLOCK_IO_PROTOCOL` over them. `PartitionDxe` and `FatDxe` are already in
the platform, so the volume appears as a normal `SimpleFileSystem` and UniXNU
finds `EFI/UniXNU/unixnu.toml` without knowing any of this.

The driver carries its own device-tree walker (about eighty lines) instead of
linking `FdtLib`. Two reasons: it must work before anything that owns a device
tree object has initialised, and it only ever needs one node.

`UniXNURamDiskDxe` from the UniXNU package does the same job on Mu-Silicium, but
it takes the tree pointer from a SEC handoff HOB that this EDK2 port does not
create. It is left out of the build; see `docs/EDK2-SAGIT.md`.

## What goes in the volume

```
/EFI/UniXNU/unixnu.toml
/EFI/UniXNU/Payloads/kernel.macho
/EFI/UniXNU/Payloads/kernel.macho.adt
```

`tools/mksagit-payload.py` builds it: FAT16, label `ADRENOLINE`, timestamps
pinned to the FAT epoch so the same inputs give the same bytes, 24 MiB by
default. The configuration is one `xnu` entry plus `Restart` and `Power off`,
which is what the built-in menu carries.

```sh
python3 tools/mksagit-payload.py \
  --kernel out/sagit/tree/build/xnu/ios-obj/DEVELOPMENT_ARM64_BCM2837/mach.development.bcm2837.unstripped.noctf \
  --adt <generated adt> \
  --output build/payload.img

PAYLOAD=build/payload.img tools/build-sagit-edk2.sh
```

Without `PAYLOAD` the image boots to the UniXNU menu with nothing in it, which
is still a useful test: it proves the boot manager and the loader are in the
firmware and running.

## Budget

The `boot` partition is 64 MiB (`BOARD_BOOTIMAGE_PARTITION_SIZE` in the
LineageOS device tree for sagit). What is spent:

| | bytes |
|---|---|
| kernel part, gzipped FD | 5 632 973 |
| kernel part, Android device tree | 5 468 482 |
| ramdisk, 24 MiB payload | 25 165 824 |
| header and padding | ~4 000 |
| **image** | **36 270 080** |

That leaves about 27 MiB. It is not enough for the Apple restore ramdisk
(106 954 752 B raw, 83 582 628 B even with `xz -9`, and the image would need
both): the APFS root cannot arrive this way. It has to come from storage, which
is where the firmware stages in `docs/ADR-0005-firmware-stages.md` come in.

## What has been proven, and what has not

Proven on the host: the image is built reproducibly, the header carries the
addresses above, the payload volume is readable with `mdir`, and the module is
in the firmware volume (`FV/Guid.xref`).

Not proven yet: the handoff on hardware. The open questions in order of
likelihood:

1. Whether this ABL writes `linux,initrd-start`/`-end` for a non-Android payload.
   If it does not, `RamDiskDxe` finds no `/chosen` initrd and the menu comes up
   empty — that is the diagnostic, not a crash.
2. Whether the pages at `0xE0000000` are free when the driver claims them.
   If they are not, the driver says so and publishes anyway; the payload is then
   readable but not protected.
3. Whether the bootloader accepts a 34.6 MiB boot image. Nothing in the header
   suggests it would not, but this device has only ever been flashed with 11 MiB.
