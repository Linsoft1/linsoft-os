# Linsoft Desktop

## Bootable Live/installer ISO

`../Linsoft-OS-Live.iso` boots the Native edition from DVD/USB on BIOS and UEFI
PCs, either as a Live system (changes kept in RAM) or straight into the disk
installer; the installer is also available from Start in the Live desktop.
Build, test and installer details are in [linsoft/iso/README.md](linsoft/iso/README.md).

## Default release: Linsoft Desktop Professional (Native)

`Linsoft-Desktop-Native.qcow2` is the independent experimental version of an original
Linsoft desktop. One new C/Xlib/Xft program implements its own window manager,
bottom panel, Start menu and file manager over the existing Xorg server.
It is not an Openbox theme and does not start Openbox, tint2 or the PCManFM
desktop in its primary session. Existing GTK/system/application tools are
retained. The normal `../Start-Linsoft-OS.cmd` launcher now starts this desktop
as Linsoft Desktop Professional. `../Start-Linsoft-Native.cmd` remains an alias.
The disk keeps its Native filename so existing backups and edition receipts
remain valid. The older Openbox-based Professional disk and all backups are preserved separately,
but its Windows launcher has been removed. The active Native disk no longer
contains the Openbox, tint2 or PCManFM packages. Xorg, GTK/XFCE settings and
notification services stay installed because Native and applications need them.
The expanded Native shell includes original icons, edge resizing/snapping,
multi-selection copy/move, location/history/hidden-file controls, trash
restore, UTF-8 input, saved wallpaper/colors/icon size/pinned-app settings,
and live audio/network/battery/USB status with existing system controls.
It also supports searchable installed applications in Start, file tabs and
image previews, current-directory name filtering, internal drag-and-drop with
copy/move confirmation, four workspaces and a calendar/notification center.
Use `../Update-Linsoft-Native.cmd` with Native shut down to create a verified
pre-update backup. `../Manage-Linsoft-Native.cmd` provides manually confirmed
full-disk recovery, including documents, while retaining the replaced disk.

See [Native controls, build, tests and limitations](linsoft/native/README.md).
This deliberately limited first version is not a feature-equivalent
replacement for KDE/GNOME.

## Preserved release: Professional (legacy Openbox desktop)

`Linsoft-Desktop-Professional.qcow2` is an independent derivative of Complete.
This standalone 12 GiB image is no longer the default and has no separate Windows
launcher. The earlier Complete and Unified images remain
preserved. Professional adds:

- First-boot password creation using native Debian `passwd`, followed by a
  password-authenticated graphical login. No shared/default password is shipped.
  The personal password is entered locally in the VM, not in chat.
- Screen locking via XScreenSaver/PAM, Super+L, and a ten-minute idle lock.
  Login and system authentication use Debian sudo/PAM rather than custom
  password hashing. Failed or cancelled login never opens a root shell.
- Root-owned host/session request directories. The application user must
  authenticate each privileged request through a validated fixed-action sudo
  helper; no unrestricted passwordless root shell is granted.
- An application/update manager (Super+A) for repository search, package
  information, install/remove, installed inventory, index refresh and upgrades.
  Actual APT progress, confirmations and failures are visible in a terminal.
  Removing protected desktop/authentication dependencies is refused.
- A quick-controls panel for sound, network, calendar, appearance, wallpaper,
  panel launchers, locking, changing the personal password and logging out.
  Panel preferences live in the user's configuration and survive reboot.
- Full offline disk backup/restore through `../Manage-Linsoft-OS.cmd`.

At first startup, set your own password in the native terminal, enter it again
for confirmation, then use it to log in as `linsoft`. Password typing in that
terminal is intentionally invisible. No personal password is stored in these
instructions or requested in chat.

Actual QEMU checks verified wrong-password refusal, password creation/login,
manual screen lock with rejected and accepted unlock attempts, a UID 1000
desktop and terminal, blocked passwordless sudo, protected package removal,
repository installation/removal through the graphical application manager,
logout/cancelled login/relogin, authenticated DHCP, reboot persistence of a
document and appearance, and orderly password-authorized shutdown.
The final disk passed warning-free offline ext4 and QCOW2 checks, an empty
dpkg audit, and an upgrade simulation with no pending application upgrades
after refreshing the signed indexes. Test helpers were removed; a separate
fixture-free boot verified first setup, graphical login, terminal and shutdown.
The released base still has a locked account and first-setup marker, not a
test password. Real release screenshots are `../Linsoft-Professional-preview.png`,
`../Linsoft-Professional-first-start.png` and `../Linsoft-Professional-controls.png`.

### Passwords and recovery

On this local installation, the owner's opt-in `/etc/linsoft-apps/autologin`
marker disables graphical login/first-password setup and the XScreenSaver
daemon. The desktop still runs as UID 1000 and sudo still requires the existing
password for system changes. Anyone with access to the VM can open the desktop;
logout returns directly to a new desktop. Removing this root-owned marker
restores normal password login and locking.

The owner's additional `/etc/linsoft-apps/passwordless` opt-in removes password
prompts for the existing fixed application-manager and system-action helpers.
It requires autologin and selects `professional-sudoers-passwordless` during
reinstallation. The local `linsoft` account has no password; the host root
account remains locked and arbitrary root commands are not added to sudoers.
Anyone with access to the desktop can install trusted packages or request
system changes without a password. Removing these markers alone does not
restore a password: set a new personal password offline and restore the normal
sudo policy before re-enabling login.

The normal boot is managed by a root startup supervisor rather than a root
autologin shell; desktop applications run as UID 1000 only after login.
The underlying LFS root account is locked. Diagnostic/original-DRM GRUB
entries open the Debian login console, not an unauthenticated host root shell.
The privileged original DRM tools are available from the authenticated
desktop System Tools action, with a fresh password confirmation.

