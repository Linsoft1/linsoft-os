#!/bin/sh
set -eu

if [ "$(id -u)" -ne 0 ]; then
    printf '%s\n' "Spusti skript ako správca: sudo bash ./build.sh" >&2
    exit 1
fi

lb config \
    --mode debian \
    --distribution trixie \
    --architectures amd64 \
    --archive-areas "main contrib non-free non-free-firmware" \
    --security false \
    --initsystem systemd \
    --binary-images iso-hybrid \
    --debian-installer live \
    --debian-installer-gui true \
    --win32-loader false \
    --uefi-secure-boot disable \
    --linux-flavours amd64 \
    --linux-packages linux-image-linsoft \
    --iso-application "Linsoft OS 1.5" \
    --iso-publisher "Linsoft" \
    --iso-volume "LINSOFT" \
    --bootappend-live "boot=live components quiet splash hostname=linsoft username=linsoft user-fullname=Linsoft locales=sk_SK.UTF-8 keyboard-layouts=sk,us"

openbox_defaults="config/includes.chroot/etc/xdg/openbox"
openbox_user_defaults="config/includes.chroot/etc/skel/.config/openbox"
mkdir -p "$openbox_user_defaults"
cp "$openbox_defaults/rc.xml" "$openbox_defaults/menu.xml" "$openbox_user_defaults/"
cp config/includes.chroot/usr/share/icons/Linsoft-Icons/scalable/apps/distributor-logo-linsoft.svg \
    config/includes.chroot/etc/skel/.face
chmod +x config/includes.chroot/usr/bin/linsoft-hub config/includes.chroot/usr/bin/linsoft-store config/includes.chroot/usr/lib/linsoft/store-helper
chmod +x "$openbox_defaults/autostart"

chmod +x config/hooks/normal/9000-linsoft-branding.hook.binary
chmod +x config/hooks/normal/7000-linsoft-plymouth.hook.chroot
chmod +x build-kernel.sh
./build-kernel.sh
lb clean --all
MKSQUASHFS_OPTIONS="${MKSQUASHFS_OPTIONS:-} -b 1048576" lb build
