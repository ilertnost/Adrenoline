# Getting at IPSW contents on Linux

Everything here was verified on Arch Linux, GCC 16.2.1, CMake 4.4.4, against
`iPhone_4.7_P3_16.7.16_20H392_Restore.ipsw` (5.7 GB, iOS 16.7.16, `D20AP`).

## 1. The `ipsw` CLI

Do not use `releases/latest/download/ipsw_linux_amd64.tar.gz` — that asset name
does not exist and returns a 9-byte "Not Found" body. The release carries two
different tools whose names both end in `linux_x86_64.tar.gz`:

- `ipswd_*` — the daemon, not what we want;
- `ipsw_*` — the CLI.

```sh
URL=$(curl -sS https://api.github.com/repos/blacktop/ipsw/releases/latest |
  python3 -c 'import sys,json
for a in json.load(sys.stdin)["assets"]:
    n=a["name"]
    if n.startswith("ipsw_") and n.endswith("linux_x86_64.tar.gz"):
        print(a["browser_download_url"]); break')
curl -sSL -o ipsw.tar.gz "$URL" && tar xzf ipsw.tar.gz
./ipsw version   # 3.1.739
```

## 2. APFS mounting needs `apfs-fuse`

`ipsw extract --files` and `ipsw extract --dyld` both mount the encrypted-APFS
DMG. Without a helper they fail with *"failed to find apfs-fuse"*. Point ipsw at
our build with `IPSW_APFS_FUSE_PATH`.

`apfs-fuse` is not in the Arch repos, so build it. Two things break the upstream
tree with a modern toolchain:

1. CMake 4 rejects `cmake_minimum_required` below 3.5 →
   `-DCMAKE_POLICY_VERSION_MINIMUM=3.5`.
2. `ApfsLib/PList.h` relies on a transitive `<cstdint>` include that GCC 16 no
   longer provides → add `#include <cstdint>`.

```sh
sudo pacman -S --needed base-devel cmake fuse3 git
git clone --recursive https://github.com/sgan81/apfs-fuse
cd apfs-fuse/build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_POLICY_VERSION_MINIMUM=3.5
make -j16
```

## 3. Working invocation

```sh
export TMPDIR=/data/opencode/tmp                       # never /tmp: it is a small tmpfs
export IPSW_APFS_FUSE_PATH=/data/opencode/tools/apfs-fuse/build/apfs-fuse

IPSW=~/Загрузки/iPhone_4.7_P3_16.7.16_20H392_Restore.ipsw

# every .metallib in the system volume
ipsw extract --files --pattern '.*\.metallib$' -o fs "$IPSW"

# the arm64 dyld shared cache (split into .NN plus .dylddata/.dyldlinkedit)
ipsw extract --dyld --dyld-arch arm64e -o dsc "$IPSW"

# the Apple GPU driver bundle
ipsw extract --files --pattern '.*AGX.*' -o agx "$IPSW"
```

`TMPDIR` matters: ipsw stages the DMG under `$TMPDIR`, and the DMG is 2.6 GB.

## 4. What came out

| artifact | contents |
|---|---|
| `fs/.../QuartzCore.framework/default.metallib` | 141 AIR functions, 829 968 bytes — the compositor's shader set |
| `fs/.../**.metallib` | 140 libraries total across the system |
| `dsc/.../dyld_shared_cache_arm64.*` | split cache, 43 parts |
| `agx/.../AGXMetalA11.bundle` | `com.apple.AGXMetalA11`, 3.3 MB, 43 files, almost all `N_M.bin` |

`AGXMetalA11.bundle` lives in `/System/Library/Extensions/`, not a framework. It
is Apple's GPU microcode plus the user client for Apple's own GPU; it contains
no information that maps onto Adreno 540 command packets.

See `docs/CA-SHADER-INVENTORY.md` for the compositor shader list and
`tools/mtlbinfo.py` for the container parser.