The old single-user root-password entry is not a password-recovery mechanism
for this edition because the host root account is locked. Preserve a full
offline backup; forgotten-password recovery requires authorized offline
maintenance. There is no secret/default administrator password.

This is a single-user X11 integration, not encrypted storage, a hardened
multiuser display manager, or a chroot security sandbox. Someone who can
modify the disk/GRUB offline can bypass local authentication. A normal
logout closes the graphical session and returns to login; save work first.

### Application updates and backup

APT uses the existing signed Debian 13 repositories. It updates the separate
application environment, **not the LFS kernel or LFS system libraries**.
At login a nonprivileged check may notify about upgrades in the saved indexes;
refresh indexes in the application manager to obtain current results.
Repository/package scripts must be trusted because they run as root.

Before a package change the helper saves an installed-package inventory,
APT/desktop/sudo configuration and its operation log under the root-only
`/var/lib/linsoft-package-history`. These are diagnostics/configuration
archives, **not an automatic full-system rollback**.

For a complete backup, shut down Linsoft and run `Manage-Linsoft-OS.cmd` in
Windows. Choose Yes to back up or No to restore. Restore requires an explicit
confirmation because it replaces the entire disk, including documents, with
the backed-up state. The previous disk is retained as a before-restore safety
copy. Backups are standalone QCOW2 images checked before promotion; open VM
images and backing-file chains are refused. Backups are not encrypted.
The manager was exercised with a real full-disk backup and restore; a
`qemu-img compare` check confirmed identical restored guest contents.

CLI equivalents, from the Windows workspace:

```powershell
.\Manage-Linsoft-OS.cmd -Action Backup
.\Manage-Linsoft-OS.cmd -Action List
.\Manage-Linsoft-OS.cmd -Action Restore -BackupPath 'C:\path\backup.qcow2' -ConfirmRestore
```

The manager now defaults to the Native disk used by Linsoft Desktop Professional.
Use `-ImageName Linsoft-Desktop-Professional.qcow2` for the legacy Openbox edition
or `-ImageName Linsoft-Desktop-Complete.qcow2` for the preserved Complete edition.
Do not start a VM while an offline backup/restore operation is running.

### Reproducing Professional

Build optional dependencies in the separate build root with
`LINSOFT_LFS_BUILD=1 LINSOFT_DESKTOP_PROFESSIONAL=1 bash
linsoft/build-debian-apps`, or install the declared professional packages into
the existing separate application tree of a closed derivative image.
Mount that image at `/mnt/linsoft-debian-image`, then run
`LINSOFT_LFS_BUILD=1 bash linsoft/install-professional-desktop`.
The installer extends Complete and its boot splash, validates sudoers and
preserves existing documents and configured passwords on subsequent runs.
Use a private mount namespace for offline maintenance and fully unmount the
image before filesystem checks or starting QEMU.
Use this installer, not a base installer, when updating Professional.
Related tests are `linsoft/test-professional-actions`,
`linsoft/test-desktop-extras`, `linsoft/test-desktop-host-actions` and
`linsoft/test-unified-actions`.

QEMU-first scope remains explicit: wired virtual networking/audio/USB can be
tested here. Wi-Fi connection management, Bluetooth pairing, physical GPU
support, laptop battery/brightness, sleep/resume and physical-PC installation
still need appropriate real hardware. No success is claimed for those devices
just because a settings package is installed.

## Preserved release: Complete

`Linsoft-Desktop-Complete.qcow2` remains a standalone
12 GiB virtual disk based on the preserved Unified edition. The Windows launcher
now selects Linsoft Desktop Professional's Native disk instead. Historical Complete release screenshots are
`../Linsoft-Complete-preview.png` and `../Linsoft-Complete-settings.png`.
The earlier Unified, Apps, Interactive and System images remain available.

### Animated Linsoft startup

The default Complete boot uses an original Linsoft ring/L logo, a dark
background and a rotating twelve-dot loading indicator instead of the visible
kernel/service console. This is a native LFS/SysV DRM splash, not Windows
artwork and not a fake timed desktop video. It runs while the real system
services start, then releases the display before Xorg opens VT8.
The actual QEMU startup is shown in `../Linsoft-Startup-preview.png`.

The graphical GRUB menu remains available. Diagnostic, recovery and original
DRM entries retain their console output and bypass the animation. Escape
skips the animation while startup continues; it is not a text-log viewer.
For visible boot logs select the diagnostic GRUB entry. Serial boot
output remains available for troubleshooting. A 120-second watchdog leaves
the splash if startup stalls; a missing display reports an error instead of
preventing the system from booting.

Normal graphical startup writes Xorg/xinit and desktop-session console output
to the root-only application-environment
`/opt/linsoft-debian/var/log/linsoft-desktop-session.log` instead of displaying
the Xorg banner on VT8. The previous session is retained with `.previous`;
the detailed Xorg log remains at
`/opt/linsoft-debian/var/log/Xorg.linsoft.log`. Diagnostic console startup
retains visible output, and graphical-session failures still reach the
existing supervisor error/retry path.

To reproduce on a **closed** Complete image mounted at
`/mnt/linsoft-debian-image`, generate the original asset with the existing
Pillow environment, then run `LINSOFT_LFS_BUILD=1 bash
linsoft/install-boot-splash`. The generator accepts `--splash-only`.
The installer compiles against the LFS runtime, includes libdrm with its
upstream source notices, validates GRUB syntax and preserves the previous
configuration. Use it after the desktop installer when rebuilding the image.
This startup path is tested with the QEMU virtio display, not all physical GPUs.
Real QEMU screenshots verified that the loading dots change between frames.
Both normal animation handoff and skipping the animation reached the clean
desktop; sound/settings/About, DRM return and orderly shutdown still passed.
The final image also passed offline ext4 and QCOW2 integrity checks.

