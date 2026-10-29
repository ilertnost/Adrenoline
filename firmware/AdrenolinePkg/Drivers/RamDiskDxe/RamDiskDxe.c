/** @file
  Publish the payload volume that the Android bootloader placed in RAM as a
  read-only block device, so that FatDxe can mount it and UniXNU can read its
  configuration and kernel.

  The boot image this firmware ships in is packed with a ramdisk; ABL copies it
  to the address in the header and writes linux,initrd-start / linux,initrd-end
  into the device tree it hands over. edk2-msm8998 stores that device tree
  pointer at PcdFdtStoreAddress, so the bounds are read from the same tree the
  rest of the platform uses -- no address is hardcoded here.

  Copyright (c) 2026 ilertnost
  SPDX-License-Identifier: GPL-3.0-or-later
**/

#include <Uefi.h>

#include <Guid/Fdt.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/BlockIo.h>
#include <Protocol/DevicePath.h>

#define FDT_MAGIC          0xd00dfeedU
#define FDT_BEGIN_NODE     1U
#define FDT_END_NODE       2U
#define FDT_PROP           3U
#define FDT_NOP            4U
#define FDT_END            9U

#define RAMDISK_BLOCK_SIZE 512U

#pragma pack(1)
typedef struct {
  VENDOR_DEVICE_PATH          Vendor;
  EFI_DEVICE_PATH_PROTOCOL    End;
} RAMDISK_DEVICE_PATH;
#pragma pack()

STATIC RAMDISK_DEVICE_PATH  mDevicePath = {
  {
    { HARDWARE_DEVICE_PATH, HW_VENDOR_DP, { (UINT8)sizeof (VENDOR_DEVICE_PATH), (UINT8)(sizeof (VENDOR_DEVICE_PATH) >> 8) } },
    { 0x5E6A1D42, 0x1C3B, 0x4A87, { 0x9F, 0x2E, 0x6B, 0x51, 0xC4, 0x0D, 0x77, 0xA8 } }
  },
  { END_DEVICE_PATH_TYPE, END_ENTIRE_DEVICE_PATH_SUBTYPE, { sizeof (EFI_DEVICE_PATH_PROTOCOL), 0 } }
};

STATIC UINT8                    *mRamBase;
STATIC EFI_BLOCK_IO_MEDIA       mMedia;

/**
  Big-endian readers. A device tree is big-endian regardless of the host.
**/
STATIC
UINT32
Fdt32 (
  IN CONST UINT8  *P
  )
{
  return ((UINT32)P[0] << 24) | ((UINT32)P[1] << 16) | ((UINT32)P[2] << 8) | (UINT32)P[3];
}

STATIC
UINT64
Fdt64 (
  IN CONST UINT8  *P
  )
{
  return ((UINT64)Fdt32 (P) << 32) | (UINT64)Fdt32 (P + 4);
}

/**
  Read an address property whose width depends on the tree: two cells or one.
**/
STATIC
UINT64
FdtAddress (
  IN CONST UINT8  *Data,
  IN UINT32       Length
  )
{
  if (Length >= 8) {
    return Fdt64 (Data);
  }

  if (Length >= 4) {
    return Fdt32 (Data);
  }

  return 0;
}

