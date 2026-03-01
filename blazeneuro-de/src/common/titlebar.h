/*
 * BlazeNeuro Custom Title Bar (CSD)
 * Shared header providing macOS-style traffic-light window controls.
 * Include in any GTK app that needs close/minimize/fullscreen buttons.
 *
 * Usage:
 *   GtkWidget *content = ...; // your main content widget
 *   blazeneuro_add_titlebar(win, "App Title", content);
 */

#ifndef BLAZENEURO_TITLEBAR_H
#define BLAZENEURO_TITLEBAR_H

#include <gtk/gtk.h>

/* ── State for fullscreen toggle ───────────────────────── */
static int _bn_is_fullscreen = 0;

/* ── Button callbacks ──────────────────────────────────── */
static void _bn_titlebar_close(GtkWidget *btn, gpointer data) {
    (void)btn;
    GtkWidget *win = GTK_WIDGET(data);
    gtk_widget_destroy(win);
}

static void _bn_titlebar_minimize(GtkWidget *btn, gpointer data) {
    (void)btn;
    GtkWidget *win = GTK_WIDGET(data);
    gtk_window_iconify(GTK_WINDOW(win));
}

static void _bn_titlebar_fullscreen(GtkWidget *btn, gpointer data) {
    (void)btn;
    GtkWidget *win = GTK_WIDGET(data);
    if (_bn_is_fullscreen) {
        gtk_window_unfullscreen(GTK_WINDOW(win));
        _bn_is_fullscreen = 0;
    } else {
        gtk_window_fullscreen(GTK_WINDOW(win));
        _bn_is_fullscreen = 1;
    }
}

/* ── Drag support: move window by dragging the title bar ─ */
static gdouble _bn_drag_x, _bn_drag_y;
static gboolean _bn_dragging = FALSE;

static gboolean _bn_titlebar_press(GtkWidget *widget, GdkEventButton *ev, gpointer data) {
    (void)widget;
    (void)data;
    if (ev->button == 1) {
        _bn_dragging = TRUE;
        _bn_drag_x = ev->x_root;
        _bn_drag_y = ev->y_root;
    }
    return FALSE;
}

static gboolean _bn_titlebar_release(GtkWidget *widget, GdkEventButton *ev, gpointer data) {
    (void)widget; (void)ev; (void)data;
    _bn_dragging = FALSE;
    return FALSE;
}

static gboolean _bn_titlebar_motion(GtkWidget *widget, GdkEventMotion *ev, gpointer data) {
    (void)widget;
    if (_bn_dragging) {
        GtkWidget *win = GTK_WIDGET(data);
        gint wx, wy;
        gtk_window_get_position(GTK_WINDOW(win), &wx, &wy);
        gtk_window_move(GTK_WINDOW(win),
                        wx + (gint)(ev->x_root - _bn_drag_x),
                        wy + (gint)(ev->y_root - _bn_drag_y));
        _bn_drag_x = ev->x_root;
        _bn_drag_y = ev->y_root;
    }
    return FALSE;
}

/* ── Double-click title bar to toggle fullscreen ───────── */
static gboolean _bn_titlebar_dblclick(GtkWidget *widget, GdkEventButton *ev, gpointer data) {
    (void)widget;
    if (ev->type == GDK_2BUTTON_PRESS && ev->button == 1) {
        _bn_titlebar_fullscreen(NULL, data);
        return TRUE;
    }
    return FALSE;
}

/* ── Create a traffic-light button ─────────────────────── */
static GtkWidget *_bn_make_circle_btn(const char *css_class,
                                       GCallback callback,
                                       gpointer data) {
    GtkWidget *btn = gtk_button_new();
    gtk_widget_set_size_request(btn, 14, 14);
    gtk_style_context_add_class(gtk_widget_get_style_context(btn), "titlebar-btn");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn), css_class);
    g_signal_connect(btn, "clicked", callback, data);
    return btn;
}

/**
 * blazeneuro_add_titlebar:
 * @win: the GtkWindow to add a titlebar to
 * @title: display text for the title bar
 * @content: the main content widget (will be packed below the title bar)
 *
 * Wraps the window content in a vbox with a custom title bar on top.
 * The title bar has macOS-style traffic-light buttons (close, minimize, fullscreen).
 * Must be called BEFORE gtk_widget_show_all().
 */
static inline void blazeneuro_add_titlebar(GtkWidget *win,
                                            const char *title,
                                            GtkWidget *content) {
    /* Disable server-side decorations */
    gtk_window_set_decorated(GTK_WINDOW(win), FALSE);

    /* Main vertical container */
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    /* Title bar container — event box for dragging */
    GtkWidget *titlebar_ebox = gtk_event_box_new();
    gtk_widget_add_events(titlebar_ebox,
                          GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
                          GDK_POINTER_MOTION_MASK);
    g_signal_connect(titlebar_ebox, "button-press-event",
                     G_CALLBACK(_bn_titlebar_press), win);
    g_signal_connect(titlebar_ebox, "button-press-event",
                     G_CALLBACK(_bn_titlebar_dblclick), win);
    g_signal_connect(titlebar_ebox, "button-release-event",
                     G_CALLBACK(_bn_titlebar_release), win);
    g_signal_connect(titlebar_ebox, "motion-notify-event",
                     G_CALLBACK(_bn_titlebar_motion), win);

    GtkWidget *titlebar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_style_context_add_class(gtk_widget_get_style_context(titlebar), "titlebar");
    gtk_container_add(GTK_CONTAINER(titlebar_ebox), titlebar);

    /* Traffic light buttons (left side, macOS style) */
    GtkWidget *btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_margin_start(btn_box, 12);
    gtk_widget_set_valign(btn_box, GTK_ALIGN_CENTER);

    GtkWidget *close_btn = _bn_make_circle_btn(
        "titlebar-btn-close", G_CALLBACK(_bn_titlebar_close), win);
    GtkWidget *min_btn = _bn_make_circle_btn(
        "titlebar-btn-minimize", G_CALLBACK(_bn_titlebar_minimize), win);
    GtkWidget *full_btn = _bn_make_circle_btn(
        "titlebar-btn-fullscreen", G_CALLBACK(_bn_titlebar_fullscreen), win);

    gtk_box_pack_start(GTK_BOX(btn_box), close_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(btn_box), min_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(btn_box), full_btn, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(titlebar), btn_box, FALSE, FALSE, 0);

    /* Title label (centered) */
    GtkWidget *title_label = gtk_label_new(title);
    gtk_style_context_add_class(gtk_widget_get_style_context(title_label), "titlebar-title");
    gtk_widget_set_hexpand(title_label, TRUE);
    gtk_label_set_xalign(GTK_LABEL(title_label), 0.5);
    gtk_box_pack_start(GTK_BOX(titlebar), title_label, TRUE, TRUE, 0);

    /* Right spacer to balance the buttons */
    GtkWidget *spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_size_request(spacer, 70, -1); /* ~same width as 3 buttons */
    gtk_box_pack_end(GTK_BOX(titlebar), spacer, FALSE, FALSE, 0);

    /* Assemble: titlebar on top, content below */
    gtk_box_pack_start(GTK_BOX(vbox), titlebar_ebox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), content, TRUE, TRUE, 0);

    gtk_container_add(GTK_CONTAINER(win), vbox);
}

#endif /* BLAZENEURO_TITLEBAR_H */