## Unified desktop foundation

`Linsoft-Desktop-Unified.qcow2` is a standalone derivative of the preserved
Apps image. The main graphical login is now Xorg/Openbox rather than the
custom DRM renderer. Firefox, LibreOffice, PCManFM and the real xterm/Bash
terminal run together as UID 1000, with one tint2 task panel, desktop icons,
and the original Linsoft Night wallpaper and window theme. This is a Linsoft
integration of those upstream projects, not KDE Plasma or a new window manager.
The underlying OS is still LFS; application libraries remain in
`/opt/linsoft-debian`.

Double-click `../Start-Linsoft-OS.cmd` to start Complete with 2 GiB RAM.
The older Apps, Interactive and System images remain unchanged. Actual
screenshots are `../Linsoft-Unified-preview.png` and
`../Linsoft-Unified-apps-preview.png`.

### Using the shared desktop

- Click the **L** launcher at the left of the panel, right-click the desktop,
  or press Super+Space for the application/system menu.
- Super+B: browser; Super+W: Writer; Super+C: Calc; Super+E: Files;
  Super+T: terminal; Super+D: show/hide the desktop.
- Alt+Tab / Alt+Shift+Tab switches windows. Title-bar buttons minimize,
  maximize/restore and close. Double-click the title bar or press Super+Up
  to maximize/restore; Super+Down minimizes.
- Drag the title bar to move a window, its lower-right border to resize it,
  or use Alt+left-drag / Alt+right-drag anywhere in the window.
- PCManFM provides real folders, copying, moving, renaming, trash and file
  opening. Documents and Downloads are the same persistent user directories
  as the Apps edition, not a second copy.

The main terminal is now **Linsoft Terminal (xterm)**, not the custom DRM
PTY renderer. Both are real Bash terminals, but the main terminal runs as the
unprivileged application user. The DRM terminal remains available in the
original-desktop fallback.

The menu offers **Install trusted .deb package**. Choose a trusted Debian 13
amd64/`all` package, save work and confirm closing the graphical session.
The host then opens the existing root APT installer, with its separate
`INSTALL` confirmation, and starts the unified desktop again when it exits.
This deliberately does not grant an unrestricted passwordless root shell
to graphical applications. Installing packages still requires trusting their
root scripts; the app tree is not a security sandbox.

Restart and shutdown in the menu require confirmation. **Original desktop /
system tools** closes the shared session and opens the preserved DRM desktop,
where the existing LFS settings, diagnostics, mixer and root terminal remain.
Save documents before closing either session. GRUB also has a separate
**povodna DRM plocha** entry; diagnostic and recovery entries remain available.
If the primary X session fails to start, the startup wrapper reports the
failure and falls back to DRM rather than repeatedly restarting X.

The existing root autologin is retained for the LFS startup/control wrapper;
the graphical programs themselves run as UID 1000. This is not a hardened
multiuser login implementation. Application-session sound, Wi-Fi management,
physical GPU acceleration and physical-PC boot remain unverified. The VM uses
software rendering; cold browser/office startup can take time under emulation.
DNS configuration is refreshed while the session is running so a DHCP update
after graphical startup does not leave applications using stale DNS.
The DNS helper is stopped and reaped before releasing the graphical console,
including its pending sleep. The application user has the standard
`/usr/local/bin:/usr/bin:/bin` command path. GTK 4 dialogs use the Cairo
renderer to avoid expensive GPU shader initialization in the software VM.

To reproduce, first build/install the separate Debian root as described
below (including the declared `xdotool` dependency). Mount an independent,
closed copy at `/mnt/linsoft-debian-image`, then run:

```sh
LINSOFT_LFS_BUILD=1 bash linsoft/install-unified-desktop
```

The installer uses the host's existing `rsvg-convert` to render the supplied
original SVG wallpaper, installs primary-session integration, saves the
previous root startup profile, and adds the DRM boot entry. It never installs
Openbox/GTK libraries over LFS libraries. Do not run it on a disk open in QEMU.
The installed/requested package manifests are refreshed during deployment.

### Verified behavior

The final independent image passed real QEMU tests of primary graphical boot,
the panel menu, nonroot DNS/HTTP, file-manager and Bash windows, mouse window
movement and border resizing, Alt+Tab, maximize/restore/minimize, actual
Writer/Calc documents, and an external Firefox page in the same session.
Separate tests selected and installed a real Debian `hello` package through
the graphical picker and root APT confirmation, executed it, returned
automatically to the shared desktop, rebooted through the confirmed action,
and verified package/folder persistence before confirmed shutdown.

Escape cancellation of power actions, confirmed DRM return, and the actual
GRUB DRM boot entry were tested. The cleaned release booted without test
helpers, ran its UID 1000 terminal with the proper command path, released the
app lock/mount namespace on exit, and shut down normally. Offline ext4 and
QCOW2 checks found no errors; dpkg audit was empty and APT reported no pending
upgrades against its refreshed package indexes. The shell action tests are
`linsoft/test-unified-actions`; they also reject CRLF desktop entries, which
tint2 does not parse correctly.

## Complete desktop extensions

`Linsoft-Desktop-Complete.qcow2` is an independent copy of the closed Unified
image, with the opt-in extensions installed into its existing application
tree. User documents are preserved; the Unified image remains unchanged.
Complete passed the release checks below and is selected by the Windows
launcher. The extensions provide:

- A real settings launcher for display, mouse/cursor, keyboard, GTK appearance,
  PCManFM wallpaper preferences, PulseAudio volume control, notifications,
  task manager and searchable categorized applications.
- Slovak locale and Firefox/LibreOffice translations, a Slovak main menu,
  panel settings/audio buttons and a live IPv4 address display.
- Host LFS identification supplied read-only to the "About Linsoft OS" dialog,
  separately from the Debian application version.
