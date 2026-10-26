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
2. 512 MiB EFI System Partition (FAT32, `EFI/BOOT/` with shim, signed GRUB,
   MokManager and `linsoft-secureboot.cer`),
3. ext4 root partition labelled `Linsoft` using the rest of the disk.

It copies the pristine system from the squashfs (changes made in the Live
session are not copied), writes `fstab` by UUID, points GRUB at the new
`PARTUUID`, generates new machine IDs and installs GRUB for BIOS and UEFI. The
EFI loader uses the removable fallback path, so no NVRAM entry is needed.
Secure Boot may stay enabled (see below). Dual boot and custom partitioning
are not supported.

## Secure Boot

The UEFI boot chain is the one used by Ubuntu-based tools such as Ventoy:

1. `EFI/BOOT/BOOTX64.EFI` is Ubuntu's shim, signed by the *Microsoft UEFI CA
   2011* that almost every PC trusts.
2. Shim starts `grubx64.efi` (GRUB 2.12 with every module built in) and GRUB
   starts `linsoft-kernel`. Both are signed with the Linsoft key.
3. On the first Secure Boot start the Linsoft key is unknown, so shim shows
   "Verification failed" and opens MokManager. Press Enter, then a key within
   10 s, choose **Enroll key from disk → LINSOFT_EFI →
   linsoft-secureboot.cer → Continue → Yes → Reboot**. The key is stored in
   firmware NVRAM, so the Live medium and the installed system then start
   without questions.

Without Secure Boot (or on BIOS) nothing has to be confirmed. GRUB cannot load
fonts under Secure Boot, so its menu is then shown in text mode. Computers
that only trust the newer *Microsoft UEFI CA 2023* or that have "Microsoft
third-party UEFI CA" disabled (some Secured-core PCs) still refuse shim; allow
the third-party CA in the firmware or turn Secure Boot off there.

`build-signed-efi` creates the loader set and signs the kernel. GRUB 2.12
would start a kernel whose PE `MajorImageVersion` is 1 or higher through the
firmware `LoadImage()`, which ignores MOK keys, so the version is set to 0
before signing and GRUB uses its shim-verified EFI handover loader. The
private key is read from `/root/linsoft-secureboot` (`LINSOFT_SB_KEYDIR`);
keep it outside the repository and reuse it for every release, otherwise
users have to enroll a new key. A kernel rebuilt on an installed system is
unsigned and only boots with Secure Boot off.

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
dosfstools rsync shim-signed sbsigntool`, plus `ovmf netcat-openbsd
qemu-system-x86` for testing):

```bash
cd "/mnt/c/Linsoft os  pro/linsoft-desktop/linsoft/iso"
LINSOFT_LFS_BUILD=1 bash build-iso-kernel     # once, or after kernel changes
bash build-live-iso                           # [IMAGE.qcow2] [OUTPUT.iso]
bash test-live-iso "/mnt/c/Linsoft os  pro/Linsoft-OS-Live.iso"
```

The source image must not be running in QEMU. A `.sha256` file is written next
to the ISO. Write it to a USB stick with Rufus (DD mode recommended; ISO mode
also works because `EFI/BOOT` is in the ISO tree), balenaEtcher or `dd`; the
stick is erased.

`test-live-iso` boots the ISO in QEMU on BIOS and UEFI (DVD and USB stick),
with Secure Boot and Microsoft keys (MokManager key enrollment, then a
Rufus-style FAT copy), drives the installer with key presses onto an empty
20 GB virtual disk and boots the installed disk on BIOS, UEFI and UEFI Secure
Boot. Screenshots and serial logs stay in
`/var/tmp/linsoft-iso-test`. It is a virtual-machine test only; real hardware
(graphics, Wi-Fi, firmware quirks) has not been tested.
