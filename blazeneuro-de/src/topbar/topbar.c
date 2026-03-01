/*
 * BlazeNeuro Top Bar
 * macOS-like menu bar at the top of the screen.
 * Shows system name, clock, and status indicators.
 * Written in C with GTK3.
 */

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

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

static Window get_active_window(Display *xdpy) {
    Atom net_active_window = XInternAtom(xdpy, "_NET_ACTIVE_WINDOW", False);
    Atom actual;
    int format;
    unsigned long n, left;
    unsigned char *data = NULL;

    if (XGetWindowProperty(xdpy, DefaultRootWindow(xdpy), net_active_window, 0, 1,
                           False, XA_WINDOW, &actual, &format, &n, &left, &data) != Success ||
        !data || n == 0) {
        if (data) XFree(data);
        return None;
    }

    Window w = *(Window *)data;
    XFree(data);
    return w;
}

static void send_client_message(Display *xdpy, Window target, Atom message_type,
                                long data0, long data1, long data2) {
    XEvent ev;
    memset(&ev, 0, sizeof(ev));
    ev.xclient.type = ClientMessage;
    ev.xclient.window = target;
    ev.xclient.message_type = message_type;
    ev.xclient.format = 32;
    ev.xclient.data.l[0] = data0;
    ev.xclient.data.l[1] = data1;
    ev.xclient.data.l[2] = data2;
    ev.xclient.data.l[3] = 1;

    XSendEvent(xdpy, DefaultRootWindow(xdpy), False,
               SubstructureRedirectMask | SubstructureNotifyMask, &ev);
    XFlush(xdpy);
}

static void close_active_window(void) {
    GdkDisplay *gdk_display = gdk_display_get_default();
    if (!gdk_display) return;

    Display *xdpy = gdk_x11_display_get_xdisplay(gdk_display);
    Window active = get_active_window(xdpy);
    if (active == None) return;

    Atom net_close_window = XInternAtom(xdpy, "_NET_CLOSE_WINDOW", False);
    send_client_message(xdpy, active, net_close_window, CurrentTime, 0, 0);
}

static void minimize_active_window(void) {
    GdkDisplay *gdk_display = gdk_display_get_default();
    if (!gdk_display) return;

    Display *xdpy = gdk_x11_display_get_xdisplay(gdk_display);
    Window active = get_active_window(xdpy);
    if (active == None) return;

    XIconifyWindow(xdpy, active, DefaultScreen(xdpy));
    XFlush(xdpy);
}

static void toggle_active_state(Atom atom1, Atom atom2) {
    GdkDisplay *gdk_display = gdk_display_get_default();
    if (!gdk_display) return;

    Display *xdpy = gdk_x11_display_get_xdisplay(gdk_display);
    Window active = get_active_window(xdpy);
    if (active == None) return;

    Atom net_wm_state = XInternAtom(xdpy, "_NET_WM_STATE", False);
    send_client_message(xdpy, active, net_wm_state, 2, atom1, atom2); /* toggle */
}

static void on_close_clicked(GtkButton *btn, gpointer data) {
    (void)btn; (void)data;
    close_active_window();
}

static void on_minimize_clicked(GtkButton *btn, gpointer data) {
    (void)btn; (void)data;
    minimize_active_window();
}