- GVfs dependencies for file operations and trash support.
- Initial default document/browser/package associations, preserving an
  existing user's association file. Opening a selected DEB passes that exact
  filename into the existing confirmation flow.
- A confirmed, fixed-action LFS bridge for DHCP renewal, host date/RTC and
  timezone changes, disk information, and removable USB mounting/unmounting.
  The bridge shares the app session's private mount namespace, so mounted
  media is actually visible in the file manager. It does not provide an
  arbitrary root-command interface or run graphical settings as root.
- PulseAudio in the nonroot application session, with membership in the
  groups owning the actual LFS sound-device nodes.

Build the optional dependencies separately, without changing an open VM disk:

```sh
LINSOFT_LFS_BUILD=1 LINSOFT_DESKTOP_EXTRAS=1 bash linsoft/build-debian-apps
bash linsoft/test-desktop-extras
bash linsoft/test-desktop-host-actions
```

`install-desktop-extras` is an **offline deployment** step, not a
live-update command. It requires a closed image mounted at the usual target,
with every package in `debian/packages-desktop-extras` already installed in
its Debian tree and `sk_SK.UTF-8` generated. It does not copy over the build
root or replace an existing user's documents. It installs the regular Unified
integration first and enables the extensions only after dependency checks.
Do not run it against the active VM. Use this extension installer again
instead of the base installer when updating an extension-enabled image.

Extra shortcuts are Super+I (settings), Super+R (app search),
Ctrl+Shift+Escape (task manager), and multimedia volume keys. The initial
Xfce shortcut profile is empty so its daemon does not steal Openbox bindings;
existing user shortcut configuration is preserved.
The first session seeds the GTK appearance only if no user profile exists;
it does not force `GTK_THEME` over subsequent graphical theme selections.
The panel has settings and sound controls, a compact live IPv4 status, and a
readable clock/date; clicking the clock opens confirmed host time settings.

Preparation tests opened and normally closed real X11 windows for settings,
display, mouse, keyboard, appearance, task manager, app finder, notification
preferences, network status and About. Unit tests cover dispatch, cancellation
and explicit failures. These are isolated Xvfb/build-root tests, **not a
certification of the running Linsoft desktop**.
The same isolated tests passed real copy/rename/trash/restore, the volume
control window, a nonroot PulseAudio stream into an explicitly synthetic
test sink, and a session D-Bus notification. A synthetic private-mount target
also passed repeated extension installation and generated XML validation.
No physical sound-device playback or removable-media access was established
by those tests.

Real Complete-image QEMU tests verified:

- Nonroot playback through the guest's ALSA-backed PulseAudio sink and
  nonzero PCM captured from the virtual HDA output, not a synthetic null sink.
- Confirmed DHCP renewal, host date/RTC and timezone changes.
- Removable USB mount, user-file write and safe unmount, followed by an
  exact-content check against the raw USB image after QEMU exited. Only the
  disposable USB fixture retained writes; the tested OS disk used snapshots.
- Slovak locale, host identity, MIME defaults, real file copy/rename/trash/
  restore, a D-Bus notification, and ten normally opened/closed settings
  windows in the actual VM.
- Confirmed reboot, retained documents, timezone and saved GTK settings, then
  confirmed shutdown.

The fixture-free release separately booted its primary desktop, opened sound
controls by clicking the actual panel button, opened settings with Super+I,
displayed the real About dialog, and ran its UID 1000 terminal. Returning to
DRM released the app lock, host service and private mounts. Test helpers were
absent. The cleaned disk passed offline ext4 and QCOW2 checks, an empty dpkg
audit and zero pending upgrades against refreshed signed package indexes.

### Hardware and privilege limits

QEMU supplies a virtual wired adapter, not Wi-Fi. The network page shows
addresses, routes and DNS; DHCP renewal uses the existing LFS dhcpcd daemon.
No competing NetworkManager is started. A Wi-Fi connection picker, real
wireless firmware/driver support, physical GPU acceleration and general
physical-PC compatibility have not been certified.

USB controls are manual: use the disk overview to find a removable device or
partition (for example `/dev/sda1`), then choose mount or safe unmount. FAT,
exFAT and ext2/3/4 are accepted with `nosuid,nodev,noexec`; FAT/exFAT is owned
by the graphical user, while ext filesystems retain their existing ownership.
This is not automatic mounting. Nonremovable system disks are rejected.

Host changes require a confirmation dialog but use the retained root
autologin/control architecture, not a password-authenticated multiuser
authorization system. The separate Debian root is not a security sandbox.
Host-action tests verify that an invalid date does not proceed to clock/RTC
writes and that unsupported actions and invalid paths are rejected.

## Preserved Debian application edition

`Linsoft-Desktop-Apps.qcow2` extends the original LFS/DRM desktop with a
separate Debian 13 (trixie), amd64 application tree at `/opt/linsoft-debian`.
It does not install Debian's libc, dpkg, or APT into the LFS system directories.
The older Interactive/System images are preserved.
The Windows `Start-Linsoft-OS.cmd` launcher now selects the Complete derivative
described above with 2 GiB RAM. This Apps image remains a separate preserved
DRM-first edition.

### Installing a DEB

1. Close the Debian graphical session if it is running.
2. In the Linsoft **Subory** browser, navigate to a trusted `.deb` file
   (Ctrl+L edits the directory), then open it with Enter or a double-click.
3. Choose **Pokracovat**. The real text-console installer displays the package
   metadata and warning. Type `INSTALL` to continue.
4. APT refreshes its signed Debian repositories, resolves dependencies and
   requests any necessary installation confirmation. Read any errors; press
   Enter at the final prompt to return to the desktop.

The first graphical confirmation can be cancelled with Escape or **Zrusit**.
Package metadata is not a signature or proof that a local DEB is trustworthy.
Use Debian 13-compatible amd64 packages (or architecture-independent `all`
packages). Arbitrary Ubuntu packages, other architectures, kernel packages,
drivers and system services are not generally supported by this environment.

