#!/bin/sh
set -eu

if [ "$(id -u)" -ne 0 ]; then
    printf '%s\n' "Spusti skript ako správca: sudo bash ./build-kernel.sh" >&2
    exit 1
fi

project_root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
kernel_version=6.12.113
kernel_release="${kernel_version}-linsoft-amd64"
kernel_localversion="-linsoft-amd64"
kernel_build_root="$project_root/build/linux-$kernel_version"
kernel_source="$kernel_build_root/linux-$kernel_version"
kernel_archive="$kernel_build_root/linux-$kernel_version.tar.xz"
kernel_config="$project_root/config/kernel/linsoft-amd64.config"
package_dir="$project_root/config/packages.chroot"
meta_package="linux-image-linsoft-amd64"
meta_version="${kernel_version}-linsoft1"

for tool in bc bison curl dpkg-deb flex gcc getconf make openssl pahole perl tar xz; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        printf 'Chýba nástroj %s. Doinštaluj build závislosti podľa README.md.\n' "$tool" >&2
        exit 1
    fi
done

if [ ! -f "$kernel_config" ]; then
    printf 'Chýba konfigurácia jadra: %s\n' "$kernel_config" >&2
    exit 1
fi

mkdir -p "$kernel_build_root" "$package_dir"

if [ ! -f "$kernel_archive" ]; then
    curl --fail --location --retry 3 \
        "https://cdn.kernel.org/pub/linux/kernel/v6.x/linux-$kernel_version.tar.xz" \
        --output "$kernel_archive"
fi
xz --test "$kernel_archive"

if [ ! -f "$kernel_source/Makefile" ]; then
    tar --extract --file "$kernel_archive" --directory "$kernel_build_root"
fi

cd "$kernel_source"
cp "$kernel_config" .config
scripts/config --file .config \
    --set-str LOCALVERSION "$kernel_localversion" \
    --disable LOCALVERSION_AUTO \
    --disable MODULE_SIG \
    --set-str SYSTEM_TRUSTED_KEYS "" \
    --set-str SYSTEM_REVOCATION_KEYS ""
make olddefconfig

for required_setting in \
    CONFIG_BLK_DEV_INITRD \
    CONFIG_DEVTMPFS \
    CONFIG_EFI \
    CONFIG_MODULES \
    CONFIG_VIRTIO; do
    if ! grep -q "^$required_setting=y$" .config; then
        printf 'Konfigurácia jadra musí mať zapnuté %s.\n' "$required_setting" >&2
        exit 1
    fi
done

for required_module in CONFIG_BLK_DEV_LOOP CONFIG_OVERLAY_FS CONFIG_SQUASHFS; do
    if ! grep -q "^$required_module=[ym]$" .config; then
        printf 'Konfigurácia jadra musí obsahovať modul %s.\n' "$required_module" >&2
        exit 1
    fi
done

configured_release=$(make --no-print-directory kernelrelease)
if [ "$configured_release" != "$kernel_release" ]; then
    printf 'Neočakávaná verzia jadra: %s (očakávaná %s).\n' "$configured_release" "$kernel_release" >&2
    exit 1
fi

rm -f \
    "$package_dir"/linux-image-"$kernel_release"_*.deb \
    "$package_dir"/"$meta_package"_*.deb

jobs=$(getconf _NPROCESSORS_ONLN)
if [ "$jobs" -gt 4 ]; then
    jobs=4
fi

make --no-print-directory -j"$jobs" bindeb-pkg \
    KDEB_PKGVERSION="${kernel_version}-linsoft1" \
    DPKG_FLAGS=-d \
    KBUILD_BUILD_USER=linsoft \
    KBUILD_BUILD_HOST=build

set --
for image_package in "$kernel_build_root"/linux-image-"$kernel_release"_*.deb; do
    if [ -f "$image_package" ]; then
        set -- "$@" "$image_package"
    fi
done
if [ "$#" -ne 1 ]; then
    printf 'Očakával sa jeden balík linux-image jadra %s, nájdených: %s.\n' \
        "$kernel_release" "$#" >&2
    exit 1
fi

image_package=$1
image_package_name=$(dpkg-deb --field "$image_package" Package)
image_package_version=$(dpkg-deb --field "$image_package" Version)
if [ "$image_package_name" != "linux-image-$kernel_release" ]; then
    printf 'Vytvorený balík jadra má neočakávaný názov: %s.\n' "$image_package_name" >&2
    exit 1
fi

meta_root=$(mktemp -d "$kernel_build_root/linux-image-linsoft.XXXXXX")
trap 'rm -rf "$meta_root"' EXIT HUP INT TERM
mkdir -p "$meta_root/DEBIAN"
cat > "$meta_root/DEBIAN/control" <<EOF
Package: $meta_package
Version: $meta_version
Section: kernel
Priority: optional
Architecture: all
Maintainer: Linsoft OS <noreply@linsoft.invalid>
Depends: $image_package_name (= $image_package_version)
Description: Linsoft Linux kernel metapackage
 Custom upstream Linux kernel and matching modules built for Linsoft OS.
EOF

cp "$image_package" "$package_dir/"
dpkg-deb --build "$meta_root" "$package_dir/${meta_package}_${meta_version}_all.deb" >/dev/null

printf 'Vytvorené jadro: %s (%s)\n' "$image_package_name" "$image_package_version"
