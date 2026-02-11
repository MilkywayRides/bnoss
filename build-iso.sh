#!/bin/bash
set -e

# BlazeNeuro Desktop OS Build Script
# Creates a fully functional Ubuntu-based desktop ISO with custom C-based DE

ROOTFS="./rootfs"
ISOROOT="./isoroot"
DISTRO="jammy"  # Ubuntu 22.04

echo "=== Step 1: Bootstrap Ubuntu base system ==="
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
    wine64

# distrobox is unavailable in some Ubuntu releases/repositories
if apt-cache show distrobox >/dev/null 2>&1; then
    apt-get install -y distrobox
else
    echo "[INFO] distrobox package not found for $DISTRO; skipping distrobox installation"
fi
"

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
sudo tee "$ROOTFS/etc/lightdm/lightdm.conf.d/50-login.conf" > /dev/null << EOF
[Seat:*]
user-session=blazeneuro
greeter-hide-users=false
greeter-show-manual-login=true
EOF

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

VMLINUZ=$(ls "$ROOTFS"/boot/vmlinuz-* 2>/dev/null | head -1)
INITRD=$(ls "$ROOTFS"/boot/initrd.img-* 2>/dev/null | head -1)

if [ -z "$VMLINUZ" ] || [ -z "$INITRD" ]; then
    echo "ERROR: Kernel or initrd not found!"
    ls -la "$ROOTFS/boot/"
    exit 1
fi

cp "$VMLINUZ" "$ISOROOT/boot/vmlinuz"
cp "$INITRD" "$ISOROOT/boot/initrd.img"

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
