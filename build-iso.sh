#!/bin/bash
set -e

# bnoss Desktop OS Build Script
# Creates a bootable Ubuntu-based desktop ISO with XFCE4, Chromium, and utilities

ROOTFS="./rootfs"
ISOROOT="./isoroot"
DISTRO="jammy"  # Ubuntu 22.04

echo "=== Step 1: Bootstrap Ubuntu base system ==="
sudo debootstrap --arch=amd64 "$DISTRO" "$ROOTFS" http://archive.ubuntu.com/ubuntu

echo "=== Step 2: Configure chroot ==="
# Mount necessary filesystems
sudo mount --bind /dev "$ROOTFS/dev"
sudo mount --bind /dev/pts "$ROOTFS/dev/pts"
sudo mount -t proc proc "$ROOTFS/proc"
sudo mount -t sysfs sysfs "$ROOTFS/sys"

# Configure apt sources
sudo tee "$ROOTFS/etc/apt/sources.list" > /dev/null << EOF
deb http://archive.ubuntu.com/ubuntu $DISTRO main restricted universe multiverse
deb http://archive.ubuntu.com/ubuntu $DISTRO-updates main restricted universe multiverse
deb http://archive.ubuntu.com/ubuntu $DISTRO-security main restricted universe multiverse
EOF

# Set hostname
echo "bnoss" | sudo tee "$ROOTFS/etc/hostname" > /dev/null

# Configure hosts
sudo tee "$ROOTFS/etc/hosts" > /dev/null << EOF
127.0.0.1   localhost
127.0.1.1   bnoss
EOF

# Set locale
sudo chroot "$ROOTFS" bash -c "
export DEBIAN_FRONTEND=noninteractive
apt-get update
apt-get install -y locales
locale-gen en_US.UTF-8
update-locale LANG=en_US.UTF-8
"

echo "=== Step 3: Install kernel and core system ==="
sudo chroot "$ROOTFS" bash -c "
export DEBIAN_FRONTEND=noninteractive
apt-get install -y \
    linux-image-generic \
    systemd \
    systemd-sysv \
    sudo \
    dbus \
    udev \
    network-manager \
    iproute2 \
    iputils-ping \
    wget \
    curl \
    nano \
    vim-tiny \
    bash-completion \
    ca-certificates \
    apt-utils
"

echo "=== Step 4: Install XFCE4 desktop environment ==="
sudo chroot "$ROOTFS" bash -c "
export DEBIAN_FRONTEND=noninteractive
apt-get install -y \
    xfce4 \
    xfce4-terminal \
    xfce4-settings \
    xfce4-panel \
    xfce4-session \
    xfce4-appfinder \
    thunar \
    mousepad \
    lightdm \
    lightdm-gtk-greeter \
    xorg \
    xserver-xorg \
    xinit \
    dbus-x11 \
    adwaita-icon-theme \
    gnome-themes-extra \
    fonts-dejavu-core \
    fonts-liberation
"

echo "=== Step 5: Install Chromium browser ==="
sudo chroot "$ROOTFS" bash -c "
export DEBIAN_FRONTEND=noninteractive
apt-get install -y chromium-browser || apt-get install -y chromium
"

echo "=== Step 6: Create user account ==="
sudo chroot "$ROOTFS" bash -c "
useradd -m -s /bin/bash -G sudo,adm,cdrom,audio,video,plugdev user
echo 'user:user' | chpasswd
echo 'root:root' | chpasswd
"

echo "=== Step 7: Configure auto-login ==="
sudo mkdir -p "$ROOTFS/etc/lightdm/lightdm.conf.d"
sudo tee "$ROOTFS/etc/lightdm/lightdm.conf.d/50-autologin.conf" > /dev/null << EOF
[Seat:*]
autologin-user=user
autologin-user-timeout=0
user-session=xfce
EOF

# Set default target to graphical
sudo chroot "$ROOTFS" systemctl set-default graphical.target

echo "=== Step 8: Configure networking ==="
sudo chroot "$ROOTFS" systemctl enable NetworkManager

