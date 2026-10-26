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

`linsoft-install` offers two modes (the Live medium itself is never offered):

* **Whole disk** (minimum 12 GiB) – after the user types `ANO` the disk is
  erased and gets the layout below.
* **Next to Windows** – only when started through UEFI on a GPT disk that
  already has an EFI System Partition and at least 12 GiB of unpartitioned
  space (create it in Windows: Disk Management → Shrink Volume). Existing
  partitions are never resized or formatted: a new ext4 partition is appended
  into the largest free area, the loaders go to `\EFI\linsoft\` (shim as
  `shimx64.efi`), the signed GRUB's `\EFI\BOOT\grub.cfg` stub is written (an
  existing one is kept as `grub.cfg.pred-linsoft`), `efibootmgr` adds a
  "Linsoft OS" NVRAM entry, GRUB gets a `Windows` entry chainloading
  `bootmgfw.efi`, BIOS GRUB is skipped and the hardware clock is set to local
  time (`UTC=0`) like Windows.

Whole-disk layout:

1. 1 MiB BIOS boot partition (GRUB for BIOS),
2. 512 MiB EFI System Partition (FAT32, `EFI/BOOT/` with shim, signed GRUB,
   MokManager and `linsoft-secureboot.cer`),
3. ext4 root partition labelled `Linsoft` using the rest of the disk.

It copies the pristine system from the squashfs (changes made in the Live
session are not copied), writes `fstab` by UUID, points GRUB at the new
`PARTUUID`, generates new machine IDs and installs GRUB for BIOS and UEFI. The
EFI loader uses the removable fallback path, so no NVRAM entry is needed.
Secure Boot may stay enabled (see below). Custom partitioning is not supported.

The installer also asks for a password for the user `linsoft`. When one is
set, it is stored with `chpasswd`, the `autologin`/`passwordless` markers are
removed and `/etc/linsoft-apps/sudoers-password` becomes the sudoers policy:
the Native desktop starts behind the XScreenSaver lock and administrative
actions ask for the password (Wi-Fi, diagnostics and device actions do not).
An empty password keeps the automatic login of the Live system.

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

Without an installer password the installed system keeps the autologin and
passwordless fixed-action opt-ins of the Live system. Anyone with physical
access can then use the desktop and the fixed system helpers, so set a
password on a shared computer and review
[Passwords and recovery](../../README.md#passwords-and-recovery).

## How it works

* `build-iso-kernel` downloads the long-term Linux release (default 6.18.58,
  `LINSOFT_KERNEL_VERSION`), verifies its SHA-256, reuses the LFS kernel
  configuration and applies `kernel-hardware.list` (`y|m|n OPTION`; `!` lines
  must hold afterwards): squashfs, overlayfs, NVMe/VMD, USB UAS, SD/MMC,
  simpledrm, Wi-Fi (Intel, Realtek, MediaTek, Atheros, Broadcom), amdgpu,
  nouveau, i915/xe, I2C HID touchpads, SOF/ACP audio and Bluetooth. The result
  goes to `/mnt/lfs/sources/linsoft-iso-kernel-out`; `boot/kernel-release`
  tells `build-live-iso` the module directory.
* `install-hardware-support ROOTFS KVER` runs inside `build-live-iso`: it adds
  Debian `non-free-firmware`, installs firmware packages, `wpasupplicant`,
  `iw`, `pciutils`, `usbutils`, `alsa-ucm-conf` and `efibootmgr` into the
  Debian layer (APT cache `/var/tmp/linsoft-apt-cache`), turns host
  `/lib/firmware` into a link to the Debian firmware, installs the host tools
  `linsoft-wifi` (wpa_supplicant service, init script `S24linsoft-wifi`) and
  `linsoft-hwreport`, the desktop tools `linsoft-wifi-ui`, `linsoft-diag` and
  `linsoft-update-check`, and writes the version from `VERSION` into
  `os-release` (`LINSOFT_VERSION`). The ISO must stay below 2 GiB (GitHub
  Releases limit); `build-live-iso` checks it.
* `initramfs-init` (static BusyBox) finds the medium by the build-specific
  `/linsoft/live/<id>.id` marker, mounts `rootfs.squashfs` under a RAM
  overlayfs and adds the Live-only files from `live-overlay/` to the RAM layer.
* `live-overlay/etc/rc.d/init.d/linsoft-live` is a Live-only root service that
  opens the installer on VT9 when the desktop launcher requests it.
* The ISO copy is cleaned of shell histories, caches, the Firefox profile,
  SSH directories, logs and downloaded APT packages; the Debian machine IDs
  are regenerated.

## Desktop

`install-hardware-support` installs XFCE 4.20 in the Debian chroot and makes it
the default (`/etc/linsoft-apps/default-desktop`). The Linsoft panel
(`debian/xfce-panel.xml`), desktop settings (`debian/xfce-desktop.xml`) and
the Linsoft wallpaper replace the XFCE defaults through `dpkg-divert`.
`linsoft-app-session` reads the per-user choice from
`~/.config/linsoft/desktop` (`xfce` or `native`), set with
`linsoft-power desktop` (*Zmenit pracovne prostredie*). Power actions go through
`linsoft-power` / `linsoft-session-action` because XFCE's own shutdown
buttons need logind.

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

## Kernel updates

Installed systems (1.3 and later) update the kernel without a new ISO:

```bash
LINSOFT_LFS_BUILD=1 LINSOFT_KERNEL_VERSION=6.18.59 bash build-iso-kernel
bash build-kernel-package /var/tmp/linsoft-kpkg
gh release upload vX.Y /var/tmp/linsoft-kpkg/linsoft-kernel-*   # .tar.zst, .sig, .sha256
```

`build-kernel-package` signs the kernel for Secure Boot (`sign-kernel`, the
same step the ISO uses), packs it with its modules into
`linsoft-kernel-KVER.tar.zst` and signs the package with the Linsoft key
(`linsoft/hardware/linsoft-update-key.pem` is the public half). In the
system, **Aktualizacie** (`linsoft-update-check`) offers the newest package
of the latest release whose version is higher than the installed kernel,
downloads it and asks the host service (`kernel-update`) to install it.
`/usr/sbin/linsoft-kernel-update` then, as root on the LFS host:

* refuses Live mode, versions that are not newer, and packages without a
  valid Linsoft signature, unexpected paths or a kernel not prepared for
  Secure Boot - before anything is changed;
* installs the modules, keeps the current kernel as
  `/boot/linsoft-kernel.old` (with its modules) and replaces
  `/boot/linsoft-kernel` atomically;
* adds the GRUB entry "Linsoft OS - predchadzajuce jadro (KVER)" and
  updates `LINSOFT_KERNEL` in `/etc/os-release`.

To test an update, build a second kernel with another suffix and run step 7:

```bash
LINSOFT_LFS_BUILD=1 LINSOFT_KERNEL_LOCALVERSION=-linsoft-kernel-test \
    LINSOFT_KERNEL_OUT=linsoft-kernel-test-out bash build-iso-kernel
bash build-kernel-package /var/tmp/linsoft-kpkg /mnt/lfs/sources/linsoft-kernel-test-out
LINSOFT_TEST_KERNEL_PACKAGE=/var/tmp/linsoft-kpkg/linsoft-kernel-6.18.58-linsoft-kernel-test.tar.zst \
    bash test-live-iso ISO      # LINSOFT_TEST_KERNEL_ONLY=1 reuses the installed disk
```

Step 7 checks that a tampered package is rejected, installs the update, boots
the new kernel and the fallback kernel with Secure Boot and unlocks the
desktop on the new kernel.
