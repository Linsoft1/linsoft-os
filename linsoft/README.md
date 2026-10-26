# Linsoft session integration

This folder is the downstream customization area; upstream source remains
unmodified in the sibling component directories.

The separate [Native desktop](native/README.md) adds an original
C/Xlib/Xft window manager, panel and file manager in one program. Its
`/etc/linsoft-apps/native` marker selects the new primary session without
starting the legacy three desktop components. `install-native-desktop`
targets a closed, separately copied Native image. The default Windows launcher
now selects that image as Linsoft Desktop Professional; the old Openbox-based
Professional image and backups are preserved without a separate Windows launcher.
The active Native image removes Openbox/tint2/PCManFM; necessary Xorg,
application libraries, settings and notification services remain installed.
Native also includes edge resizing/snapping, multi-selection copy/move,
navigation history, trash restore, UTF-8 editing, persistent desktop
preferences and a live system-status panel. Its exact controls and remaining
limits are documented in the linked Native guide.
The next Native expansion adds installed-application Start search, eight file
tabs with name filtering and image previews, confirmed internal drag/copy/move,
four workspaces, a calendar/notification center and removable-storage change
notifications. Native-specific Windows update/recovery launchers select its
separate disk and verify full backups before manually confirmed restoration.

The preserved legacy Professional image uses packaged Debian Xorg/Openbox,
PCManFM and tint2, with password login/locking, application management and
quick controls described in [the main README](../README.md).
Its repeatable offline deployment script is `install-professional-desktop`,
which extends `install-desktop-extras`, `install-unified-desktop` and the
original boot splash after checking optional dependencies and the Slovak
locale. It preserves configured passwords and documents on reinstall.
Related shell tests are `test-professional-actions`, `test-unified-actions`,
`test-desktop-extras` and the root/build-environment
`test-desktop-host-actions`. Complete and Unified remain preserved as earlier
editions. Offline maintenance requires a closed image in a private mount
namespace; unmount it completely before filesystem checks or booting it.
The source-build workflow below is separate from those prebuilt images.

`build-desktop` builds and installs the five upstream components after their
system prerequisites are available. Run it from the source root inside the
target LFS chroot, for example
`LINSOFT_LFS_CHROOT=1 PREFIX=/usr JOBS=2 sh linsoft/build-desktop`. The
confirmation variable prevents accidentally installing into the host system.
It builds `libfm-extra`, `menu-cache`, full `libfm`, Openbox, LXPanel, and
PCManFM in dependency order. It does not build Xorg, GTK, or other prerequisite
libraries.

The build script installs `linsoft-session` in `PREFIX/bin` and the session
entry in `PREFIX/share/xsessions`. The launcher starts the desktop components
inside an existing X session, so install an X server and `xterm` as well. It
starts LXPanel and the PCManFM desktop with the `Linsoft` profile before
starting `openbox-session`.

Run `sh install-user-config` as the desktop user to install the Openbox menu,
LXPanel profile, PCManFM defaults, and app launchers under the active XDG
configuration/data directories. Existing files are preserved. The X session
entry is in `data/xsessions/linsoft.desktop`; install it in the display
manager's session directory after installing `linsoft-session`.