# Create a desktop shortcut for the terminal
sudo mkdir -p "$ROOTFS/home/user/Desktop"
sudo tee "$ROOTFS/home/user/Desktop/terminal.desktop" > /dev/null << EOF
[Desktop Entry]
Version=1.0
Type=Application
Name=Terminal
Exec=xfce4-terminal
Icon=utilities-terminal
Terminal=false
Categories=System;TerminalEmulator;
EOF

# Create a desktop shortcut for Chromium
sudo tee "$ROOTFS/home/user/Desktop/chromium.desktop" > /dev/null << EOF
[Desktop Entry]
Version=1.0
Type=Application
Name=Chromium Browser
Exec=chromium-browser --no-sandbox
Icon=chromium-browser
Terminal=false
Categories=Network;WebBrowser;
EOF

# Create a desktop shortcut for Files
sudo tee "$ROOTFS/home/user/Desktop/files.desktop" > /dev/null << EOF
[Desktop Entry]
Version=1.0
Type=Application
Name=Files
Exec=thunar
Icon=system-file-manager
Terminal=false
Categories=System;FileManager;
EOF

sudo chmod +x "$ROOTFS/home/user/Desktop/"*.desktop
sudo chroot "$ROOTFS" chown -R user:user /home/user

echo "=== Step 9: Clean up chroot ==="
sudo chroot "$ROOTFS" apt-get clean
sudo chroot "$ROOTFS" rm -rf /var/cache/apt/archives/* /tmp/* /var/tmp/*

# Unmount chroot filesystems
sudo umount "$ROOTFS/dev/pts" || true
sudo umount "$ROOTFS/dev" || true
sudo umount "$ROOTFS/proc" || true
sudo umount "$ROOTFS/sys" || true

echo "=== Step 10: Build live ISO ==="
mkdir -p "$ISOROOT"/{boot/grub,live}

# Find and copy kernel + initrd
VMLINUZ=$(ls "$ROOTFS"/boot/vmlinuz-* 2>/dev/null | head -1)
INITRD=$(ls "$ROOTFS"/boot/initrd.img-* 2>/dev/null | head -1)

if [ -z "$VMLINUZ" ] || [ -z "$INITRD" ]; then
    echo "ERROR: Kernel or initrd not found!"
    ls -la "$ROOTFS/boot/"
    exit 1
fi

cp "$VMLINUZ" "$ISOROOT/boot/vmlinuz"
cp "$INITRD" "$ISOROOT/boot/initrd.img"

# Install live-boot in the rootfs for live booting
sudo mount --bind /dev "$ROOTFS/dev"
sudo mount --bind /dev/pts "$ROOTFS/dev/pts"
sudo mount -t proc proc "$ROOTFS/proc"
sudo mount -t sysfs sysfs "$ROOTFS/sys"

sudo chroot "$ROOTFS" bash -c "
export DEBIAN_FRONTEND=noninteractive
apt-get update
apt-get install -y live-boot
"

sudo umount "$ROOTFS/dev/pts" || true
sudo umount "$ROOTFS/dev" || true
sudo umount "$ROOTFS/proc" || true
sudo umount "$ROOTFS/sys" || true

# Create squashfs
sudo mksquashfs "$ROOTFS" "$ISOROOT/live/filesystem.squashfs" -comp xz -e boot

# GRUB config
cat > "$ISOROOT/boot/grub/grub.cfg" << 'GRUBCFG'
set timeout=5
set default=0

menuentry "bnoss Desktop" {
    linux /boot/vmlinuz boot=live toram quiet splash
    initrd /boot/initrd.img
}

menuentry "bnoss Desktop (Safe Mode)" {
    linux /boot/vmlinuz boot=live toram nomodeset quiet
    initrd /boot/initrd.img
}
GRUBCFG

echo "=== Step 11: Generate ISO ==="
grub-mkrescue -o bnoss.iso "$ISOROOT"

echo "=== Build complete! ==="
ls -lh bnoss.iso
