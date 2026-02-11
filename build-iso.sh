#!/bin/bash
set -e

# ╔══════════════════════════════════════════════════════════╗
# ║  BlazeNeuro OS — Build Script                           ║
# ║  Custom Linux distribution with kernel from kernel.org  ║
# ║  Change KERNEL_URL below to update the kernel version.  ║
# ╚══════════════════════════════════════════════════════════╝

# ── Configuration ──────────────────────────────────────────
#    Change this URL to upgrade/downgrade the Linux kernel
KERNEL_URL="https://cdn.kernel.org/pub/linux/kernel/v6.x/linux-6.12.11.tar.xz"

OS_NAME="BlazeNeuro"
OS_VERSION="1.0"
OS_CODENAME="nova"
ROOTFS="./rootfs"
ISOROOT="./isoroot"
DISTRO="jammy"  # Base package repository (Debian/Ubuntu for apt packages only)
# ───────────────────────────────────────────────────────────

echo "=== Step 1: Bootstrap base system (package repository) ==="
echo "Using $DISTRO packages for userspace libraries only."
echo "Kernel will be compiled from: $KERNEL_URL"
sudo debootstrap --arch=amd64 "$DISTRO" "$ROOTFS" http://archive.ubuntu.com/ubuntu

echo "=== Step 2: Configure chroot ==="
sudo mount --bind /dev "$ROOTFS/dev"
sudo mount --bind /dev/pts "$ROOTFS/dev/pts"
sudo mount -t proc proc "$ROOTFS/proc"
sudo mount -t sysfs sysfs "$ROOTFS/sys"

# Full apt sources
sudo tee "$ROOTFS/etc/apt/sources.list" > /dev/null << EOF
deb http://archive.ubuntu.com/ubuntu $DISTRO main restricted universe multiverse
deb http://archive.ubuntu.com/ubuntu $DISTRO-updates main restricted universe multiverse
deb http://archive.ubuntu.com/ubuntu $DISTRO-security main restricted universe multiverse
deb http://archive.ubuntu.com/ubuntu $DISTRO-backports main restricted universe multiverse
EOF

echo "blazeneuro" | sudo tee "$ROOTFS/etc/hostname" > /dev/null

sudo tee "$ROOTFS/etc/hosts" > /dev/null << EOF
127.0.0.1   localhost
127.0.1.1   blazeneuro
EOF

sudo chroot "$ROOTFS" bash -c "
export DEBIAN_FRONTEND=noninteractive
apt-get update
apt-get install -y locales
locale-gen en_US.UTF-8
update-locale LANG=en_US.UTF-8
"

echo "=== Step 2b: Brand as $OS_NAME ==="
# Override OS identity files
sudo tee "$ROOTFS/etc/os-release" > /dev/null << OSREL
NAME="$OS_NAME"
VERSION="$OS_VERSION ($OS_CODENAME)"
ID=blazeneuro
ID_LIKE=debian
VERSION_ID=$OS_VERSION
VERSION_CODENAME=$OS_CODENAME
PRETTY_NAME="$OS_NAME $OS_VERSION"
HOME_URL="https://github.com/MilkywayRides/bnoss"
SUPPORT_URL="https://github.com/MilkywayRides/bnoss/issues"
OSREL

sudo tee "$ROOTFS/etc/lsb-release" > /dev/null << LSBREL
DISTRIB_ID=$OS_NAME
DISTRIB_RELEASE=$OS_VERSION
DISTRIB_CODENAME=$OS_CODENAME
DISTRIB_DESCRIPTION="$OS_NAME $OS_VERSION"
LSBREL

# Login banners
echo "$OS_NAME $OS_VERSION \\n \\l" | sudo tee "$ROOTFS/etc/issue" > /dev/null
echo "$OS_NAME $OS_VERSION" | sudo tee "$ROOTFS/etc/issue.net" > /dev/null

