# BlazeNeuro — Desktop Linux OS

A fully functional Debian-based desktop Linux distro with a custom desktop environment, built via GitHub Actions.

## What's Included

| Category | Software |
|---|---|
| Desktop | BlazeNeuro DE (custom GTK shell with window manager, desktop, dock, topbar, launcher) |
| Browser | Chromium |
| Code Editor | Visual Studio Code |
| File Manager | BlazeNeuro Files |
| Terminal | BlazeNeuro Terminal |
| Package Managers | apt, flatpak |
| Dev Tools | git, python3, build-essential |
| Utilities | htop, neofetch, BlazeNeuro Calculator, BlazeNeuro Notes, Task Viewer, gnome-calculator, gnome-system-monitor |

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

# Via flatpak
flatpak install flathub org.mozilla.firefox

# Via AppImage
chmod +x SomeApp.AppImage && ./SomeApp.AppImage
```

## Linux App Compatibility

BlazeNeuro is Debian-based, so it can run most Debian/Ubuntu GUI/CLI apps directly with `apt`.

It also includes additional compatibility tooling:
- **Flatpak** for cross-distro desktop apps
- **FUSE/AppImage support** via `fuse3` and `libfuse2`

> Note: Kernel modules/drivers and apps requiring a specific host distro init stack may still need manual setup.

## Building from Source

```bash
# Build the desktop environment locally
cd blazeneuro-de && make clean && make

# Build the full ISO (requires root, debootstrap, squashfs-tools, etc.)
sudo ./build-iso.sh
```

## Default Credentials

- **User**: `user` / `blazeneuro` (sudo access)
- **Root**: `root` / `root`

You can change your password after login with:
```bash
passwd
```

## Keyboard Shortcuts

| Shortcut | Action |
|---|---|
| `Alt+Enter` | Open terminal |
| `Alt+Space` | Open launcher |
| `Alt+Tab` | Cycle windows |
| `Alt+F4` | Close window |
| `Alt+Click` | Move window |
| `Alt+Right-Click` | Resize window |
