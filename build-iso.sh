#!/bin/bash
set -euo pipefail

# ╔══════════════════════════════════════════════════════════╗
# ║  BlazeNeuro OS — Build Script                           ║
# ║  Debian Bookworm base with BlazeNeuro Desktop Environment║
# ╚══════════════════════════════════════════════════════════╝

# ── Configuration ──────────────────────────────────────────
OS_NAME="BlazeNeuro"
OS_VERSION="1.0"
OS_CODENAME="nova"
ROOTFS="$(pwd)/rootfs"
ISOROOT="$(pwd)/isoroot"
DISTRO="bookworm"
MIRROR="http://deb.debian.org/debian"
ISO_OUTPUT="blazeneuro.iso"
# ───────────────────────────────────────────────────────────

CHROOT_MOUNTED=0

# ── Helper Functions ───────────────────────────────────────
mount_chroot() {
    if [ "$CHROOT_MOUNTED" -eq 1 ]; then return; fi
    echo "  → Mounting chroot filesystems..."
    sudo mount --bind /dev "$ROOTFS/dev"
    sudo mount --bind /dev/pts "$ROOTFS/dev/pts"
    sudo mount -t proc proc "$ROOTFS/proc"
    sudo mount -t sysfs sysfs "$ROOTFS/sys"
    CHROOT_MOUNTED=1
}

umount_chroot() {
    if [ "$CHROOT_MOUNTED" -eq 0 ]; then return; fi
    echo "  → Unmounting chroot filesystems..."
    sudo umount "$ROOTFS/dev/pts" 2>/dev/null || true
    sudo umount "$ROOTFS/dev" 2>/dev/null || true
    sudo umount "$ROOTFS/proc" 2>/dev/null || true
    sudo umount "$ROOTFS/sys" 2>/dev/null || true
    CHROOT_MOUNTED=0
}

cleanup() {
    echo ""
    echo "=== Cleanup ==="
    umount_chroot
}

trap cleanup EXIT

step_time() {
    date +%s
}

print_step() {
    local step_num="$1"
    local step_name="$2"
    echo ""
    echo "══════════════════════════════════════════════════════"
    echo "  Step $step_num: $step_name"
    echo "══════════════════════════════════════════════════════"
}

run_chroot() {
    sudo chroot "$ROOTFS" bash -c "
export DEBIAN_FRONTEND=noninteractive
export LC_ALL=C
$1
"
}

# ── Step 1: Bootstrap Debian base system ───────────────────
START=$(step_time)
print_step 1 "Bootstrap Debian $DISTRO base system"

sudo debootstrap --arch=amd64 "$DISTRO" "$ROOTFS" "$MIRROR"

echo "  ✓ Base system bootstrapped in $(( $(step_time) - START ))s"

# ── Step 2: Configure chroot ──────────────────────────────
print_step 2 "Configure chroot environment"

# APT sources with main + contrib + non-free (for firmware)
sudo tee "$ROOTFS/etc/apt/sources.list" > /dev/null << EOF
deb $MIRROR $DISTRO main contrib non-free non-free-firmware
deb $MIRROR $DISTRO-updates main contrib non-free non-free-firmware
deb http://security.debian.org/debian-security $DISTRO-security main contrib non-free non-free-firmware
EOF

echo "blazeneuro" | sudo tee "$ROOTFS/etc/hostname" > /dev/null

sudo tee "$ROOTFS/etc/hosts" > /dev/null << EOF
127.0.0.1   localhost
127.0.1.1   blazeneuro
EOF

mount_chroot

run_chroot "
apt-get update
apt-get install -y locales
sed -i 's/# en_US.UTF-8/en_US.UTF-8/' /etc/locale.gen
locale-gen
update-locale LANG=en_US.UTF-8
"

echo "  ✓ Chroot configured"

# ── Step 3: Brand as BlazeNeuro ───────────────────────────
print_step 3 "Brand as $OS_NAME"

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

sudo rm -f "$ROOTFS/etc/legal" 2>/dev/null || true

echo "  ✓ Branding applied"

# ── Step 4: Install core system packages ──────────────────
print_step 4 "Install core system packages"
START=$(step_time)

run_chroot "
apt-get install -y \
    linux-image-amd64 \
    firmware-linux-free \
    live-boot \
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
    apt-utils \
    software-properties-common \
    gnupg \
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
    gnome-calculator \
    gnome-system-monitor \
    evince \
    mousepad \
    gnome-disk-utility \
    flatpak \
    fuse3 \
    libfuse2 \
    \
    mesa-utils \
    mesa-vulkan-drivers \
    libgl1-mesa-dri \
    xserver-xorg-video-all \
    vainfo \
    \
    firmware-linux-free \
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
    system-config-printer \
    \
    ntfs-3g \
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
    inxi \
    \
    grub-pc-bin \
    grub-efi-amd64-bin \
    grub-common
