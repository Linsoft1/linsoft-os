# Changelog – Linsoft OS

All notable changes to Linsoft OS. Versions follow the GitHub releases
(https://github.com/Linsoft1/linsoft-os/releases).

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