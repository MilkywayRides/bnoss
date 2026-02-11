# BlazeNeuro — Desktop Linux OS

A fully functional Ubuntu-based desktop Linux distro built via GitHub Actions.

## What's Included

| Category | Software |
|---|---|
| Desktop | BlazeNeuro DE (custom GTK shell with desktop view, right-click context menu, and wallpaper customization) |
| Browser | Chromium |
| Code Editor | Visual Studio Code |
| File Manager | BlazeNeuro Files |
| Terminal | BlazeNeuro Terminal |
| Software Center | GNOME Software (install apps via GUI) |
| Package Managers | apt, snap, flatpak |
| Dev Tools | git, python3, build-essential |
| Utilities | htop, neofetch, BlazeNeuro Calculator, BlazeNeuro Notes, Task Viewer, screenshot, task manager |

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

## Linux App Compatibility

BlazeNeuro is Ubuntu-based, so it can run most Ubuntu GUI/CLI apps directly with `apt`.

It also includes additional compatibility tooling:
- **Snap** for sandboxed app packages
- **Flatpak** for cross-distro desktop apps
- **Distrobox + Podman** to run apps from other distros (for example Kali userland tools)
- **Wine** for many Windows executables
- **FUSE/AppImage support** via `fuse3` and `libfuse2`

> Note: Kernel modules/drivers and apps requiring a specific host distro init stack may still need manual setup.

## Default Credentials

- **User**: `user` / `blazeneuro` (sudo access)
- **Root**: `root` / `root`

You can change your password after login with:
```bash
passwd
```
