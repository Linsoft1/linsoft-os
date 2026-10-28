# Linsoft OS

**Váš priestor. Váš systém.**

**Current release: Linsoft OS 1.5 (28 Oct 2026)** — Debian 13 (trixie),
custom Linux `6.12.113-linsoft-amd64`, Openbox, Tint2 and Linsoft Hub.
Author: Martin Pastorek.

* Website: https://linsoft1.github.io/linsoft-os/
* [Download Linsoft OS 1.5](https://github.com/Linsoft1/linsoft-os/releases/download/v1.5/Linsoft-OS-Live.iso)
* [SHA-256 checksum](https://github.com/Linsoft1/linsoft-os/releases/download/v1.5/Linsoft-OS-Live.iso.sha256)
* [Release notes](https://github.com/Linsoft1/linsoft-os/releases/tag/v1.5)
* [Changelog](CHANGELOG.md)
* [Version 1.5 build configuration](linsoft/debian-live/)

## What changed in 1.5

The Store application and privileged helper now have Linux LF shebangs and
can launch. Plymouth retains the Linsoft logo and shows the appropriate
startup, shutdown or restart text. The rounded taskbar has an SK/US keyboard
indicator (Alt+Shift), hidden workspace labels and an aligned Start menu.
The wallpaper and login screen use Linsoft branding.

This image is the **Debian/Openbox edition**, not the LFS/XFCE/Native image
published as 1.4. It uses the Debian-signed kernel and a Microsoft-signed
shim, so it boots with Secure Boot enabled; the 1.4 in-system kernel updater is
not included. It is not an in-place upgrade of 1.4.
Older releases and their source remain available.

## Status and warnings

Experimental. Verify the checksum and try live mode in a virtual machine
before installation. The image contains BIOS and UEFI boot catalog entries,
but full boot paths, a clean installation and physical hardware are not
certified for this release.

**Secure Boot**: the image boots through Microsoft-signed shim, Debian GRUB and
the Debian kernel (tested in QEMU/OVMF with Secure Boot on). Recommended
memory for initial testing: 4 GB. Back up your files before installation;
automatic partitioning can erase the selected disk. Changes made in a live
session are lost at reboot.

## Repository layout

| Path | Contents |
| --- | --- |
| [linsoft/debian-live/](linsoft/debian-live/) | Version 1.5 live-build scripts, kernel configuration and system customizations |
| [linsoft/](linsoft/README.md) | Earlier LFS/Native/XFCE integration and tools |
| [linsoft/native/](linsoft/native/README.md) | Earlier C/Xlib desktop source |
| [linsoft/iso/](linsoft/iso/README.md) | Earlier image lineage and its installer/tests |
| [README-desktop.md](README-desktop.md) | Earlier edition history and build notes |
| `libfm`, `lxpanel`, `menu-cache`, `openbox`, `pcmanfm` | Upstream source submodules |
| [docs/](docs/index.html) | GitHub Pages website |

ISO images are published as GitHub Release assets, not committed to Git.

## Licenses and source

Linux, GNU, Debian, Openbox and other included software retain their original
licenses (GPL, LGPL, MIT and others). Upstream submodules retain their licenses.
The 1.5 build downloads the upstream Linux 6.12.113 source from kernel.org and
builds it using the included configuration and script. Debian package source
is available from the Debian 13 archives using the matching package versions
in the release's `filesystem.packages` manifest. The earlier LFS edition's
source and build information remain in their original directories.
