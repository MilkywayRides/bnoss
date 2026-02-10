#!/bin/bash
set -e

# BlazeNeuro Desktop OS Build Script
# Creates a fully functional Ubuntu-based desktop ISO with XFCE4, software installation, and disk installer

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

# Full apt sources with universe/multiverse for maximum package availability
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
    neofetch
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
    xfce4-taskmanager \
    xfce4-screenshooter \
    xfce4-power-manager \
    xfce4-notifyd \
    thunar \
    thunar-archive-plugin \
    mousepad \
    ristretto \
    lightdm \
    lightdm-gtk-greeter \
    lightdm-gtk-greeter-settings \
    xorg \
    xserver-xorg \
    xinit \
    dbus-x11 \
    adwaita-icon-theme \
    papirus-icon-theme \
    gnome-themes-extra \
    arc-theme \
    fonts-dejavu-core \
    fonts-liberation \
    fonts-noto-color-emoji \
    pulseaudio \
    pavucontrol \
    gvfs \
    gvfs-backends \
    file-roller \
    evince \
    gnome-calculator \
    gnome-disk-utility \
    synaptic
"

echo "=== Step 5: Install Chromium browser ==="
sudo chroot "$ROOTFS" bash -c "
export DEBIAN_FRONTEND=noninteractive
apt-get install -y chromium-browser || apt-get install -y chromium
"

echo "=== Step 6: Install VS Code ==="
sudo chroot "$ROOTFS" bash -c "
export DEBIAN_FRONTEND=noninteractive
wget -qO- https://packages.microsoft.com/keys/microsoft.asc | gpg --dearmor > /usr/share/keyrings/packages.microsoft.gpg
echo 'deb [arch=amd64 signed-by=/usr/share/keyrings/packages.microsoft.gpg] https://packages.microsoft.com/repos/code stable main' > /etc/apt/sources.list.d/vscode.list
apt-get update
apt-get install -y code
"

echo "=== Step 7: Install Snap support ==="
sudo chroot "$ROOTFS" bash -c "
export DEBIAN_FRONTEND=noninteractive
apt-get install -y snapd
systemctl enable snapd.socket || true
"

echo "=== Step 8: Install GNOME Software Center ==="
sudo chroot "$ROOTFS" bash -c "
export DEBIAN_FRONTEND=noninteractive
apt-get install -y gnome-software gnome-software-plugin-snap
"

echo "=== Step 9: Install Calamares installer ==="
sudo chroot "$ROOTFS" bash -c "
export DEBIAN_FRONTEND=noninteractive
apt-get install -y calamares calamares-settings-debian 2>/dev/null || {
    echo 'Calamares not in repos, installing ubiquity instead...'
    apt-get install -y ubiquity ubiquity-frontend-gtk ubiquity-slideshow-ubuntu 2>/dev/null || {
        echo 'Creating manual install script instead...'
    }
}
"

# Create a simple install-to-disk script as fallback
sudo tee "$ROOTFS/usr/local/bin/install-blazeneuro" > /dev/null << 'INSTALL_SCRIPT'
#!/bin/bash
# BlazeNeuro Install to Disk
echo "========================================="
echo "  BlazeNeuro OS - Install to Disk"
echo "========================================="
echo ""

if [ "$(id -u)" -ne 0 ]; then
    echo "Please run as root: sudo install-bnoss"
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

echo "=== Step 10: Create user account ==="
sudo chroot "$ROOTFS" bash -c "
useradd -m -s /bin/bash -G sudo,adm,cdrom,audio,video,plugdev user
echo 'user:user' | chpasswd
echo 'root:root' | chpasswd
# Allow user to use sudo without password
echo 'user ALL=(ALL) NOPASSWD:ALL' > /etc/sudoers.d/user
"

echo "=== Step 11: Configure auto-login ==="
sudo mkdir -p "$ROOTFS/etc/lightdm/lightdm.conf.d"
sudo tee "$ROOTFS/etc/lightdm/lightdm.conf.d/50-autologin.conf" > /dev/null << EOF
[Seat:*]
autologin-user=user
autologin-user-timeout=0
user-session=xfce
EOF

sudo chroot "$ROOTFS" systemctl set-default graphical.target

echo "=== Step 12: Configure networking ==="
sudo chroot "$ROOTFS" systemctl enable NetworkManager

echo "=== Step 13: Set up desktop ==="
sudo mkdir -p "$ROOTFS/home/user/Desktop"
sudo mkdir -p "$ROOTFS/home/user/Documents"
sudo mkdir -p "$ROOTFS/home/user/Downloads"
sudo mkdir -p "$ROOTFS/home/user/Pictures"
sudo mkdir -p "$ROOTFS/home/user/Music"
sudo mkdir -p "$ROOTFS/home/user/Videos"

# Desktop shortcut: Terminal
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

# Desktop shortcut: Chromium
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

# Desktop shortcut: VS Code
sudo tee "$ROOTFS/home/user/Desktop/code.desktop" > /dev/null << EOF
[Desktop Entry]
Version=1.0
Type=Application
Name=Visual Studio Code
Exec=code --no-sandbox --unity-launch
Icon=com.visualstudio.code
Terminal=false
Categories=Development;IDE;
EOF

# Desktop shortcut: Files
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

# Desktop shortcut: Software Center
sudo tee "$ROOTFS/home/user/Desktop/software.desktop" > /dev/null << EOF
[Desktop Entry]
Version=1.0
Type=Application
Name=Software Center
Exec=gnome-software
Icon=org.gnome.Software
Terminal=false
Categories=System;
EOF

# Desktop shortcut: Install to Disk
sudo tee "$ROOTFS/home/user/Desktop/install-bnoss.desktop" > /dev/null << EOF
[Desktop Entry]
Version=1.0
Type=Application
Name=Install BlazeNeuro to Disk
Exec=xfce4-terminal -e "sudo install-blazeneuro"
Icon=drive-harddisk
Terminal=false
Categories=System;
EOF

sudo chmod +x "$ROOTFS/home/user/Desktop/"*.desktop
sudo chroot "$ROOTFS" chown -R user:user /home/user

# Apply Arc-Dark theme and Papirus icons
sudo mkdir -p "$ROOTFS/home/user/.config/xfce4/xfconf/xfce-perchannel-xml"
sudo tee "$ROOTFS/home/user/.config/xfce4/xfconf/xfce-perchannel-xml/xsettings.xml" > /dev/null << 'XSETTINGS'
<?xml version="1.0" encoding="UTF-8"?>
<channel name="xsettings" version="1.0">
  <property name="Net" type="empty">
    <property name="ThemeName" type="string" value="Arc-Dark"/>
    <property name="IconThemeName" type="string" value="Papirus-Dark"/>
  </property>
</channel>
XSETTINGS
sudo chroot "$ROOTFS" chown -R user:user /home/user/.config

echo "=== Step 14: Clean up chroot ==="
sudo chroot "$ROOTFS" apt-get clean
sudo chroot "$ROOTFS" rm -rf /var/cache/apt/archives/* /tmp/* /var/tmp/*

sudo umount "$ROOTFS/dev/pts" || true
sudo umount "$ROOTFS/dev" || true
sudo umount "$ROOTFS/proc" || true
sudo umount "$ROOTFS/sys" || true

echo "=== Step 15: Build live ISO ==="
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

echo "=== Step 16: Generate ISO ==="
grub-mkrescue -o blazeneuro.iso "$ISOROOT"

echo "=== Build complete! ==="
ls -lh blazeneuro.iso
echo ""
echo "Default login: user / user"
echo "To install to disk, run: sudo install-blazeneuro"