/**
  Find linux,initrd-start and linux,initrd-end in /chosen.

  A hand-written walk, because this must work before any library that owns a
  device tree object is initialised, and because it only ever needs one node.

  @param[in]  Fdt    Pointer to a flattened device tree.
  @param[out] Start  First byte of the initrd.
  @param[out] End    One past the last byte of the initrd.

  @retval EFI_SUCCESS            Both properties were found.
  @retval EFI_NOT_FOUND          The tree has no /chosen node with an initrd.
  @retval EFI_INVALID_PARAMETER  The blob is not a device tree.
**/
STATIC
EFI_STATUS
FindInitrd (
  IN  CONST UINT8  *Fdt,
  OUT UINT64       *Start,
  OUT UINT64       *End
  )
{
  UINT32        TotalSize;
  UINT32        StructOffset;
  UINT32        StringsOffset;
  CONST UINT8   *Cursor;
  CONST UINT8   *StructEnd;
  CONST UINT8   *Strings;
  CONST CHAR8   *Name;
  UINT32        Depth;
  BOOLEAN       InChosen;
  UINT64        FoundStart;
  UINT64        FoundEnd;

  if ((Fdt == NULL) || (((UINTN)Fdt & 7) != 0)) {
    return EFI_INVALID_PARAMETER;
  }

  if (Fdt32 (Fdt) != FDT_MAGIC) {
    return EFI_INVALID_PARAMETER;
  }

  TotalSize     = Fdt32 (Fdt + 4);
  StructOffset  = Fdt32 (Fdt + 8);
  StringsOffset = Fdt32 (Fdt + 12);
  if ((TotalSize < 40) || (StructOffset >= TotalSize) || (StringsOffset >= TotalSize)) {
    return EFI_INVALID_PARAMETER;
  }

  Cursor    = Fdt + StructOffset;
  StructEnd = Fdt + TotalSize;
  Strings   = Fdt + StringsOffset;
  Depth     = 0;
  InChosen  = FALSE;
  FoundStart = 0;
  FoundEnd   = 0;

  while (Cursor + 4 <= StructEnd) {
    UINT32  Token = Fdt32 (Cursor);

    Cursor += 4;

    if (Token == FDT_BEGIN_NODE) {
      Name = (CONST CHAR8 *)Cursor;
      // The name is NUL-terminated and padded to four bytes.
      while ((Cursor < StructEnd) && (*Cursor != 0)) {
        Cursor++;
      }

      while ((Cursor < StructEnd) && (((UINTN)Cursor & 3) != 0)) {
        Cursor++;
      }

      Depth++;
      InChosen = (BOOLEAN)((Depth == 1) && (AsciiStrCmp (Name, "chosen") == 0));
      continue;
    }

    if (Token == FDT_END_NODE) {
      if (Depth == 0) {
        return EFI_INVALID_PARAMETER;
      }

      if (Depth == 1) {
        InChosen = FALSE;
      }

      Depth--;
      continue;
    }

    if (Token == FDT_PROP) {
      UINT32       Length;
      UINT32       NameOffset;
      CONST UINT8  *Value;
      CONST CHAR8  *PropName;

      if (Cursor + 8 > StructEnd) {
        return EFI_INVALID_PARAMETER;
      }

      Length     = Fdt32 (Cursor);
      NameOffset = Fdt32 (Cursor + 4);
      Cursor    += 8;
      Value      = Cursor;
      if ((NameOffset >= TotalSize - StringsOffset) || (Value + Length > StructEnd)) {
        return EFI_INVALID_PARAMETER;
      }

      PropName = (CONST CHAR8 *)(Strings + NameOffset);
      if (InChosen) {
        if (AsciiStrCmp (PropName, "linux,initrd-start") == 0) {
          FoundStart = FdtAddress (Value, Length);
        } else if (AsciiStrCmp (PropName, "linux,initrd-end") == 0) {
          FoundEnd = FdtAddress (Value, Length);
        }
      }

      Cursor = Value + ALIGN_VALUE (Length, 4);
      continue;
    }

    if (Token == FDT_NOP) {
      continue;
    }

    if (Token == FDT_END) {
      break;
    }

    // An unknown token means the offsets are wrong; stop rather than wander.
    return EFI_INVALID_PARAMETER;
  }

  if ((FoundStart == 0) || (FoundEnd <= FoundStart)) {
    return EFI_NOT_FOUND;
  }

  *Start = FoundStart;
  *End   = FoundEnd;
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
RamDiskReset (
  IN EFI_BLOCK_IO_PROTOCOL  *This,
  IN BOOLEAN                ExtendedVerification
  )
{
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
RamDiskReadBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL  *This,
  IN  UINT32                 MediaId,
  IN  EFI_LBA                Lba,
  IN  UINTN                  BufferSize,
  OUT VOID                   *Buffer
  )
{
  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (MediaId != mMedia.MediaId) {
    return EFI_MEDIA_CHANGED;
  }

  if ((BufferSize % RAMDISK_BLOCK_SIZE) != 0) {
    return EFI_BAD_BUFFER_SIZE;
  }

  if ((Lba > mMedia.LastBlock) || ((BufferSize / RAMDISK_BLOCK_SIZE) > (mMedia.LastBlock + 1 - Lba))) {
    return EFI_INVALID_PARAMETER;
  }

  CopyMem (Buffer, mRamBase + (UINTN)Lba * RAMDISK_BLOCK_SIZE, BufferSize);
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
RamDiskWriteBlocks (
  IN EFI_BLOCK_IO_PROTOCOL  *This,
  IN UINT32                 MediaId,
  IN EFI_LBA                Lba,
  IN UINTN                  BufferSize,
  IN VOID                   *Buffer
  )
{
  return EFI_WRITE_PROTECTED;
}

STATIC
EFI_STATUS
EFIAPI
RamDiskFlushBlocks (
  IN EFI_BLOCK_IO_PROTOCOL  *This
  )
{
  return EFI_SUCCESS;
}

STATIC EFI_BLOCK_IO_PROTOCOL  mBlockIo = {
  EFI_BLOCK_IO_PROTOCOL_REVISION,
  &mMedia,
  RamDiskReset,
  RamDiskReadBlocks,
  RamDiskWriteBlocks,
  RamDiskFlushBlocks
};

/**
  Return the device tree the platform received from the bootloader.
**/
STATIC
CONST UINT8 *
PlatformFdt (
  VOID
  )
{
  VOID        *Table;
  UINT64      Store;
  UINT64      Fdt;
  UINTN       Index;

  Table = NULL;
  // There is no lookup helper in this EDK2 generation; walk the table.
  for (Index = 0; Index < gST->NumberOfTableEntries; Index++) {
    if (CompareGuid (&gST->ConfigurationTable[Index].VendorGuid, &gFdtTableGuid)) {
      Table = gST->ConfigurationTable[Index].VendorTable;
      break;
    }
  }

  if ((Table != NULL) && (Fdt32 ((CONST UINT8 *)Table) == FDT_MAGIC)) {
    return (CONST UINT8 *)Table;
  }

  Store = PcdGet64 (PcdFdtStoreAddress);
  if (Store == 0) {
    return NULL;
  }

  // PEI saved x0 here before the firmware copied itself to its load address.
  CopyMem (&Fdt, (CONST VOID *)(UINTN)Store, sizeof (Fdt));
  if ((Fdt == 0) || ((Fdt & 7) != 0)) {
    return NULL;
  }

  return (CONST UINT8 *)(UINTN)Fdt;
}

EFI_STATUS
EFIAPI
AdrenolineRamDiskDxeEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  CONST UINT8 *Fdt;
  UINT64      Start;
  UINT64      End;
  UINTN       Length;
  UINTN       Pages;
  EFI_PHYSICAL_ADDRESS Address;
  EFI_HANDLE  Handle;

  Fdt = PlatformFdt ();
  if (Fdt == NULL) {
    DEBUG ((DEBUG_ERROR, "ADRENOLINE-RAMDISK: no device tree, no initrd\n"));
    return EFI_SUCCESS;
  }

  Status = FindInitrd (Fdt, &Start, &End);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "ADRENOLINE-RAMDISK: no initrd in /chosen (%r)\n", Status));
    return EFI_SUCCESS;
  }

  if (((Start | End) & (RAMDISK_BLOCK_SIZE - 1)) != 0) {
    DEBUG ((DEBUG_ERROR, "ADRENOLINE-RAMDISK: initrd 0x%lx-0x%lx is not sector aligned\n", Start, End));
    return EFI_SUCCESS;
  }

  Length = (UINTN)(End - Start);
  Pages  = EFI_SIZE_TO_PAGES (Length);

  // Claim the bootloader's copy so the allocator can never hand these pages to
  // somebody else while UniXNU and FatDxe are reading them.
  Address = Start;
  Status  = gBS->AllocatePages (AllocateAddress, EfiLoaderData, Pages, &Address);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "ADRENOLINE-RAMDISK: could not claim 0x%lx (%r); publishing anyway\n", Start, Status));
  }

  mRamBase                = (UINT8 *)(UINTN)Start;
  mMedia.MediaId          = 1;
  mMedia.RemovableMedia   = FALSE;
  mMedia.MediaPresent     = TRUE;
  mMedia.LogicalPartition = FALSE;
  mMedia.ReadOnly         = TRUE;
  mMedia.WriteCaching     = FALSE;
  mMedia.BlockSize        = RAMDISK_BLOCK_SIZE;
  mMedia.IoAlign          = 1;
  mMedia.LastBlock        = (EFI_LBA)(Length / RAMDISK_BLOCK_SIZE) - 1;

  Handle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEfiDevicePathProtocolGuid,
                  &mDevicePath,
                  &gEfiBlockIoProtocolGuid,
                  &mBlockIo,
                  NULL
                  );
  DEBUG ((
    DEBUG_ERROR,
    "ADRENOLINE-RAMDISK: initrd 0x%lx, %lu bytes, block device %r\n",
    Start,
    (UINT64)Length,
    Status
    ));
  return EFI_SUCCESS;
}
