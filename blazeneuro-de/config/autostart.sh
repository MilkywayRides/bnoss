#!/bin/bash
# BlazeNeuro Desktop Session Startup Script
# Launched by LightDM via /usr/share/xsessions/blazeneuro.desktop

SHARE_DIR="/usr/local/share/blazeneuro"

# ── D-Bus session (MUST be first — GTK apps depend on it) ──
if [ -z "$DBUS_SESSION_BUS_ADDRESS" ]; then
    eval $(dbus-launch --sh-syntax --exit-with-session)
    export DBUS_SESSION_BUS_ADDRESS
fi

# ── Apply GTK Theme ─────────────────────────────────────
export GTK_THEME="Adwaita:dark"
export XDG_CURRENT_DESKTOP="BlazeNeuro"
export XDG_SESSION_DESKTOP="blazeneuro"
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
    xsetroot -solid "#09090b"
fi

# ── Start compositor ───────────────────────────────────
if [ -f "$SHARE_DIR/picom.conf" ]; then
    picom --config "$SHARE_DIR/picom.conf" -b 2>/dev/null &
else
    picom -b 2>/dev/null &
fi

# Wait for compositor to initialize
sleep 0.5

# ── Start desktop components ───────────────────────────
blazeneuro-desktop &
sleep 0.3
blazeneuro-topbar &
sleep 0.3
blazeneuro-dock &
sleep 0.3

# ── Network Manager applet ─────────────────────────────
nm-applet 2>/dev/null &

# ── Start Window Manager (must be last, blocks) ───────
exec blazeneuro-wm
