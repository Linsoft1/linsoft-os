# Linsoft OS

**Váš priestor. Váš systém.** Linsoft OS is an experimental operating system built
from scratch on Linux From Scratch 12.4, with an original C/Xlib desktop
(window manager, panel, Start menu and file manager).

**Version:** Linsoft OS 1.2 (26 Oct 2026) · Linsoft Kernel 6.18 LTS · LFS 12.4 · Debian 13 apps · x86-64 BIOS/UEFI/Secure Boot

**Author:** Martin Pastorek – Native desktop programmed in C (Xlib/Xft) and the
customized **Linsoft Kernel** (Linux 6.18 LTS: Wi-Fi, AMD/NVIDIA/Intel graphics,
laptop touchpads and audio, Live USB/DVD boot, NVMe and SD card support).

What is new: [CHANGELOG.md](CHANGELOG.md).

* Website: https://linsoft1.github.io/linsoft-os/
* Download: [Linsoft-OS-Live.iso](https://github.com/Linsoft1/linsoft-os/releases/latest/download/Linsoft-OS-Live.iso)
  (BIOS + UEFI, Live system and disk installer)

![Linsoft OS desktop](docs/img/desktop.png)

## Repository layout

| Path | Contents |
| --- | --- |
| [linsoft/](linsoft/README.md) | Linsoft desktop integration, Native desktop source, installers and tests |
| [linsoft/native/](linsoft/native/README.md) | Native C/Xlib/Xft desktop shell |
| [linsoft/iso/](linsoft/iso/README.md) | Live/installer ISO build, installer and QEMU test |
| [README-desktop.md](README-desktop.md) | Detailed edition history and build notes |
| `libfm`, `lxpanel`, `menu-cache`, `openbox`, `pcmanfm` | Unmodified upstream LXDE/Openbox sources (git submodules) |
| [docs/](docs/index.html) | GitHub Pages website |

Prebuilt QCOW2 disk images are not stored in this repository; the ISO is
published as a GitHub Release asset.

## Status and warnings

Experimental. Automated tests run in QEMU (BIOS, UEFI, Secure Boot); use the
built-in **Diagnostika hardveru** tool and report problems on real computers in
[Issues](https://github.com/Linsoft1/linsoft-os/issues). The installer either
erases the whole selected disk or (UEFI + GPT) installs into free space created
in Windows, keeping Windows bootable. The installer can set a user password;
without it the system logs in automatically. Secure Boot can stay enabled: the
first start asks once to enroll the Linsoft key in MokManager (see
[linsoft/iso](linsoft/iso/README.md#secure-boot)).

Known limits: Bluetooth has kernel drivers but no pairing UI yet, enterprise
(802.1X) Wi-Fi is not supported, NVIDIA uses the open nouveau driver, and new
kernels arrive as a new release ISO.

### Supported computers

| Computer / firmware setting | Status |
| --- | --- |
| x86-64, legacy BIOS (CSM) | Works |
| x86-64, UEFI, Secure Boot off | Works |
| x86-64, UEFI, Secure Boot on (most Windows 10/11 PCs) | Works after enrolling the Linsoft key once |
| Secured-core PCs / firmware with "Microsoft third-party UEFI CA" disabled | Needs that option enabled (or Secure Boot off) – same as Ubuntu, Fedora or Ventoy |
| 32-bit CPUs, 32-bit UEFI, ARM, Apple Mac | Not supported |

The shim is signed with Microsoft UEFI CA 2011. Firmware that trusts only
Windows-signed loaders cannot start any third-party OS until the third-party CA
is enabled; Linsoft OS cannot work around this itself.

## Licenses

Linsoft OS bundles Linux, GNU, Debian and other open-source software under
their own licenses (GPL, LGPL, MIT, ...). The upstream submodules keep their
original licenses. Source for bundled third-party packages is available from
their upstream projects (Linux From Scratch 12.4 and Debian 13 archives).