**This chroot is not a hostile-code sandbox.** Package scripts run as root
and share the kernel and devices with LFS; a malicious package can affect the
whole VM. Install only trusted software. Signed repositories do not authenticate
an independently downloaded DEB.

### Browser, office and application session

Select **Debian aplikacie** on the desktop, or the Debian button in Linsoft
Store. It opens a separate Xorg/Openbox session on VT8 with a tint2 panel,
PCManFM, xterm, Firefox ESR, and LibreOffice Writer/Calc. These applications
run as the locked `linsoft` user (UID 1000), not as the root DRM desktop.
**Linsoft Browser** is an original start page and launcher for unmodified
Mozilla Firefox ESR, not a newly implemented browser. Mozilla and LibreOffice
attribution and licenses are retained. Firefox hardware acceleration is
disabled for this software-rendered VM configuration; its sandbox is not
disabled.

Right-click the application desktop or press Super+Space for its menu.
**Installed applications** opens the installed desktop launchers in PCManFM;
programs can also be started by their executable name in the Debian terminal.
Shortcuts: Super+B browser, Super+W Writer, Super+C Calc, Super+E files,
Super+T terminal. Use **Return to Linsoft desktop** in the menu to close the
session and restore the DRM desktop. Save documents before leaving.

Files persist under `/home/linsoft/Documents` and `/home/linsoft/Downloads`
inside Debian. In the LFS Files browser these are
`/opt/linsoft-debian/home/linsoft/Documents` and
`/opt/linsoft-debian/home/linsoft/Downloads`. To install a browser-downloaded
DEB, close the app session, then open it from the latter directory in Linsoft
Files.

From an LFS root terminal:

```sh
linsoft-debian status
linsoft-debian install "/path/to/application.deb"
linsoft-debian update
linsoft-debian shell
linsoft-debian desktop
```

Only one integration command/session can use the app tree at a time; close
the session before installing or updating. `shell` is a privileged Debian
administration shell; `exit` returns to LFS. `update` upgrades Debian apps,
not the LFS kernel or LFS libraries. The original signed offline Linsoft
catalog remains independent of APT.

Build with `LINSOFT_LFS_BUILD=1 bash linsoft/build-debian-apps` on the Linux
build host. This uses authenticated debootstrap and Debian repositories and
applies available upgrades. `linsoft/install-debian-apps` accepts only the
separately mounted app image at `/mnt/linsoft-debian-image`, refuses to
replace an existing app tree, and requires the same root/build guard.
Never deploy to an image open in QEMU. Installed/requested package lists are
stored in `/usr/share/doc/linsoft-debian`.

This is not Plasma or a general native LFS package manager. The application
session is separate from the custom DRM terminal/window. Browser/office
startup is slow with QEMU CPU emulation; use at least 2 GiB guest RAM.
Physical-PC boot, Wi-Fi, GPU acceleration, and application-session audio are
not certified by this addition.

Validation passed real Files/keyboard and mouse DEB confirmation followed by
APT installation and execution of Debian `hello`; nonroot DNS and HTTP 200;
Firefox loading its local page and an external Example Domain page; Writer
displaying the generated ODT and Calc displaying an ODS with the expected
cell value 42; return to the root DRM terminal; unchanged LFS libc/desktop
hashes after installation; document/package persistence across reboot and
orderly shutdown. C input/PTY tests and 12 layout checks passed at 800x600,
1280x720 and 1920x1080. Temporary guest test helpers/packages were removed
from the release image. These tests used QEMU snapshots, not physical PCs.

## KDE Plasma direction

The selected target is now KDE Plasma on the existing Linsoft LFS 12.4
System V base, not a replacement distribution. The earlier Openbox sources
and integration below are retained, not installed into the running VM.

The LFS build tree has the input and Wayland/session foundations described
below, but Qt and Plasma are not installed or boot-tested. The existing
QCOW2 image still contains the original DRM/KMS desktop; installing libraries
in the offline tree does not update that image.

`linsoft/build-lfs-graphics-foundation` starts the source build with
libpciaccess 0.18.1, libdrm 2.4.125 (including Intel userspace support),
libevdev 1.13.4, mtdev 1.1.7, and libinput 1.29.0, using the BLFS 12.4
System V versions. Run from a Linux host as root with the offline build tree
at `/mnt/lfs`:

```sh
LINSOFT_LFS_BUILD=1 bash linsoft/build-lfs-graphics-foundation
```

It verifies pinned SHA-256 digests of the downloaded archives. The initial
archives were also checked against the published BLFS MD5 sums; neither
check constitutes an upstream cryptographic signature.
It installs into the LFS build tree only, never an open VM image. Existing
chroot mounts cause it to stop rather than interfere with another build.
Sources and build results remain in `/mnt/lfs/sources/linsoft-plasma-foundation`.

These five libraries have been built and installed in the offline build
tree. Seven hardware-independent libdrm tests and five libinput tests passed.
The libinput Python checks use pytest 8.4.1 in a dedicated test virtualenv
under the source cache; they do not add Python packages to the target system.
Hardware-dependent tests and the full libinput test suite were not run.
Tablet support via libwacom and libinput's graphical debug tool are not
enabled in this foundation build.

`linsoft/build-lfs-session-foundation` has also built and installed
libxml2 2.14.5, D-Bus 1.16.2, Wayland 1.24.0, wayland-protocols 1.45, and
pixman 0.46.4. All 26 Wayland tests and 173 protocol tests passed.
`linsoft/build-lfs-desktop-tools` builds xkeyboard-config 2.45,
libxkbcommon 1.11.0, and CMake 4.1.0. US and Slovak keyboard layouts have
been successfully compiled with the installed libxkbcommon.

