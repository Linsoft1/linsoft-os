# Linsoft OS Live + installer ISO

`build-live-iso` turns a closed Linsoft QCOW2 image (default:
`Linsoft-Desktop-Native.qcow2`) into a hybrid ISO, `Linsoft-OS-Live.iso`, that
boots from DVD or USB on BIOS and UEFI computers. The source image is attached
read-only and is never modified.

## Boot menu

| Entry | Purpose |
| --- | --- |
| Linsoft OS Live - vyskusat bez instalacie | Full desktop running from the medium; all changes are kept in RAM and lost on shutdown. |
| Instalovat Linsoft OS na disk | Boots straight into the text installer on the screen. |
| Linsoft OS Live - nacitat do RAM | Copies the system (~1.6 GB) to RAM first, then the medium can be removed. |
| Linsoft OS Live - diagnostika / konzola | Text console with boot messages. |

The installer is also available inside the Live desktop: search Start for
**Instalovat Linsoft OS** (a launcher is also placed in `~/Desktop`). It opens
on its own full-screen console and returns to the desktop when finished.

## Installer

`linsoft-install` erases the whole selected disk (minimum 12 GiB; the Live
medium itself is never offered) after the user types `ANO`, then creates:

1. 1 MiB BIOS boot partition (GRUB for BIOS),
2. 512 MiB EFI System Partition (FAT32, `EFI/BOOT/BOOTX64.EFI`),
3. ext4 root partition labelled `Linsoft` using the rest of the disk.

It copies the pristine system from the squashfs (changes made in the Live
session are not copied), writes `fstab` by UUID, points GRUB at the new
`PARTUUID`, generates new machine IDs and installs GRUB for BIOS and UEFI. The
EFI loader uses the removable fallback path, so no NVRAM entry is needed.
Secure Boot must be disabled because the loader and kernel are unsigned.
Dual boot and custom partitioning are not supported.

The installed system is the same Native edition as the source image, including
its autologin/passwordless fixed-action opt-ins (Native requires autologin).
Anyone with physical access can use the desktop and the fixed system helpers,
so do not install it on a shared computer without reviewing
[Passwords and recovery](../../README.md#passwords-and-recovery).

## How it works

* `build-iso-kernel` copies the LFS kernel tree to
  `/mnt/lfs/sources/linsoft-iso-kernel` and enables squashfs (with xattrs),
  overlayfs, NVMe, USB UAS, SD/MMC and simpledrm as built-ins. The original
  tree is left untouched. The ISO kernel replaces `/boot/linsoft-kernel` in the
  ISO system only.
* `initramfs-init` (static BusyBox) finds the medium by the build-specific
  `/linsoft/live/<id>.id` marker, mounts `rootfs.squashfs` under a RAM
  overlayfs and adds the Live-only files from `live-overlay/` to the RAM layer.
* `live-overlay/etc/rc.d/init.d/linsoft-live` is a Live-only root service that
  opens the installer on VT9 when the desktop launcher requests it.
* The ISO copy is cleaned of shell histories, caches, the Firefox profile,
  SSH directories, logs and downloaded APT packages; the Debian machine IDs
  are regenerated.

## Building

Run as root in WSL/Linux (Ubuntu 24.04 packages: `qemu-utils squashfs-tools
xorriso grub-pc-bin grub-efi-amd64-bin grub-common mtools busybox-static
dosfstools rsync`, plus `ovmf netcat-openbsd qemu-system-x86` for testing):

```bash
cd "/mnt/c/Linsoft os  pro/linsoft-desktop/linsoft/iso"
LINSOFT_LFS_BUILD=1 bash build-iso-kernel     # once, or after kernel changes
bash build-live-iso                           # [IMAGE.qcow2] [OUTPUT.iso]
bash test-live-iso "/mnt/c/Linsoft os  pro/Linsoft-OS-Live.iso"
```

The source image must not be running in QEMU. A `.sha256` file is written next
to the ISO. Write it to a USB stick with Rufus (DD mode), balenaEtcher or
`dd`; the stick is erased.

`test-live-iso` boots the ISO in QEMU on BIOS and UEFI, drives the installer
with key presses onto an empty 20 GB virtual disk and boots the installed disk
on BIOS and UEFI. Screenshots and serial logs stay in
`/var/tmp/linsoft-iso-test`. It is a virtual-machine test only; real hardware
(graphics, Wi-Fi, firmware quirks) has not been tested.
