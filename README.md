# BlazeNeuro — Desktop Linux OS

A fully functional Ubuntu-based desktop Linux distro built via GitHub Actions.

## What's Included

| Category | Software |
|---|---|
| Desktop | XFCE4 with Arc-Dark theme & Papirus icons |
| Browser | Chromium |
| Code Editor | Visual Studio Code |
| File Manager | Thunar |
| Terminal | xfce4-terminal |
| Software Center | GNOME Software (install apps via GUI) |
| Package Managers | apt, snap |
| Dev Tools | git, python3, build-essential |
| Utilities | htop, neofetch, calculator, screenshot, task manager |

## Download

Go to [Actions](../../actions) → click the latest successful run → download **blazeneuro-desktop-iso**.

## Boot

```bash
# With QEMU (2GB+ RAM recommended)
qemu-system-x86_64 -cdrom blazeneuro.iso -m 2048 -enable-kvm

# Or burn to USB
sudo dd if=blazeneuro.iso of=/dev/sdX bs=4M status=progress
```

## Install to Disk

Once booted, click **"Install BlazeNeuro to Disk"** on the desktop, or run:
```bash
sudo install-blazeneuro
```

## Installing Software

```bash
# Via apt
sudo apt install firefox

# Via snap
sudo snap install spotify

# Via GUI
# Open "Software Center" from the desktop
```

## Default Credentials

- **User**: `user` / `user` (sudo access)
- **Root**: `root` / `root`