Run these scripts with the same `LINSOFT_LFS_BUILD=1` guard, sequentially;
the shared chroot must not be used by concurrent builds.
Session/tools archives are checked against BLFS 12.4 MD5 checksums and their
SHA-256 digests are printed. MD5 is only an archive consistency check, not
cryptographic authentication.

`linsoft/build-lfs-mesa` has built and installed Mesa 25.1.8 for Wayland
EGL/GLES and GBM in the offline tree.
It verifies a pinned SHA-256 digest (the archive was also checked against
the BLFS MD5). This initial configuration selects crocus, virgl, and
softpipe, with LLVM, X11/GLX, Vulkan, and video acceleration disabled.
It does **not** provide the modern Intel iris driver: Mesa 25.1.8 needs
the LLVM/libclc compiler stack to build iris. Crocus is for older Intel
generations and is not a substitute for iris on a modern Intel UHD GPU.
Softpipe is a slow software fallback, not the final Plasma graphics solution.
Virgl also requires compatible virtual GPU configuration and host support.

Mesa's missing Python build dependencies were installed only in
`/sources/linsoft-plasma-foundation/mesa-build-venv`, not in system Python:
Mako 1.3.10, PyYAML 6.0.2, packaging 25.0, and MarkupSafe 3.0.4.
The builder uses that virtualenv when present. To reproduce it inside LFS,
after downloading the corresponding wheels into `mesa-build-wheels`:

```sh
cd /sources/linsoft-plasma-foundation
python3 -m venv mesa-build-venv
mesa-build-venv/bin/python -m pip install --no-index \
    --find-links=mesa-build-wheels \
    Mako==1.3.10 PyYAML==6.0.2 packaging==25.0 MarkupSafe==3.0.4
```

After installation, `linsoft/validate-lfs-session` runs the Wayland,
protocol, and pixman suites, compiles US/Slovak keyboard layouts, checks a
real EGL/GLES red-pixel render/readback with software rendering, and queries
the session D-Bus service. Use `LINSOFT_LFS_BUILD=1` as with the builders.
Passing these checks does not validate physical hardware or a Plasma login.
The full validator has passed: 26 Wayland tests, 173 protocol tests, and
35 pixman tests (234 total, zero failures), both keyboard layout compilations,
and a D-Bus `ListNames` request. The rendering check reported EGL 1.5 with
the softpipe renderer and the exact expected RGBA value `255,0,0,255`.

These libraries are not a complete graphics driver stack. Remaining major
stages include a system D-Bus boot service, PAM/elogind and device permissions,
X11 compatibility, Mesa with the appropriate GPU drivers, Qt 6, KDE Frameworks 6, Plasma, and
desktop applications. Intel hardware may also need device-specific firmware.
KDE/Qt versions must be selected as a compatible set, tested before release,
and checked for current security fixes rather than treating old BLFS pins as
a production update policy.

An original dark cyan/navy KDE color scheme and SVG wallpaper are supplied in
`linsoft/plasma/`. As the desktop user, run
`sh linsoft/plasma/install-appearance` to copy them into the user's XDG data
directory, preserving existing files. Once Plasma is installed, select
**Linsoft Night** in Colors and the wallpaper selector. The installer does
not change the active theme, install Plasma, or replace KDE attribution.

References:
- <https://www.linuxfromscratch.org/blfs/view/12.4/x/libdrm.html>
- <https://www.linuxfromscratch.org/blfs/view/12.4/x/x7lib.html>
- <https://www.linuxfromscratch.org/blfs/view/12.4/x/x7driver.html>
- <https://www.linuxfromscratch.org/blfs/view/12.4/kde/plasma-all.html>

## Preserved Openbox edition

This is the new source tree for Linsoft Desktop. It uses Openbox as its window
manager, LXPanel as its panel, PCManFM as its file manager and desktop, and
libfm/menu-cache as supporting libraries.

The upstream source is kept in separate Git checkouts under this directory so
its original history, copyright notices, and licenses remain available:

| Component | Upstream repository | License |
| --- | --- | --- |
| Openbox | <https://github.com/danakj/openbox> | GPL-2.0 |
| LXPanel | <https://github.com/lxde/lxpanel> | GPL-2.0 |
| PCManFM | <https://github.com/lxde/pcmanfm> | GPL-2.0 |
| libfm | <https://github.com/lxde/libfm> | GPL-2.0 |
| menu-cache | <https://github.com/lxde/menu-cache> | LGPL-2.1-or-later |

The `linsoft/` directory contains Linsoft session integration, a bottom LXPanel
profile, PCManFM desktop defaults, an Openbox menu, and an X session entry.
The upstream projects are not renamed internally and their license or
attribution notices must be retained. Linsoft Desktop is the name of this
downstream desktop distribution and integration.

## Linsoft OS graphics

The prebuilt Linsoft QEMU image uses the Linux kernel's built-in `virtio_gpu`
DRM/KMS driver with QEMU's `virtio-vga` adapter. This provides the graphics
driver needed by that virtual machine; it is not a claim of support for every
physical Intel, AMD, or NVIDIA graphics card. The current VM display is
configured for 1920x1080 and zoom-to-fit fullscreen so the desktop is not
rendered as a small, letterboxed image in QEMU.

From the workspace root, double-click `Start-Linsoft-OS.cmd` (or run
`.\Start-Linsoft-OS.ps1` in PowerShell) to start the prebuilt image. The
launcher refuses to open a second copy of the same VM. Close the running VM
normally before starting a different image or making changes to its disk. The
launcher now selects `Linsoft-Desktop-Complete.qcow2`, using packaged Debian
Xorg/Openbox/PCManFM/tint2 and the extensions above. The Unified, Apps, Interactive,
System, workstation and pointer-repair images are preserved. The original
DRM implementation and its input/visual tests remain in
`../linsoft-desktop-drm-backup/` and are still available as a runtime fallback.
This does not install KDE or build the upstream LXDE source tree into LFS.
`../Linsoft-Unified-preview.png` shows the actual cleaned primary desktop.
Physical-GPU driver support still requires the corresponding hardware and
validation.