# Custom MOTD
sudo rm -rf "$ROOTFS/etc/update-motd.d"/*
sudo tee "$ROOTFS/etc/update-motd.d/00-blazeneuro" > /dev/null << 'MOTD'
#!/bin/bash
echo ""
echo "  ╔══════════════════════════════════════╗"
echo "  ║   Welcome to BlazeNeuro OS           ║"
echo "  ║   Type 'neofetch' for system info    ║"
echo "  ╚══════════════════════════════════════╝"
echo ""
MOTD
sudo chmod +x "$ROOTFS/etc/update-motd.d/00-blazeneuro"

# Remove Ubuntu legal notice and help URL
sudo rm -f "$ROOTFS/etc/legal" 2>/dev/null
sudo rm -f "$ROOTFS/etc/default/motd-news" 2>/dev/null

echo "=== Step 3: Install core system packages ==="
sudo chroot "$ROOTFS" bash -c "
export DEBIAN_FRONTEND=noninteractive
apt-get install -y \
    linux-firmware \
    systemd \
    systemd-sysv \
    sudo \
    dbus \
    udev \
    network-manager \
    nm-tray \
    iproute2 \
    iputils-ping \
    wget \
    curl \
    nano \
    vim-tiny \
    bash-completion \
    ca-certificates \
    apt-utils \
    software-properties-common \
    gpg \
    apt-transport-https \
    git \
    python3 \
    python3-pip \
    build-essential \
    unzip \
    zip \
    htop \
    neofetch \
    feh \
    picom \
    xorg \
    xserver-xorg \
    xinit \
    dbus-x11 \
    libgtk-3-dev \
    libvte-2.91-dev \
    libx11-dev \
    pkg-config \
    adwaita-icon-theme \
    papirus-icon-theme \
    fonts-inter \
    fonts-jetbrains-mono \
    fonts-noto-color-emoji \
    lightdm \
    lightdm-gtk-greeter \
    lightdm-gtk-greeter-settings \
    gnome-calculator \
    gnome-system-monitor \
    evince \
    mousepad \
    gnome-disk-utility \
    flatpak \
    podman \
    fuse3 \
    libfuse2 \
    wine64 \
    \
    mesa-utils \
    mesa-vulkan-drivers \
    libgl1-mesa-dri \
    xserver-xorg-video-intel \
    xserver-xorg-video-amdgpu \
    xserver-xorg-video-nouveau \
    xserver-xorg-video-fbdev \
    xserver-xorg-video-vesa \
    vainfo \
    intel-media-va-driver \
    \
    linux-firmware \
    \
    xserver-xorg-input-libinput \
    xserver-xorg-input-synaptics \
    xserver-xorg-input-evdev \
    xserver-xorg-input-wacom \
    \
    pulseaudio \
    pulseaudio-utils \
    pavucontrol \
    alsa-utils \
    \
    bluez \
    blueman \
    \
    cups \
    cups-browsed \
    system-config-printer \
    \
    ntfs-3g \
    exfat-fuse \
    exfatprogs \
    dosfstools \
    btrfs-progs \
    e2fsprogs \
    xfsprogs \
    \
    usb-modeswitch \
    usbutils \
    pciutils \
    lshw \
    inxi

# distrobox is unavailable in some Ubuntu releases/repositories
if apt-cache show distrobox >/dev/null 2>&1; then
    apt-get install -y distrobox
else
    echo '[INFO] distrobox package not available; skipping'
fi
"

echo "=== Step 3b: Compile custom Linux kernel from kernel.org ==="
KERNEL_TARBALL="$(basename "$KERNEL_URL")"
KERNEL_DIR="$(basename "$KERNEL_TARBALL" .tar.xz)"

# Download kernel source
if [ ! -f "/tmp/$KERNEL_TARBALL" ]; then
    echo "Downloading kernel from $KERNEL_URL ..."
    wget -q --show-progress -O "/tmp/$KERNEL_TARBALL" "$KERNEL_URL"
fi

sudo cp "/tmp/$KERNEL_TARBALL" "$ROOTFS/tmp/"

sudo mount --bind /dev "$ROOTFS/dev"
sudo mount --bind /dev/pts "$ROOTFS/dev/pts"
sudo mount -t proc proc "$ROOTFS/proc"
sudo mount -t sysfs sysfs "$ROOTFS/sys"

sudo chroot "$ROOTFS" bash -c "
export DEBIAN_FRONTEND=noninteractive

# Install kernel build dependencies
apt-get install -y \
    build-essential bc bison flex libssl-dev libelf-dev \
    cpio kmod initramfs-tools dwarves

cd /tmp
tar xf $KERNEL_TARBALL
cd $KERNEL_DIR

# Use default config optimized for desktop
make defconfig

# Enable essential desktop features
scripts/config --enable CONFIG_SMP
scripts/config --enable CONFIG_MODULES
scripts/config --enable CONFIG_MODULE_UNLOAD
scripts/config --enable CONFIG_DRM
scripts/config --enable CONFIG_DRM_I915
scripts/config --enable CONFIG_DRM_AMDGPU
scripts/config --enable CONFIG_DRM_NOUVEAU
scripts/config --enable CONFIG_DRM_FBDEV_EMULATION
scripts/config --enable CONFIG_FRAMEBUFFER_CONSOLE
scripts/config --enable CONFIG_FB_VESA
scripts/config --enable CONFIG_SOUND
scripts/config --enable CONFIG_SND
scripts/config --enable CONFIG_SND_HDA_INTEL
scripts/config --enable CONFIG_USB
scripts/config --enable CONFIG_USB_XHCI_HCD
scripts/config --enable CONFIG_USB_EHCI_HCD
scripts/config --enable CONFIG_INPUT_EVDEV
scripts/config --enable CONFIG_EXT4_FS
scripts/config --enable CONFIG_BTRFS_FS
scripts/config --enable CONFIG_NTFS3_FS
scripts/config --enable CONFIG_VFAT_FS
scripts/config --enable CONFIG_FUSE_FS
scripts/config --enable CONFIG_SQUASHFS
scripts/config --enable CONFIG_OVERLAY_FS
scripts/config --enable CONFIG_BLK_DEV_LOOP
scripts/config --enable CONFIG_NETFILTER
scripts/config --enable CONFIG_WIRELESS
scripts/config --enable CONFIG_CFG80211
scripts/config --enable CONFIG_MAC80211
scripts/config --enable CONFIG_BT
scripts/config --set-str CONFIG_DEFAULT_HOSTNAME blazeneuro
scripts/config --set-str CONFIG_LOCALVERSION -blazeneuro

# Build kernel (use all available cores)
make -j\$(nproc)

# Install modules and kernel
make modules_install
make install

# Generate initramfs for our kernel
KVER=\$(make kernelrelease)
update-initramfs -c -k \$KVER

# Clean up source
cd /tmp
rm -rf $KERNEL_DIR $KERNEL_TARBALL
"

sudo umount "$ROOTFS/dev/pts" || true
sudo umount "$ROOTFS/dev" || true
sudo umount "$ROOTFS/proc" || true
sudo umount "$ROOTFS/sys" || true

echo "Custom kernel compiled and installed."

echo "=== Step 4: Install Chromium and VS Code ==="
sudo chroot "$ROOTFS" bash -c "
export DEBIAN_FRONTEND=noninteractive
apt-get install -y chromium || apt-get install -y chromium-browser

wget -qO- https://packages.microsoft.com/keys/microsoft.asc | gpg --dearmor > /usr/share/keyrings/packages.microsoft.gpg
echo 'deb [arch=amd64 signed-by=/usr/share/keyrings/packages.microsoft.gpg] https://packages.microsoft.com/repos/code stable main' > /etc/apt/sources.list.d/vscode.list
apt-get update
apt-get install -y code
"

echo "=== Step 5: Install Snap and Software Center ==="
sudo chroot "$ROOTFS" bash -c "
export DEBIAN_FRONTEND=noninteractive
apt-get install -y snapd gnome-software gnome-software-plugin-snap
systemctl enable snapd.socket || true
"

echo "=== Step 6: Build BlazeNeuro Desktop Environment ==="
# Copy source code into chroot
sudo cp -r blazeneuro-de "$ROOTFS/tmp/"

# Build and install inside chroot
sudo chroot "$ROOTFS" bash -c "
cd /tmp/blazeneuro-de
make install
cd /
rm -rf /tmp/blazeneuro-de
"

echo "=== Step 7: Create Install Script ==="
sudo tee "$ROOTFS/usr/local/bin/install-blazeneuro" > /dev/null << 'INSTALL_SCRIPT'
#!/bin/bash
# BlazeNeuro Install to Disk
echo "========================================="
echo "  BlazeNeuro OS - Install to Disk"
echo "========================================="
echo ""

if [ "$(id -u)" -ne 0 ]; then
    echo "Please run as root: sudo install-blazeneuro"
    exit 1
fi

echo "Available disks:"
lsblk -d -o NAME,SIZE,MODEL | grep -v loop
echo ""
read -p "Enter target disk (e.g., sda): " TARGET_DISK

if [ -z "$TARGET_DISK" ]; then
    echo "No disk selected. Aborting."
    exit 1
fi

echo ""
echo "WARNING: This will ERASE ALL DATA on /dev/$TARGET_DISK!"
read -p "Are you sure? (yes/no): " CONFIRM

if [ "$CONFIRM" != "yes" ]; then
    echo "Aborted."
    exit 1
fi

echo "Partitioning /dev/$TARGET_DISK..."
parted -s /dev/$TARGET_DISK mklabel gpt
parted -s /dev/$TARGET_DISK mkpart primary fat32 1MiB 512MiB
parted -s /dev/$TARGET_DISK set 1 esp on
parted -s /dev/$TARGET_DISK mkpart primary ext4 512MiB 100%

echo "Formatting partitions..."
mkfs.vfat -F32 /dev/${TARGET_DISK}1
mkfs.ext4 -F /dev/${TARGET_DISK}2

echo "Installing system..."
mount /dev/${TARGET_DISK}2 /mnt
mkdir -p /mnt/boot/efi
mount /dev/${TARGET_DISK}1 /mnt/boot/efi

# Copy live filesystem
cp -ax / /mnt/ 2>/dev/null || rsync -aAXv / /mnt/ --exclude={"/mnt","/proc","/sys","/dev","/run","/tmp","/live","/cdrom"}

# Configure fstab
UUID_ROOT=$(blkid -s UUID -o value /dev/${TARGET_DISK}2)
UUID_EFI=$(blkid -s UUID -o value /dev/${TARGET_DISK}1)
cat > /mnt/etc/fstab << FSTAB
UUID=$UUID_ROOT  /          ext4  errors=remount-ro  0  1
UUID=$UUID_EFI   /boot/efi  vfat  umask=0077         0  1
FSTAB

# Install GRUB
mount --bind /dev /mnt/dev
mount --bind /proc /mnt/proc
mount --bind /sys /mnt/sys
chroot /mnt grub-install /dev/$TARGET_DISK
chroot /mnt update-grub
umount /mnt/dev /mnt/proc /mnt/sys

echo ""
echo "Installation complete! You can reboot now."
echo "Remove the installation media before rebooting."
INSTALL_SCRIPT
sudo chmod +x "$ROOTFS/usr/local/bin/install-blazeneuro"

echo "=== Step 8: Create user account ==="
sudo chroot "$ROOTFS" bash -c "
useradd -m -s /bin/bash -G sudo,adm,cdrom,audio,video,plugdev user
echo 'user:blazeneuro' | chpasswd
echo 'root:root' | chpasswd
# Allow user to use sudo without password
echo 'user ALL=(ALL) NOPASSWD:ALL' > /etc/sudoers.d/user
"

echo "=== Step 9: Configure login screen ==="
sudo mkdir -p "$ROOTFS/etc/lightdm/lightdm.conf.d"
sudo tee "$ROOTFS/etc/lightdm/lightdm.conf.d/50-blazeneuro.conf" > /dev/null << EOF
[Seat:*]
autologin-user=user
autologin-user-timeout=0
user-session=blazeneuro
greeter-session=lightdm-gtk-greeter
greeter-hide-users=false
EOF

# Brand the LightDM greeter
sudo tee "$ROOTFS/etc/lightdm/lightdm-gtk-greeter.conf" > /dev/null << 'GREETER'
[greeter]
theme-name = Adwaita-dark
icon-theme-name = Papirus-Dark
font-name = Inter 11
background = #09090b
GREETER

sudo chroot "$ROOTFS" systemctl set-default graphical.target

echo "=== Step 10: Configure networking ==="
sudo chroot "$ROOTFS" systemctl enable NetworkManager

echo "=== Step 11: Set up user home ==="
sudo mkdir -p "$ROOTFS/home/user/Documents"
sudo mkdir -p "$ROOTFS/home/user/Downloads"
sudo mkdir -p "$ROOTFS/home/user/Pictures"
sudo mkdir -p "$ROOTFS/home/user/Music"
sudo mkdir -p "$ROOTFS/home/user/Videos"

# Set default wallpaper
if [ -f "blazeneuro-de/assets/wallpaper.png" ]; then
    echo "$ROOTFS/usr/local/share/blazeneuro/assets/wallpaper.png" > "$ROOTFS/home/user/.blazeneuro-wallpaper"
fi

sudo chroot "$ROOTFS" chown -R user:user /home/user

echo "=== Step 12: Clean up chroot ==="
sudo chroot "$ROOTFS" apt-get clean
sudo chroot "$ROOTFS" rm -rf /var/cache/apt/archives/* /tmp/* /var/tmp/*

sudo umount "$ROOTFS/dev/pts" || true
sudo umount "$ROOTFS/dev" || true
sudo umount "$ROOTFS/proc" || true
sudo umount "$ROOTFS/sys" || true

echo "=== Step 13: Build live ISO ==="
mkdir -p "$ISOROOT"/{boot/grub,live}



# Install live-boot
sudo mount --bind /dev "$ROOTFS/dev"
sudo mount --bind /dev/pts "$ROOTFS/dev/pts"
sudo mount -t proc proc "$ROOTFS/proc"
sudo mount -t sysfs sysfs "$ROOTFS/sys"

sudo chroot "$ROOTFS" bash -c "
export DEBIAN_FRONTEND=noninteractive
apt-get update
apt-get install -y live-boot
apt-get clean
"

sudo umount "$ROOTFS/dev/pts" || true
sudo umount "$ROOTFS/dev" || true
sudo umount "$ROOTFS/proc" || true
sudo umount "$ROOTFS/sys" || true

# Create squashfs
# Copy kernel and initrd (now that live-boot is installed)
VMLINUZ=$(ls "$ROOTFS"/boot/vmlinuz-* 2>/dev/null | head -1)
INITRD=$(ls "$ROOTFS"/boot/initrd.img-* 2>/dev/null | head -1)

if [ -z "$VMLINUZ" ] || [ -z "$INITRD" ]; then
    echo "ERROR: Kernel or initrd not found!"
    ls -la "$ROOTFS/boot/"
    exit 1
fi

cp "$VMLINUZ" "$ISOROOT/boot/vmlinuz"
cp "$INITRD" "$ISOROOT/boot/initrd.img"

sudo mksquashfs "$ROOTFS" "$ISOROOT/live/filesystem.squashfs" -comp xz -e boot

# GRUB config
cat > "$ISOROOT/boot/grub/grub.cfg" << 'GRUBCFG'
set timeout=5
set default=0

menuentry "BlazeNeuro Desktop" {
    linux /boot/vmlinuz boot=live toram quiet splash
    initrd /boot/initrd.img
}

menuentry "BlazeNeuro Desktop (Safe Mode)" {
    linux /boot/vmlinuz boot=live toram nomodeset quiet
    initrd /boot/initrd.img
}

menuentry "BlazeNeuro Desktop (Persistent)" {
    linux /boot/vmlinuz boot=live toram persistence quiet splash
    initrd /boot/initrd.img
}
GRUBCFG

echo "=== Step 14: Generate ISO ==="
grub-mkrescue -o blazeneuro.iso "$ISOROOT"

echo "=== Build complete! ==="
ls -lh blazeneuro.iso
echo ""
echo "Default login: user / user"
echo "To install to disk, run: sudo install-blazeneuro"
