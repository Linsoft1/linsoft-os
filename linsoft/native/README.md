# Linsoft Native Desktop

An original C/Xlib/Xft program supplies the window manager, bottom panel,
Start menu, background and file manager together. It runs over the existing
Xorg server and Debian application environment on the LFS host. It does not
replace Linux, Xorg, drivers, or the existing application packages.

The separate `Linsoft-Desktop-Native.qcow2` image is launched with
[`Start-Linsoft-Native.cmd`](../../../Start-Linsoft-Native.cmd).
The normal Windows launcher now defaults to this disk, displayed as
Linsoft Desktop Professional. The Native disk filename and internal identifiers
remain unchanged for compatibility with existing backups and helpers.
The old Openbox edition's disk is preserved, but its Windows launcher is removed.
The active image has only Native: Openbox, tint2 and PCManFM packages are purged
without automatic dependency removal. Xorg, application libraries, XFCE settings
and notifications remain necessary components, not additional desktop sessions.
Neither launcher copies files between the two disks. Native inherits the owner's existing
autologin/passwordless fixed-action policy; it does not enable unrestricted
sudo commands.

## Original Linsoft icons

The Native shell draws its own vector-style icon set with Xlib, without
external icon fonts, bitmap files or third-party artwork.
The same scalable line drawings and cyan/blue, mint, gold and violet palette
are used on the desktop, Start menu, panel, task buttons, window titles and
file-manager toolbar/list. Text labels remain next to icons.

The set covers the Linsoft mark, folders, terminal, browser, documents,
spreadsheets, settings, applications, sound, restart, power, generic files,
pictures, audio, video, archives, home, parent directory, new folder, network,
battery and USB.
File types are selected by case-insensitive extensions; unknown files keep a
generic document icon. These are visual hints, not MIME validation. Task and
title icons use the application's X11 WM_CLASS, with a generic application
icon for unknown classes. They do not replace Firefox/LibreOffice branding
inside those applications or install a system-wide GTK icon theme.

The desktop provides one-click shortcuts for files, terminal, browser, Writer,
Calc, settings and application management. The panel uses distinct network,
battery and USB icons, wider task buttons, and 24- to 28-pixel workspace targets.
Slovak interface labels include UTF-8 diacritics.

## Controls

- Bottom Start menu: files, terminal, Firefox, Writer, Calc, settings,
  application management, sound, restart and shutdown.
- Task buttons: focus/restore a window, or minimize the focused window.
- Title bar: left-drag to move; right-drag its blank area to resize.
  Drag an edge to resize; double-click the title to toggle maximize.
  The three buttons minimize, toggle maximize, and close. Normal resizing
  respects application minimum/maximum sizes and character-cell increments.
- `Super+E`: files; `Super+T`: terminal; `Super+I`: settings.
- `Super+Space`: Start; `Super+P`: Native desktop preferences.
- Type in Start to filter built-in actions and up to 255 visible installed
  desktop applications. Matching is case-insensitive UTF-8; Up/Down, Enter,
  Backspace and Escape work while searching. Empty searches show the original
  ten actions. GIO launches desktop files without evaluating Exec text in a shell.
- `Alt+Tab`: cycle applications; `Alt+F4`: close.
- `Super+Up`: toggle maximize; `Super+Down`: minimize.
- `Super+Left/Right`: snap to the left/right half. Moving a title to a screen
  edge snaps the window; the top edge maximizes. Maximize/restore retains
  the normal geometry. Transient modal dialogs are centered over their parent
  and receive focus instead of that parent while open.
- `Super+1` through `Super+4`: switch workspace; add Shift to move the focused
  window and its transient dialogs there. Numbered panel buttons also switch.
  Task buttons and Alt+Tab stay on the current workspace. Layouts are not
  restored after logout.
- Files: double-click or Enter to open, Backspace to go up, mouse wheel
  to scroll, arrows to select, F5 to refresh.
- `Ctrl+Shift+N`: create a folder; `F2`: rename; Enter confirms and Escape
  cancels name editing. Backspace removes the last character.
- `Ctrl+L`: edit the absolute location; `Alt+Left/Right`: navigation history;
  `Ctrl+H`: show/hide hidden files. Toolbar buttons provide the same actions.