## PC network, sound, and console runtime

The PC-runtime image derives from the Night image, preserving all earlier
images and leaving Windows drivers and physical disks untouched.
Read-only Windows hardware enumeration found:

| Device | Hardware identifier | Linsoft support prepared |
| --- | --- | --- |
| Realtek PCIe GbE | PCI 10ec:8168, revision 15 | Built-in r8169; RTL8168H firmware |
| USB Audio and HID | USB 0573:1573 | Built-in snd-usb-audio |
| Intel display audio | HDA 8086:281c | HDA Intel and HDMI codec |
| Intel UHD Graphics | PCI 8086:46b3 | Existing i915 kernel driver; full accelerated userspace remains unfinished |

No physical Wi-Fi adapter was reported. This is not proof of Wi-Fi support:
an adapter's exact PCI/USB ID, compatible driver, firmware, regulatory database
and WPA supplicant/configuration remain necessary. No Wi-Fi driver was guessed,
and no wireless credentials are stored. This image is still a QCOW2 VM image,
not a tested USB installer for the physical PC.

New reproducible offline-LFS builders:

```sh
LINSOFT_LFS_BUILD=1 bash linsoft/build-lfs-pc-runtime
LINSOFT_LFS_BUILD=1 bash linsoft/build-lfs-pc-kernel
```

Run sequentially. The runtime builder installs dhcpcd 10.2.4 (with a dedicated
privilege-separation user), alsa-lib 1.2.14, and alsa-utils 1.2.14.
The downloads were initially checked against BLFS 12.4 MD5 sums; the script
now pins their observed SHA-256 digests. The kernel builder preserves its
original configuration and adds USB/HDA audio drivers, DRM fbdev emulation,
and framebuffer console support. Resulting kernel files use `-pc` names in the
offline tree, leaving its previous boot kernel untouched.

The `linsoft/install-pc-runtime` script installs pre-staged packages, the
rebuilt desktop, and the new kernel into a separately mounted Linsoft image
at `/mnt/linsoft-pc-image`; it requires `LINSOFT_LFS_BUILD=1` and root.
Stage packages from each runtime source directory with
`make DESTDIR=/sources/linsoft-pc-runtime/stage install` inside LFS before
using it. Never attach or change an image that QEMU is using.
In the new image only, the old static QEMU eth0 configuration is backed up
under `/etc/linsoft/pre-dhcp` and disabled. A SysV service starts dhcpcd to
configure detected interfaces automatically, including address, route and DNS.
This needs a working driver/link and a DHCP server; it is not proof of Internet
access on every LAN and does not authenticate Wi-Fi.

Realtek firmware and its redistribution notice are bundled in
`linsoft/pc-runtime/firmware/`, pinned by `SHA256SUMS`, from linux-firmware
revision `9a80a04181713da7f8187ec928c15160137f3262`:
<https://kernel.googlesource.com/pub/scm/linux/kernel/git/firmware/linux-firmware/+/9a80a04181713da7f8187ec928c15160137f3262/>.
The firmware notice is installed under `/usr/share/doc/linsoft-firmware`.
RTL8168 device IDs cover multiple chip variants; physical boot diagnostics
must confirm the revision and actual requested firmware before declaring
hardware support complete.

In the older PC-runtime image, clicking Konzola starts actual Bash on text
VT8 and `exit` returns to the graphical desktop. The current Interactive
edition instead opens a PTY terminal window, described below.
Settings has working DHCP renewal, audio mixer, terminal and hardware
diagnostic buttons. ALSA mixer changes are saved on successful exit.
F6 selects the card, arrows change levels, M toggles mute, and Escape returns.
The desktop's existing root privileges apply to these tools.

Snapshot-mode QEMU verification passed:
- Mouse click launched Bash and a real diagnostic script executed.
- DHCP assigned `10.0.2.15/24`, a default route and DNS automatically.
- DNS resolution and an external HTTP request returned status 200.
- The emulated HDA output was detected; 440 Hz playback produced 1,052,584
  bytes of nonzero captured audio.
- Bash exit and ALSA mixer exit restored the desktop.

These tests do not verify physical speakers, a physical cable connection,
Wi-Fi, or every control-panel function. First audio detection may report no
saved ALSA state until the first successful mixer save.
The Windows launcher adds explicit virtual LAN and HDA playback devices and
uses the WSLg PulseAudio socket for sound. It reports an error if that socket
is absent; it does not silently replace sound with a null backend.
The headless test used a WAV capture backend, not a subjective listening test.
The full KDE login, audio/network GUI, browser and other full-desktop services
remain separate unfinished work.

## Current state

### Branded boot and system-settings image

The preserved standalone System image is the Interactive image's base and adds an
original graphical BIOS GRUB menu, Linsoft logo, eight-second countdown, and
desktop, diagnostic-console and root-password recovery entries. Boot root
selection uses PARTUUID. Kernel and service logs remain textual after the
menu; an animated boot splash is not implemented.

`linsoft/boot/generate-boot-assets.py` generates the original background and
PF2 font from the bundled, licensed DejaVu font using Pillow.
`linsoft/install-boot-theme` installs them and the startup profile into the
fixed, separately mounted `/mnt/linsoft-system-image` target, guarded by root,
`LINSOFT_LFS_BUILD=1`, a mount check and Linsoft OS identity. It preserves old
GRUB/profile files, validates the new config, and never writes a physical
bootloader.