"

echo "  ✓ Core packages installed in $(( $(step_time) - START ))s"

# ── Step 5: Install Chromium and VS Code ──────────────────
print_step 5 "Install Chromium and VS Code"
START=$(step_time)

run_chroot "
# Chromium from Debian repos
apt-get install -y chromium || apt-get install -y chromium-browser || echo '[INFO] Chromium not available, skipping'

# VS Code from Microsoft repo
wget -qO- https://packages.microsoft.com/keys/microsoft.asc | gpg --dearmor > /usr/share/keyrings/packages.microsoft.gpg
echo 'deb [arch=amd64 signed-by=/usr/share/keyrings/packages.microsoft.gpg] https://packages.microsoft.com/repos/code stable main' > /etc/apt/sources.list.d/vscode.list
apt-get update
apt-get install -y code || echo '[INFO] VS Code install failed, skipping'
"

echo "  ✓ Apps installed in $(( $(step_time) - START ))s"

# ── Step 6: Build BlazeNeuro Desktop Environment ──────────
print_step 6 "Build BlazeNeuro Desktop Environment"
START=$(step_time)

# Copy DE source code into chroot
sudo cp -r blazeneuro-de "$ROOTFS/tmp/"

run_chroot "
cd /tmp/blazeneuro-de
make clean || true
make install
cd /
rm -rf /tmp/blazeneuro-de
"

echo "  ✓ BlazeNeuro DE built and installed in $(( $(step_time) - START ))s"

# ── Step 7: Create install-to-disk script ─────────────────
print_step 7 "Create install-to-disk script"

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
rsync -aAXv / /mnt/ --exclude={"/mnt","/proc","/sys","/dev","/run","/tmp","/live","/cdrom"} 2>/dev/null || \
cp -ax / /mnt/ 2>/dev/null

# Configure fstab
UUID_ROOT=$(blkid -s UUID -o value /dev/${TARGET_DISK}2)
UUID_EFI=$(blkid -s UUID -o value /dev/${TARGET_DISK}1)
cat > /mnt/etc/fstab << FSTAB
UUID=$UUID_ROOT  /          ext4  errors=remount-ro  0  1
UUID=$UUID_EFI   /boot/efi  vfat  umask=0077         0  1
FSTAB

# Install GRUB (both BIOS and EFI)
mount --bind /dev /mnt/dev
mount --bind /proc /mnt/proc
mount --bind /sys /mnt/sys

chroot /mnt grub-install --target=x86_64-efi --efi-directory=/boot/efi --bootloader-id=blazeneuro --removable 2>/dev/null || true
chroot /mnt grub-install /dev/$TARGET_DISK 2>/dev/null || true
chroot /mnt update-grub

umount /mnt/dev /mnt/proc /mnt/sys
umount /mnt/boot/efi
umount /mnt

echo ""
echo "Installation complete! You can reboot now."
echo "Remove the installation media before rebooting."
INSTALL_SCRIPT
sudo chmod +x "$ROOTFS/usr/local/bin/install-blazeneuro"

# Also create desktop shortcut for installer
sudo tee "$ROOTFS/usr/share/applications/install-blazeneuro.desktop" > /dev/null << 'DSKTP'
[Desktop Entry]
Name=Install BlazeNeuro to Disk
Comment=Install BlazeNeuro OS to your hard drive
Exec=sudo blazeneuro-terminal -e "sudo install-blazeneuro"
Icon=system-installer
Terminal=false
Type=Application
Categories=System;
DSKTP

echo "  ✓ Install script created"

# ── Step 8: Create user account ───────────────────────────
print_step 8 "Create user account"

run_chroot "
useradd -m -s /bin/bash -G sudo,adm,cdrom,audio,video,plugdev,netdev user
echo 'user:blazeneuro' | chpasswd
echo 'root:root' | chpasswd
echo 'user ALL=(ALL) NOPASSWD:ALL' > /etc/sudoers.d/user
chmod 0440 /etc/sudoers.d/user
"

echo "  ✓ User account created (user/blazeneuro)"

# ── Step 9: Configure display manager ─────────────────────
print_step 9 "Configure LightDM"

