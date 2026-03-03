#!/bin/bash
# BlazeNeuro Desktop Session Startup Script
# Launched by LightDM via /usr/share/xsessions/blazeneuro.desktop

SHARE_DIR="/usr/local/share/blazeneuro"
LOG_DIR="$HOME/.local/share/blazeneuro"
mkdir -p "$LOG_DIR"
SESSION_LOG="$LOG_DIR/session.log"

log() {
    echo "[$(date '+%H:%M:%S')] $1" | tee -a "$SESSION_LOG"
}

log "=== BlazeNeuro session starting ==="

# ── D-Bus session (MUST be first — GTK apps depend on it) ──
if [ -z "$DBUS_SESSION_BUS_ADDRESS" ]; then
    eval $(dbus-launch --sh-syntax --exit-with-session)
    export DBUS_SESSION_BUS_ADDRESS
    log "D-Bus launched: $DBUS_SESSION_BUS_ADDRESS"
fi

# ── Detect virtual environment ─────────────────────────
IS_VIRTUAL=0
VIRT_TYPE="none"
if command -v systemd-detect-virt >/dev/null 2>&1; then
    VIRT_TYPE=$(systemd-detect-virt 2>/dev/null || echo "none")
    if [ "$VIRT_TYPE" != "none" ]; then
        IS_VIRTUAL=1
        log "Running in virtual environment: $VIRT_TYPE"
    fi
elif grep -qiE 'qemu|virtualbox|vmware|kvm|bochs|xen' /sys/class/dmi/id/product_name 2>/dev/null; then
    IS_VIRTUAL=1
    VIRT_TYPE="detected-dmi"
    log "Running in virtual environment (detected via DMI)"
fi

# ── XDG / Environment ──────────────────────────────────
export GTK_THEME="Adwaita:dark"
export XDG_CURRENT_DESKTOP="BlazeNeuro"
export XDG_SESSION_DESKTOP="blazeneuro"
export XDG_SESSION_TYPE="x11"

# ── Apply GTK Theme ─────────────────────────────────────
mkdir -p "$HOME/.config/gtk-3.0"
if [ -f "$SHARE_DIR/blazeneuro.css" ]; then
    cp "$SHARE_DIR/blazeneuro.css" "$HOME/.config/gtk-3.0/gtk.css"
fi
log "GTK theme applied"

# ── Disable screen saver / DPMS to prevent color issues ─
xset s off 2>/dev/null
xset -dpms 2>/dev/null
xset s noblank 2>/dev/null

# ── Force proper color depth and refresh ───────────────
xrandr --output $(xrandr | grep " connected" | head -1 | awk '{print $1}') --mode $(xrandr | grep -A1 " connected" | tail -1 | awk '{print $1}') 2>/dev/null || true

# ── Set wallpaper ───────────────────────────────────────
if [ -f "$HOME/.blazeneuro-wallpaper" ]; then
    WP=$(cat "$HOME/.blazeneuro-wallpaper")
    if [ -f "$WP" ]; then
        feh --bg-fill "$WP" 2>/dev/null &
    else
        xsetroot -solid "#09090b"
    fi
elif [ -f "$SHARE_DIR/assets/wallpaper.png" ]; then
    feh --bg-fill "$SHARE_DIR/assets/wallpaper.png" 2>/dev/null &
else
    # Solid dark background as safe fallback
    xsetroot -solid "#09090b"
fi
log "Wallpaper set"

# ── Start compositor (with better fallback) ────────────
start_compositor() {
    if [ "$IS_VIRTUAL" -eq 1 ]; then
        # In VMs, disable compositor to prevent color corruption and crashes
        log "VM detected — skipping compositor to prevent display issues"
        return 0
    elif [ -f "$SHARE_DIR/picom.conf" ]; then
        log "Using full compositor config"
        picom --config "$SHARE_DIR/picom.conf" -b 2>>"$SESSION_LOG" &
    else
        picom -b 2>>"$SESSION_LOG" &
    fi

    sleep 0.5

    # Verify compositor is running
    if ! pgrep -x picom >/dev/null 2>&1; then
        log "WARNING: Compositor failed to start, continuing without compositing"
        return 1
    fi
    log "Compositor started successfully"
    return 0
}

start_compositor || true

# ── Start desktop components (parallelized) ────────────
log "Starting desktop components..."

# Ensure X server is ready
sleep 1

blazeneuro-desktop 2>>"$SESSION_LOG" &
blazeneuro-topbar 2>>"$SESSION_LOG" &
sleep 0.15

blazeneuro-dock 2>>"$SESSION_LOG" &
sleep 0.1

log "Desktop, topbar, dock started"

# ── Network Manager applet ─────────────────────────────
nm-applet 2>/dev/null &

log "All components launched, starting window manager..."

# ── Start Window Manager (must be last, blocks) ───────
exec blazeneuro-wm 2>>"$SESSION_LOG"