Settings includes real computer-name, time-zone and root-password controls
through `linsoft/pc-runtime/linsoft-system-settings`, alongside DHCP, ALSA,
Bash and diagnostics. These configuration tools use a text console, not
graphical forms. Reboot and shutdown use actual SysV shutdown commands after
confirmation. Password changes do not disable the previously approved root
autologin; this is not a hardened multiuser desktop.

Snapshot QEMU tests passed hostname save/restore and invalid-input rejection,
time-zone change/restore, network/DNS/external HTTP, actual recorded audio,
console/mixer return, reboot through the branded menu, and orderly shutdown.
Separate tests reached the diagnostic shell and recovery password prompt.
Physical boot, Wi-Fi, UEFI, Secure Boot, installation media, full Plasma,
LFS-system online updates remain unfinished; this is not a claim
that Linsoft works on every PC.

### Interactive files and windowed Bash

The standalone `Linsoft-Desktop-Interactive.qcow2` is preserved alongside its
newer Apps, Unified and Complete derivatives. The launcher selects Complete. The original
Interactive edition adds real folder navigation,
directory creation (including names with spaces), editable paths, scrolling,
hidden-file toggle, automatic refresh and a read-only text preview. Errors
are visible; this is not a file-list mock. No delete/move/copy GUI or generic
file associations are implemented in that DRM file browser; the Unified
desktop instead uses PCManFM.

Terminal now opens a movable, resizable, minimizable/maximizable desktop
window containing a real Bash controlling PTY. It supports ANSI colors,
cursor/alternate screens, job-control shortcuts and 1,000 scrollback lines.
Only one session is available, with no tabs or clipboard integration.
The fixed-cell DejaVu Mono renderer currently covers ASCII, not full Unicode.
Terminal launched from Files starts in that folder when creating a session.
Closing a live shell requires confirmation; `exit` stops Bash and preserves
its final output until the window is closed or a new session is opened.
Other system tools still use text VT8. Root privileges/autologin are unchanged.

`linsoft/build-lfs-terminal` downloads SHA-256-pinned libvterm 0.3.3, runs its
upstream tests, and installs its headers/static archive/license only in the
offline LFS tree. The desktop links that archive, so no new shared-library
installation is needed in the image. The SHA-256 pin was recorded from the
HTTPS upstream archive; it is not an independently authenticated signature.
Source and focused C tests are in `../linsoft-desktop-drm-backup/`.
`generate-terminal-font.py` reproducibly generates the bundled Mono atlas.

Desktop evdev keyboard grabs prevent terminal Ctrl+C from also reaching the
underlying Linux VT and killing/restarting the desktop. Console tools release
the grab temporarily and reacquire it on return.
Actual snapshot QEMU testing verified folder creation/navigation, PTY I/O,
file preview, window controls, Ctrl+C, directory/file persistence across
reboot, shell exit/reopen, close cancellation and orderly shutdown.
An additional regression boot verified the real ALSA mixer, same-page return,
keyboard reacquisition, live Bash input and LAN/external HTTP 200 afterward.

This is source code, not a replacement operating-system image. The existing
minimal LFS image does not include Xorg, an X session/login manager, or the
development/runtime libraries needed by these components. The prebuilt
Linsoft VM uses the DRM/KMS desktop with the visual and interactive updates above.

### Build and install

First build/install the component prerequisites in the target LFS system. They
include an X11 server and headers, GTK+ 2, GLib/GIO, Pango, libxml2, gettext,
intltool, gtk-doc, libtool, and the other development packages required by
each upstream `configure.ac`. The upstream configure checks are authoritative;
optional LXPanel plugins may require additional libraries. Install an X
session starter (`xinit` or a display manager), `xterm`, and the usual X11
fonts/runtime files as well. These prerequisites are not built by the script
below.

Run the following inside the LFS chroot with the target development
dependencies installed. `PREFIX` defaults to `/usr`; set `JOBS` to limit
parallel compilation. The script bootstraps and installs the five checked-out
components. It builds `libfm-extra` first, then `menu-cache`, then full `libfm`
because of their upstream dependency relationship.

```sh
cd /sources/linsoft-desktop
LINSOFT_LFS_CHROOT=1 PREFIX=/usr JOBS=2 sh linsoft/build-desktop
```

The explicit `LINSOFT_LFS_CHROOT=1` confirmation prevents accidental installs
into the build host. Set it only after entering the Linsoft LFS chroot.

The bootstrap steps generate Autotools files in the upstream checkouts. They
are normal build outputs; preserve any local source edits before building.

After installing the components, install `linsoft/linsoft-session` on `PATH`
and `linsoft/data/xsessions/linsoft.desktop` in the display manager's session
directory. The build script installs both under `PREFIX` (by default
`/usr/bin` and `/usr/share/xsessions`). As the desktop user, run
`sh linsoft/install-user-config` to install the Openbox menu, LXPanel profile,
PCManFM defaults, and file-manager/terminal launchers. Existing user files are
preserved. Select Linsoft Desktop in the display manager, or start it from an
existing X server; `linsoft-session` does not start Xorg.

The panel's application menu uses installed desktop entries. The Linsoft
configuration menu currently provides file-manager and terminal access,
desktop preferences, panel settings, restart, and log out. It does not yet
include the former DRM desktop's Store or update manager. There is no online
Linsoft package repository configured, so those features cannot safely install
or update applications yet.

## Upstream revisions downloaded

- Openbox: `3a2fbc65186c54855915490ec3d0b4b2ddc079ea`
- LXPanel: `4dec3d0d3d8f9e2a9d14dd1b099d378289b67db4`
- PCManFM: `e8a1d49b89db68bf2fd6b022961c47d82d8f9dd9`
- libfm: `7e575d13fcf0532fc181fc26391ad6fd7717ed66`
- menu-cache: `11bc3d4c66faab16f10c6071f6a0923d922e7652`

These are shallow Git checkouts. Their remotes and checked-out revisions are
recorded in each component's `.git` metadata.