sudo mkdir -p "$ROOTFS/etc/lightdm/lightdm.conf.d"
sudo tee "$ROOTFS/etc/lightdm/lightdm.conf.d/50-blazeneuro.conf" > /dev/null << EOF
[Seat:*]
autologin-user=user
autologin-user-timeout=0
user-session=blazeneuro
greeter-session=lightdm-gtk-greeter
greeter-hide-users=false
EOF

sudo tee "$ROOTFS/etc/lightdm/lightdm-gtk-greeter.conf" > /dev/null << 'GREETER'
[greeter]
theme-name = Adwaita-dark
icon-theme-name = Papirus-Dark
font-name = Inter 11
background = #09090b
GREETER

# Ensure autologin group exists and user is in it
run_chroot "
groupadd -f autologin
usermod -aG autologin user
systemctl set-default graphical.target
"

echo "  ✓ LightDM configured with auto-login"

# ── Step 10: Configure networking ─────────────────────────
print_step 10 "Configure networking"

run_chroot "
systemctl enable NetworkManager
"

echo "  ✓ NetworkManager enabled"

# ── Step 11: Set up user home ─────────────────────────────
print_step 11 "Set up user home directory"

sudo mkdir -p "$ROOTFS/home/user/"{Documents,Downloads,Pictures,Music,Videos,Desktop}

# Set default wallpaper path
if [ -f "blazeneuro-de/assets/wallpaper.png" ]; then
    echo "/usr/local/share/blazeneuro/assets/wallpaper.png" | sudo tee "$ROOTFS/home/user/.blazeneuro-wallpaper" > /dev/null
fi

# Copy installer shortcut to desktop
sudo cp "$ROOTFS/usr/share/applications/install-blazeneuro.desktop" "$ROOTFS/home/user/Desktop/" 2>/dev/null || true

sudo chroot "$ROOTFS" chown -R user:user /home/user

echo "  ✓ User home configured"

# ── Step 12: Clean up rootfs ──────────────────────────────
print_step 12 "Clean up rootfs"

run_chroot "
apt-get clean
rm -rf /var/cache/apt/archives/* /tmp/* /var/tmp/*
rm -rf /var/lib/apt/lists/*
"

umount_chroot

echo "  ✓ Rootfs cleaned"

# ── Step 13: Build live ISO ───────────────────────────────
print_step 13 "Build live ISO"
START=$(step_time)

mkdir -p "$ISOROOT"/{boot/grub,live}

# Find kernel and initrd
VMLINUZ=$(ls "$ROOTFS"/boot/vmlinuz-* 2>/dev/null | sort -V | tail -1)
INITRD=$(ls "$ROOTFS"/boot/initrd.img-* 2>/dev/null | sort -V | tail -1)

if [ -z "$VMLINUZ" ] || [ -z "$INITRD" ]; then
    echo "ERROR: Kernel or initrd not found!"
    echo "Contents of $ROOTFS/boot/:"
    ls -la "$ROOTFS/boot/"
    exit 1
fi

echo "  Using kernel: $(basename "$VMLINUZ")"
echo "  Using initrd: $(basename "$INITRD")"

cp "$VMLINUZ" "$ISOROOT/boot/vmlinuz"
cp "$INITRD" "$ISOROOT/boot/initrd.img"

# Create squashfs
echo "  Creating squashfs (this takes a while)..."
sudo mksquashfs "$ROOTFS" "$ISOROOT/live/filesystem.squashfs" \
    -comp xz -Xbcj x86 -b 1M -e boot

# GRUB config
cat > "$ISOROOT/boot/grub/grub.cfg" << 'GRUBCFG'
set timeout=5
set default=0

insmod all_video
insmod gfxterm
set gfxmode=auto
terminal_output gfxterm

set menu_color_normal=white/black
set menu_color_highlight=black/white

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

echo "  ✓ Live filesystem created in $(( $(step_time) - START ))s"

# ── Step 14: Generate ISO ─────────────────────────────────
print_step 14 "Generate ISO"
START=$(step_time)

grub-mkrescue -o "$ISO_OUTPUT" "$ISOROOT" \
    -- -volid "BLAZENEURO"

echo "  ✓ ISO generated in $(( $(step_time) - START ))s"

# ── Done ──────────────────────────────────────────────────
echo ""
echo "══════════════════════════════════════════════════════"
echo "  Build complete!"
echo "══════════════════════════════════════════════════════"
echo ""
ls -lh "$ISO_OUTPUT"
echo ""
echo "Default login: user / blazeneuro"
echo "Root password: root"
echo ""
echo "Boot with:"
echo "  qemu-system-x86_64 -cdrom $ISO_OUTPUT -m 2048 -enable-kvm"
echo ""
echo "Install to disk:"
echo "  sudo install-blazeneuro"
