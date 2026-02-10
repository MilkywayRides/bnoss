#!/bin/bash
# BlazeNeuro Desktop Session Startup Script

SHARE_DIR="/usr/local/share/blazeneuro"

# ── Apply GTK Theme ─────────────────────────────────────
export GTK_THEME="Adwaita:dark"
mkdir -p "$HOME/.config/gtk-3.0"
if [ -f "$SHARE_DIR/blazeneuro.css" ]; then
    cp "$SHARE_DIR/blazeneuro.css" "$HOME/.config/gtk-3.0/gtk.css"
fi

# ── Set wallpaper ───────────────────────────────────────
if [ -f "$HOME/.blazeneuro-wallpaper" ]; then
    WP=$(cat "$HOME/.blazeneuro-wallpaper")
    feh --bg-fill "$WP" &
elif [ -f "$SHARE_DIR/assets/wallpaper.png" ]; then
    feh --bg-fill "$SHARE_DIR/assets/wallpaper.png" &
else
    # Solid dark background
    xsetroot -solid "#0a0a0f"
fi

# ── Start compositor ───────────────────────────────────
picom --config "$SHARE_DIR/picom.conf" -b 2>/dev/null || picom -b &

# ── Start desktop components ───────────────────────────
blazeneuro-topbar &
sleep 0.3
blazeneuro-dock &
sleep 0.3

# ── D-Bus session ──────────────────────────────────────
if [ -z "$DBUS_SESSION_BUS_ADDRESS" ]; then
    eval $(dbus-launch --sh-syntax)
fi

# ── Network Manager applet ─────────────────────────────
nm-applet &

# ── Start Window Manager (must be last, blocks) ───────
exec blazeneuro-wm
