/*
 * BlazeNeuro Top Bar
 * macOS-like menu bar at the top of the screen.
 * Shows system name, clock, and status indicators.
 * Written in C with GTK3.
 */

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <X11/Xatom.h>
#include <time.h>
#include <string.h>
#include <stdio.h>

#include "../common/theme.h"

#define BAR_HEIGHT 32

/* ── Clock Update ───────────────────────────────────────── */
static gboolean update_clock(gpointer data) {
    GtkWidget *label = GTK_WIDGET(data);
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    if (!t) return TRUE;
    char buf[64];
    strftime(buf, sizeof(buf), "%a %b %d  %I:%M %p", t);
    gtk_label_set_text(GTK_LABEL(label), buf);
    return TRUE;
}

/* ── Set Struts ─────────────────────────────────────────── */
static void on_realize(GtkWidget *widget, gpointer data) {
    (void)data;
    GdkWindow *gdk_win = gtk_widget_get_window(widget);
    if (!gdk_win) return;
    GdkDisplay *display = gdk_window_get_display(gdk_win);
    Display *xdpy = gdk_x11_display_get_xdisplay(display);
    Window xwin = gdk_x11_window_get_xid(gdk_win);

    GdkMonitor *mon = gdk_display_get_primary_monitor(display);
    if (!mon) return;
    GdkRectangle geom;
    gdk_monitor_get_geometry(mon, &geom);

    long struts[12] = {0};
    struts[2] = BAR_HEIGHT; /* top */
    struts[8] = 0;          /* top_start_x */
    struts[9] = geom.width; /* top_end_x */

    Atom strut_partial = XInternAtom(xdpy, "_NET_WM_STRUT_PARTIAL", False);
    XChangeProperty(xdpy, xwin, strut_partial, XA_CARDINAL, 32,
                    PropModeReplace, (unsigned char *)struts, 12);
}

/* ── Main ───────────────────────────────────────────────── */
int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    blazeneuro_load_theme();

    GtkWidget *win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(win), "BlazeNeuro Bar");
    gtk_window_set_type_hint(GTK_WINDOW(win), GDK_WINDOW_TYPE_HINT_DOCK);
    gtk_window_set_decorated(GTK_WINDOW(win), FALSE);
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(win), TRUE);
    gtk_window_set_skip_pager_hint(GTK_WINDOW(win), TRUE);
    gtk_widget_set_app_paintable(win, TRUE);

    GdkScreen *scr = gtk_widget_get_screen(win);
    GdkVisual *vis = gdk_screen_get_rgba_visual(scr);
    if (vis) gtk_widget_set_visual(win, vis);

    gtk_style_context_add_class(gtk_widget_get_style_context(win), "topbar");

    g_signal_connect(win, "realize", G_CALLBACK(on_realize), NULL);
    g_signal_connect(win, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    /* Layout: [Logo/Title] ---- [Status] [Clock] */
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_container_add(GTK_CONTAINER(win), hbox);

    /* Left: BlazeNeuro title */
    GtkWidget *title = gtk_label_new("  BlazeNeuro");
    gtk_style_context_add_class(gtk_widget_get_style_context(title), "topbar-title");
    gtk_widget_set_margin_start(title, 12);
    gtk_box_pack_start(GTK_BOX(hbox), title, FALSE, FALSE, 0);

    /* Center spacer */
    GtkWidget *spacer = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(hbox), spacer, TRUE, TRUE, 0);

    /* Right: Clock */
    GtkWidget *clock_label = gtk_label_new("");
    gtk_style_context_add_class(gtk_widget_get_style_context(clock_label), "topbar-clock");
    gtk_widget_set_margin_end(clock_label, 16);
    gtk_box_pack_end(GTK_BOX(hbox), clock_label, FALSE, FALSE, 0);

    update_clock(clock_label);
    g_timeout_add_seconds(1, update_clock, clock_label);

    /* Position at top */
    GdkMonitor *mon = gdk_display_get_primary_monitor(gdk_display_get_default());
    if (!mon) {
        fprintf(stderr, "BlazeNeuro Bar: No primary monitor found\n");
        return 1;
    }
    GdkRectangle geom;
    gdk_monitor_get_geometry(mon, &geom);

    gtk_window_set_default_size(GTK_WINDOW(win), geom.width, BAR_HEIGHT);
    gtk_window_move(GTK_WINDOW(win), geom.x, geom.y);

    gtk_widget_show_all(win);
    gtk_main();

    return 0;
}