- `Ctrl+T`: duplicate the current file tab (up to eight); `Ctrl+W`: close it;
  `Ctrl+Tab` / `Ctrl+Shift+Tab`: cycle. Each tab keeps its path, history,
  hidden-file choice, name filter and scroll position.
- `Ctrl+F` or Hľadať: filter names in the current directory. Enter applies;
  an empty query clears it. This is not recursive or file-content search.
- Selected images get aspect-preserving previews through installed GdkPixbuf
  decoders, including PNG/JPEG, up to 24 megapixels. No thumbnails are written
  to user directories. Animated images use the decoder's static frame.
- Drag selected file rows onto a directory row, another tab, or Vyssie to
  choose Copy or Move in a confirmation dialog. Existing data is not
  deliberately overwritten. This is internal dragging, not cross-application XDND.
- Ctrl-click toggles individual selections; Shift-click/arrows extends a
  range; `Ctrl+A` selects all displayed entries.
- `Ctrl+C` and `Ctrl+V`: copy the selection to the current folder. `Ctrl+X`
  followed by `Ctrl+V` moves it. Existing
  names are rejected; copying never deliberately overwrites another item.
  Copies use GNU coreutils `cp --update=none-fail` and preserve symlinks.
- Delete requests confirmation before moving selected items to GIO trash.
  The Kos toolbar/context action opens a restore dialog using GVfs on the
  session D-Bus. Existing restore destinations are refused. File operations
  run asynchronously with an automatic refresh on completion; Escape safely
  cancels copy/move and restores staged items. Trash jobs cannot be cancelled
  after they start because GIO trash operations may already be partially done.
  Use F5 after restoring through the separate trash dialog.
- Right-click the desktop or file list for a context menu. `Menu` or
  `Shift+F10` opens the file context menu. Both menus support Up/Down,
  Enter and Escape. File-name/location editing uses XIM UTF-8 input,
  supports composed characters, Ctrl+A selects the field, and Backspace
  removes one UTF-8 character.

## Desktop preferences and system panel

Native preferences offer three background colors, three accent colors,
32/48/64-pixel desktop icons, a second pinned application (terminal,
Firefox, Writer or Calc), and PNG wallpaper up to 24 megapixels. PNGs are
decoded and validated before saving. Wallpapers stretch to the screen;
desktop text keeps its foreground color, so choose a sufficiently dark image.
The five-line configuration is atomically saved with mode 0600 in
`${XDG_CONFIG_HOME:-$HOME/.config}/linsoft-native/desktop.conf` and read on
startup or `linsoft-native --reload`. Its fields are theme index (0-2),
accent index (0-2), icon size, pinned launcher index (1-4), and an absolute
PNG path or `-`. Keep the wallpaper file available after reboot.

The panel shows PulseAudio volume/mute, active network-interface state,
reported battery capacity, and a count of USB devices. Status refreshes
every five seconds. The Native session waits briefly for PulseAudio to accept
connections before starting the status watcher. `LAN` means an interface is
up, not verified Internet access. `AC` means no battery was detected; USB
counts include hubs and
input devices, not only storage. A virtual machine cannot establish real
laptop-battery or removable-device behavior.

The default Windows launcher requests the primary display's resolution, capped
at 3840x2160 and with a 1280x720 minimum, so ultrawide monitors do not receive a
stretched 1920x1080 desktop. Smaller displays use QEMU's zoom-to-fit rendering.

The default QEMU display uses 2D VirtIO and GTK with Mesa software rendering.
On this WSL host, QEMU's EGL/Zink reports that it cannot choose an OpenGL
device, so the default launcher avoids the unverified accelerated path. This
favors predictable display over 3D performance. It is virtualized 2D display,
not Intel PCI passthrough; it does not validate bare-metal i915 or 3D
acceleration.

Panel controls reuse the existing sound, network, fixed DHCP renewal,
USB/disk overview and safe mount/unmount tools. Notification configuration
and history remain provided by the existing XFCE notification service.
The document button opens Centrum Linsoft: notification history/settings,
calendar, device overview, mount and safe unmount. Clicking the clock opens
the calendar directly; selecting a date does not change the system clock.
The gear opens Native preferences. GTK application styling remains separate.