static void on_maximize_clicked(GtkButton *btn, gpointer data) {
    (void)btn; (void)data;
    GdkDisplay *gdk_display = gdk_display_get_default();
    if (!gdk_display) return;
    Display *xdpy = gdk_x11_display_get_xdisplay(gdk_display);
    Atom horz = XInternAtom(xdpy, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
    Atom vert = XInternAtom(xdpy, "_NET_WM_STATE_MAXIMIZED_VERT", False);
    toggle_active_state(horz, vert);
}

static void on_fullscreen_clicked(GtkButton *btn, gpointer data) {
    (void)btn; (void)data;
    GdkDisplay *gdk_display = gdk_display_get_default();
    if (!gdk_display) return;
    Display *xdpy = gdk_x11_display_get_xdisplay(gdk_display);
    Atom fullscreen = XInternAtom(xdpy, "_NET_WM_STATE_FULLSCREEN", False);
    toggle_active_state(fullscreen, 0);
}

static void on_settings_clicked(GtkButton *btn, gpointer data) {
    (void)btn;
    GtkWindow *parent = GTK_WINDOW(data);
    GError *err = NULL;
    if (!g_spawn_command_line_async("blazeneuro-settings", &err)) {
        GtkWidget *dialog = gtk_message_dialog_new(parent,
                                                   GTK_DIALOG_MODAL,
                                                   GTK_MESSAGE_ERROR,
                                                   GTK_BUTTONS_CLOSE,
                                                   "Unable to launch Settings: %s",
                                                   err ? err->message : "unknown error");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        if (err) g_error_free(err);
    }
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
    blazeneuro_style_window(win, "app-topbar");

    gtk_style_context_add_class(gtk_widget_get_style_context(win), "topbar");

    g_signal_connect(win, "realize", G_CALLBACK(on_realize), NULL);
    g_signal_connect(win, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_container_add(GTK_CONTAINER(win), hbox);

    GtkWidget *title = gtk_label_new("  BlazeNeuro");
    gtk_style_context_add_class(gtk_widget_get_style_context(title), "topbar-title");
    gtk_widget_set_margin_start(title, 12);
    gtk_box_pack_start(GTK_BOX(hbox), title, FALSE, FALSE, 0);

    GtkWidget *spacer = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(hbox), spacer, TRUE, TRUE, 0);

    GtkWidget *settings_btn = gtk_button_new_with_label("⚙");
    gtk_style_context_add_class(gtk_widget_get_style_context(settings_btn), "wm-control");
    g_signal_connect(settings_btn, "clicked", G_CALLBACK(on_settings_clicked), win);
    gtk_box_pack_end(GTK_BOX(hbox), settings_btn, FALSE, FALSE, 2);

    GtkWidget *close_btn = gtk_button_new_with_label("✕");
    gtk_style_context_add_class(gtk_widget_get_style_context(close_btn), "wm-control-close");
    g_signal_connect(close_btn, "clicked", G_CALLBACK(on_close_clicked), NULL);
    gtk_box_pack_end(GTK_BOX(hbox), close_btn, FALSE, FALSE, 2);

    GtkWidget *fullscreen_btn = gtk_button_new_with_label("⛶");
    gtk_style_context_add_class(gtk_widget_get_style_context(fullscreen_btn), "wm-control");
    g_signal_connect(fullscreen_btn, "clicked", G_CALLBACK(on_fullscreen_clicked), NULL);
    gtk_box_pack_end(GTK_BOX(hbox), fullscreen_btn, FALSE, FALSE, 2);

    GtkWidget *maximize_btn = gtk_button_new_with_label("▢");
    gtk_style_context_add_class(gtk_widget_get_style_context(maximize_btn), "wm-control");
    g_signal_connect(maximize_btn, "clicked", G_CALLBACK(on_maximize_clicked), NULL);
    gtk_box_pack_end(GTK_BOX(hbox), maximize_btn, FALSE, FALSE, 2);

    GtkWidget *minimize_btn = gtk_button_new_with_label("—");
    gtk_style_context_add_class(gtk_widget_get_style_context(minimize_btn), "wm-control");
    g_signal_connect(minimize_btn, "clicked", G_CALLBACK(on_minimize_clicked), NULL);
    gtk_box_pack_end(GTK_BOX(hbox), minimize_btn, FALSE, FALSE, 2);

    GtkWidget *clock_label = gtk_label_new("");
    gtk_style_context_add_class(gtk_widget_get_style_context(clock_label), "topbar-clock");
    gtk_widget_set_margin_end(clock_label, 16);
    gtk_widget_set_margin_start(clock_label, 10);
    gtk_box_pack_end(GTK_BOX(hbox), clock_label, FALSE, FALSE, 0);

    update_clock(clock_label);
    g_timeout_add_seconds(1, update_clock, clock_label);

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
