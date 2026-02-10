/*
 * BlazeNeuro Dock
 * macOS-like bottom dock with app icons, hover effects, and tooltips.
 * Written in C with GTK3.
 */

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define DOCK_HEIGHT  64
#define ICON_SIZE    48
#define ICON_PADDING 8
#define DOCK_MARGIN  8

/* ── App Entry ──────────────────────────────────────────── */
typedef struct {
    const char *name;
    const char *exec;
    const char *icon;
} DockApp;

static DockApp dock_apps[] = {
    { "Files",       "blazeneuro-files",    "system-file-manager" },
    { "Terminal",    "blazeneuro-terminal",  "utilities-terminal" },
    { "Chromium",    "chromium-browser --no-sandbox", "chromium-browser" },
    { "VS Code",     "code --no-sandbox",   "com.visualstudio.code" },
    { "Settings",    "blazeneuro-settings",  "preferences-system" },
    { "Software",    "gnome-software",       "org.gnome.Software" },
    { "Text Editor", "mousepad",             "accessories-text-editor" },
    { "Calculator",  "gnome-calculator",     "accessories-calculator" },
    { NULL, NULL, NULL }
};

/* ── Launch App ─────────────────────────────────────────── */
static void launch_app(GtkWidget *widget, gpointer data) {
    (void)widget;
    const char *cmd = (const char *)data;
    gchar *full_cmd;
    full_cmd = g_strdup_printf("%s &", cmd);
    g_spawn_command_line_async(full_cmd, NULL);
    g_free(full_cmd);
}

/* ── Hover Effects ──────────────────────────────────────── */
static gboolean on_hover_enter(GtkWidget *widget, GdkEventCrossing *ev, gpointer data) {
    (void)ev; (void)data;
    /* Scale up on hover */
    gtk_widget_set_size_request(widget, ICON_SIZE + 12, ICON_SIZE + 12);
    return FALSE;
}

static gboolean on_hover_leave(GtkWidget *widget, GdkEventCrossing *ev, gpointer data) {
    (void)ev; (void)data;
    gtk_widget_set_size_request(widget, ICON_SIZE, ICON_SIZE);
    return FALSE;
}

/* ── Set Window as Dock Type ────────────────────────────── */
static void on_realize(GtkWidget *widget, gpointer data) {
    (void)data;
    GdkWindow *gdk_win = gtk_widget_get_window(widget);
    GdkDisplay *display = gdk_window_get_display(gdk_win);
    Display *xdpy = gdk_x11_display_get_xdisplay(display);
    Window xwin = gdk_x11_window_get_xid(gdk_win);

    /* Set struts so WM reserves space */
    GdkMonitor *mon = gdk_display_get_primary_monitor(display);
    GdkRectangle geom;
    gdk_monitor_get_geometry(mon, &geom);

    long struts[12] = {0};
    struts[3] = DOCK_HEIGHT + DOCK_MARGIN; /* bottom */
    struts[10] = 0;                        /* bottom_start_x */
    struts[11] = geom.width;               /* bottom_end_x */

    Atom strut_partial = XInternAtom(xdpy, "_NET_WM_STRUT_PARTIAL", False);
    XChangeProperty(xdpy, xwin, strut_partial, XA_CARDINAL, 32,
                    PropModeReplace, (unsigned char *)struts, 12);
}

/* ── CSS Styling ────────────────────────────────────────── */
static void apply_css(void) {
    GtkCssProvider *css = gtk_css_provider_new();
    const char *style =
        "window {"
        "  background-color: rgba(20, 20, 22, 0.75);"
        "  border-radius: 16px;"
        "  border: 1px solid rgba(255, 255, 255, 0.08);"
        "}"
        ".dock-button {"
        "  background: transparent;"
        "  border: none;"
        "  border-radius: 12px;"
        "  padding: 6px;"
        "  transition: all 200ms ease;"
        "  min-width: 48px;"
        "  min-height: 48px;"
        "}"
        ".dock-button:hover {"
        "  background-color: rgba(255, 255, 255, 0.1);"
        "}";
    gtk_css_provider_load_from_data(css, style, -1, NULL);
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(css),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(css);
}

/* ── Main ───────────────────────────────────────────────── */
int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    apply_css();

    /* Create dock window */
    GtkWidget *win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(win), "BlazeNeuro Dock");
    gtk_window_set_type_hint(GTK_WINDOW(win), GDK_WINDOW_TYPE_HINT_DOCK);
    gtk_window_set_decorated(GTK_WINDOW(win), FALSE);
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(win), TRUE);
    gtk_window_set_skip_pager_hint(GTK_WINDOW(win), TRUE);
    gtk_widget_set_app_paintable(win, TRUE);

    /* Enable RGBA visual for transparency */
    GdkScreen *scr = gtk_widget_get_screen(win);
    GdkVisual *vis = gdk_screen_get_rgba_visual(scr);
    if (vis) gtk_widget_set_visual(win, vis);

    g_signal_connect(win, "realize", G_CALLBACK(on_realize), NULL);
    g_signal_connect(win, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    /* Icon container */
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, ICON_PADDING);
    gtk_container_set_border_width(GTK_CONTAINER(box), 8);
    gtk_container_add(GTK_CONTAINER(win), box);

    /* Add app icons */
    for (int i = 0; dock_apps[i].name != NULL; i++) {
        GtkWidget *btn = gtk_button_new();
        gtk_style_context_add_class(gtk_widget_get_style_context(btn), "dock-button");
        gtk_widget_set_tooltip_text(btn, dock_apps[i].name);

        GtkWidget *icon = gtk_image_new_from_icon_name(dock_apps[i].icon, GTK_ICON_SIZE_DIALOG);
        gtk_image_set_pixel_size(GTK_IMAGE(icon), ICON_SIZE);
        gtk_container_add(GTK_CONTAINER(btn), icon);

        g_signal_connect(btn, "clicked", G_CALLBACK(launch_app), (gpointer)dock_apps[i].exec);
        g_signal_connect(btn, "enter-notify-event", G_CALLBACK(on_hover_enter), NULL);
        g_signal_connect(btn, "leave-notify-event", G_CALLBACK(on_hover_leave), NULL);

        gtk_box_pack_start(GTK_BOX(box), btn, FALSE, FALSE, 0);
    }

    /* Position at bottom center */
    GdkMonitor *mon = gdk_display_get_primary_monitor(gdk_display_get_default());
    GdkRectangle geom;
    gdk_monitor_get_geometry(mon, &geom);

    int num_apps = 0;
    for (int i = 0; dock_apps[i].name != NULL; i++) num_apps++;
    int dock_w = num_apps * (ICON_SIZE + ICON_PADDING * 2) + 16;
    int dock_x = geom.x + (geom.width - dock_w) / 2;
    int dock_y = geom.y + geom.height - DOCK_HEIGHT - DOCK_MARGIN;

    gtk_window_set_default_size(GTK_WINDOW(win), dock_w, DOCK_HEIGHT);
    gtk_window_move(GTK_WINDOW(win), dock_x, dock_y);

    gtk_widget_show_all(win);
    gtk_main();

    return 0;
}