The status watcher also detects changes in sysfs USB/removable block devices
(whole disks, excluding partitions). It posts a notification with an
"Otvorit zariadenia" action leading to the existing device controls. It does
not automatically mount media or bypass the fixed host authorization path.
Notification popups keep the notification service's placement without window
decorations or stealing application focus.
Status polling runs every five seconds, so very brief attachments may be missed.

## Verified backups and manual recovery

Close Native normally before running
[`Update-Linsoft-Native.cmd`](../../../Update-Linsoft-Native.cmd). It makes a
standalone full backup, checks QCOW2 structure, compares guest-visible contents,
and writes an edition/timestamp/SHA256 receipt. It then offers to start Native;
open Applications -> Install updates and explicitly confirm the backup.
That confirmation is an owner attestation, not a guest-verified backup token.

For a failed update, shut down Native and run
[`Manage-Linsoft-Native.cmd`](../../../Manage-Linsoft-Native.cmd), select
Restore, then confirm the chosen backup. Receipt-bearing backups are checked
for checksum and edition. Older backups without a receipt still require manual
confirmation and structural validation. The previous disk is preserved before
the verified replacement is installed.

**Recovery replaces the complete disk, including documents. Export newer
documents first. It is not automatic rollback or a separate data-volume scheme.**
Never back up, restore or mount a disk while QEMU owns it. Both the default
launcher/recovery manager and the Native-specific launchers select the Native
disk. Pass `-ImageName Linsoft-Desktop-Professional.qcow2` to manage the legacy disk.

Errors are retained in the session log and displayed in the file manager;
failed child operations also display a message on the desktop.
Restart/shutdown still require an explicit confirmation, but not a password
on this owner's passwordless installation.

## Build and installation

Runtime and build dependencies are declared in
[`packages-native`](../debian/packages-native) and
[`packages-native-build`](../debian/packages-native-build). Build against
the Debian 13 application root's ABI, not the LFS host's libraries:

```sh
make
```

The build enables C11, `-Wall -Wextra -Werror -pedantic`, with X11, Xft, libpng,
GIO/GLib and GdkPixbuf
flags from pkg-config. The checked binary is installed by
[`install-native-desktop`](../install-native-desktop).

For offline deployment, close the separate Native VM, enter a private mount
namespace (`unshare --mount --propagation private`), connect only its disk
with qemu-nbd, and mount the LFS filesystem at `/mnt/linsoft-debian-image`.
Mount the application root's proc/dev/sys and arrange DNS as in the existing
offline deployment workflow. Install the declared dependencies there using
signed Debian APT repositories, then run:

```sh
LINSOFT_LFS_BUILD=1 LINSOFT_NATIVE_IMAGE=1 bash linsoft/install-native-desktop
```

The installer requires an existing autologin-enabled Professional-derived image, checks
dependencies, compiles the original program, installs the session integration,
protects its runtime packages from application-manager removal, and creates
the root-owned `/etc/linsoft-apps/native` opt-in. It does not run the base
Professional installer or alter passwords. Fully unmount and disconnect the
disk before booting or checking its filesystem.

It simulates removal of Openbox/tint2/PCManFM and refuses a plan that changes
additional packages. It then purges only those three packages, removes them
from the protected-package list and replaces old PCManFM directory associations
with Linsoft Files. User documents and old configuration files are retained.
`linsoft-files DIRECTORY`, including directory links opened through `xdg-open`,
opens that exact directory in Native. A bounded per-request X11 exchange reports
invalid paths or failed requests explicitly instead of silently opening Home.
The declared `file` runtime dependency supplies MIME detection for `xdg-open`.
The Native session declares the standard generic XDG desktop backend so these
links do not probe unavailable GNOME/KDE session services.
Removing the `native` marker is not a rollback: those legacy programs are no
longer installed. Use a verified full-disk backup for manually confirmed recovery.

## Regression test

Inside the Debian application root, run as `linsoft`:

```sh
bash /usr/local/src/linsoft-native/test-native
```

