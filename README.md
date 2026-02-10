# bnoss — Desktop Linux OS

A custom Ubuntu-based desktop Linux distro built via GitHub Actions.

## What's Included

- **Desktop**: XFCE4
- **Browser**: Chromium
- **File Manager**: Thunar
- **Terminal**: xfce4-terminal
- **Settings**: xfce4-settings
- **Init**: systemd
- **Networking**: NetworkManager

## Download

Go to [Actions](../../actions) → click the latest successful run → download the **bnoss-desktop-iso** artifact.

## Boot

```bash
# With QEMU
qemu-system-x86_64 -cdrom bnoss.iso -m 2048 -enable-kvm

# Or burn to USB
sudo dd if=bnoss.iso of=/dev/sdX bs=4M status=progress
```

## Default Credentials

- **User**: `user` / `user`
- **Root**: `root` / `root`
