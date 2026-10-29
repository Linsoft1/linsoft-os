# Changelog – Linsoft OS

All notable changes to Linsoft OS. Versions follow the GitHub releases
(https://github.com/Linsoft1/linsoft-os/releases).

## 1.5 – Debian live / Openbox edition (28 Oct 2026)

* Website: fixes mobile image proportions and narrow-screen gallery overflow;
  adds a collapsible accessible navigation on all three pages, with all links
  still available when JavaScript is disabled.
* Publishes the updated Debian 13 live image with the Debian-signed
  `6.12.113+deb13-amd64` kernel, Openbox, Tint2 and Linsoft Hub.
* Boots with Secure Boot enabled (signed shim, GRUB and kernel); fixes the
  "Secure Boot Violation" seen with the earlier unsigned custom kernel.
* Fixes CRLF shebangs preventing Linsoft Store and its privileged helper from
  launching.
* Plymouth keeps the Linsoft logo and selects startup, shutdown and restart
  messages according to the actual mode.
* Graphical installer uses the Linsoft banner and the dark Linsoft Blue colours.
* XFCE screensaver reads its own desktop background settings, independently of
  Nitrogen. Sets its monitor-independent fallback to the Linsoft wallpaper so
  locking the Openbox session no longer shows the Debian background.
* Compact rounded taskbar, aligned Start menu, hidden workspace labels and
  SK/US keyboard indicator (Alt+Shift).
* Branded wallpaper, login screen and user image.
* This is a different image lineage from the 1.4 LFS/XFCE edition, not an
  in-place upgrade. Clean installation, full
  BIOS/UEFI boot paths and physical hardware are not certified.

## 1.4 – XFCE desktop by default

* **XFCE 4.20 is the default desktop**: bottom taskbar with the Linsoft menu,
  browser, files, terminal, open windows, volume, clock and a power button;
  Linsoft wallpaper and dark theme, no drive icons on the desktop.
* **Linsoft Native stays available**: *Zmenit pracovne prostredie* (in both
  desktops and in the power menu) chooses XFCE or Linsoft Native for the next
  login, per user.
* New power menu (lock, log out, restart, shut down) using the authorized
  Linsoft session requests.

## 1.3 – Kernel updates in the system

* **In-system kernel updates**: Aktualizacie downloads a newer Linsoft Kernel
  from the latest GitHub release and installs it without a new ISO.
  * Packages are signed with the Linsoft key; a tampered or foreign package is
    rejected before anything changes, and kernels not prepared for Secure Boot
    are refused.
  * The previous kernel and its modules are kept, with a GRUB entry
    "Linsoft OS - predchadzajuce jadro" to start it.
  * Daily notification when a new kernel is available; Live mode is never
    changed.
* Tests: tampered package rejection, update, new kernel and fallback kernel
  boot with Secure Boot, desktop unlock with the new kernel.

## 1.2 – Real hardware

* **Linsoft Kernel 6.18 LTS** (long-term support kernel, replaces 6.16.1) with
  drivers for current laptops and desktops:
  * Wi-Fi: Intel (iwlwifi), Realtek (rtw88/rtw89/rtl8xxxu), MediaTek
    (mt7921/mt7925), Qualcomm Atheros (ath9k–ath12k), Broadcom (brcmfmac).
  * Graphics: AMD (amdgpu/radeon), NVIDIA (nouveau), Intel (i915/xe) with
    hardware 3D acceleration when a GPU driver is active.
  * Laptops: I2C touchpads and touchscreens, Intel/AMD audio DSP (SOF, ACP),
    NVMe behind Intel VMD/RST, USB Ethernet adapters, laptop hotkeys.
* **Firmware** for Wi-Fi, graphics, audio and Bluetooth chips (Debian
  non-free-firmware), updated together with the system.
* **Wi-Fi with a password**: network list, WPA2/WPA3 password, hidden
  networks, automatic reconnect after restart. WIFI indicator in the panel.
* **Hardware diagnostics**: a report of devices, drivers, missing firmware,
  graphics, network and sound. In Live mode it is also saved to the USB stick.
* **User password**: the installer can set a password; the desktop then starts
  locked and administration asks for it. An empty password keeps automatic
  login.
* **Dual boot with Windows**: the installer can use free space created in
  Windows (UEFI, GPT). Windows stays untouched and GRUB offers both systems.
* **Update check**: notification when a new Linsoft OS release is published.
* GitHub Actions check of all scripts, the C desktop and the web page.

## 1.1 – Secure Boot

* Microsoft-signed shim, Linsoft-signed GRUB and kernel; enroll the Linsoft
  key once in MokManager.
* More reliable USB start; Rufus DD and ISO modes both work.

## 1.0 – First release

* Live system from USB/DVD and a disk installer for BIOS and UEFI.
* Linsoft Native desktop written in C and Linsoft Kernel 6.16.1.