The test uses its own Xvfb display `:91` and an isolated test home under
`/var/tmp/linsoft-native-test`; it refuses an existing test home/display.
It starts a private D-Bus session and verifies actual file contents,
enumeration of a directory with 8193 entries,
copy/multi-selection move, no-overwrite behavior, confirmed trash and restore
including destination collisions, location/history, UTF-8/composed-character
input, keyboard menus, PNG decoding and exact wallpaper pixels, actual
preferences dialogs and desktop-restart persistence. Window tests measure
movement/edge resizing, double-click maximize/restore and half-screen
snapping, minimize/panel restore, focus, Alt+Tab and close. Control commands
without a Native session are refused.
New-feature tests also verify independent tabs, name-filtered rename, PNG/JPEG
preview RGB values (with JPEG-loss tolerance), real mouse-drop copy/multi-item
move, installed-application Start search,
empty results, workspace transfer/visibility, and actual center/calendar dialogs.
[`test-native-actions`](../test-native-actions) checks confirmed drop copy/move,
symlink preservation, collisions, cancellation/errors, rollback on injected
copy/move failures, safe cancellation during move staging and commit, and the
Native update gate.
Run it in the Debian application root, whose coreutils supports `none-fail`.
The Windows backup manager was also exercised on an isolated small QCOW2
fixture: exact backup/restore contents, rejection without confirmation,
receipt checksum/edition rejection, and preservation of the replaced disk.
It also builds/runs `test-native-icons`, which checks all 22 distinct icon
drawings at eight sizes (16 through 96 pixels), visible pixels inside their
bounds, the shared palette, file-extension classification and X11 WM_CLASS
selection. Additional real-X11 fixtures verify modal-dialog focus, centering,
return to the parent, and preservation of modal EWMH state across maximize.
Twenty-five file-window open/close cycles check that file-descriptor count
stays unchanged and resident-memory growth remains below 8 MiB.
The deployment test removes that exact temporary home after retaining its
log. For a manual test, inspect its log and remove that test directory before
rerunning. This headless test is not a substitute for booting the actual VM.

The separate image also passed actual QEMU startup as UID 1000 with no
Openbox/tint2/PCManFM processes, GUI folder creation, real Writer, Calc and
Firefox windows, settings and application-manager dialogs, shutdown
cancellation, and the fixed passwordless helper. A real reboot changed
`/proc/sys/kernel/random/boot_id` while retaining the document and GUI-created
folder. Confirmed shutdown completed normally. The closed disk subsequently
passed read-only ext4 and QCOW2 checks; temporary test data is not part of
the desktop release.

The expanded image also passed an actual system-panel mute/unmute operation
against its PulseAudio sink and retained icon-size/pinned-app settings across
a real kernel reboot. A final clean boot verified panel placement while
switching 1920x1080 to 1280x800 and back with RandR. Its EDID mode list did
not initially contain 1280x800, so the test added that mode for the running
X session; no persistent display configuration or kernel change was needed.

## Remaining limits

This is a small, functional first desktop, not a KDE/GNOME replacement.
Its window manager implements a basic subset of ICCCM/EWMH, not advanced
tiling, session restoration or a Wayland compositor.
There is no compositor, tray/notification-area protocol, cross-application
drag-and-drop, file-content search or native notification daemon.
Hidden files start omitted. Directory entries grow dynamically, subject to
available memory and the X11 window manager's integer-index range. Large bulk
operations remain subject to the operating system's argument-size limits.

Bulk copy/move operations stage a full selection before committing it and roll
back committed items if preparation or commit fails. If the filesystem also
prevents rollback, the recovery data is retained in a hidden temporary directory
and its location is reported. Moving a selection to Trash remains a sequence
of GIO operations and can be partially completed if a later item fails. There
is no byte-level file-operation progress or file-operation undo beyond trash restore.
There is no cross-application clipboard or graphical multi-monitor
configuration. ICCCM/EWMH and accessibility support
are limited rather than complete. Testing at 1280x800 and the VM's 1920x1080 does not establish
compatibility with all screen sizes, graphics devices or applications.

Application updates, protected packages, fixed privileged helpers and existing
backup/restore tools are retained rather than replaced. Take a closed-image
backup before upgrading this experimental desktop; this is not automatic
snapshot rollback. The legacy Professional disk remains unchanged; Native is
now the default Linsoft Desktop Professional.
