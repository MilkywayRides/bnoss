/*
 * BlazeNeuro Terminal
 * Modern terminal emulator using VTE + GTK3.
 * Styled with transparent background and modern fonts.
 */

#include <gtk/gtk.h>
#include <vte/vte.h>
#include <stdlib.h>

static void on_child_exit(VteTerminal *term, gint status, gpointer data) {
    (void)term; (void)status; (void)data;
    gtk_main_quit();
}

static gboolean on_key_press(GtkWidget *widget, GdkEventKey *ev, gpointer data) {
    VteTerminal *term = VTE_TERMINAL(data);

    /* Ctrl+Shift+C: Copy */
    if ((ev->state & GDK_CONTROL_MASK) && (ev->state & GDK_SHIFT_MASK) && ev->keyval == GDK_KEY_C) {
        vte_terminal_copy_clipboard_format(term, VTE_FORMAT_TEXT);
        return TRUE;
    }
    /* Ctrl+Shift+V: Paste */
    if ((ev->state & GDK_CONTROL_MASK) && (ev->state & GDK_SHIFT_MASK) && ev->keyval == GDK_KEY_V) {
        vte_terminal_paste_clipboard(term);
        return TRUE;
    }
    /* Ctrl+Shift+Plus: Zoom in */
    if ((ev->state & GDK_CONTROL_MASK) && (ev->state & GDK_SHIFT_MASK) && ev->keyval == GDK_KEY_plus) {
        gdouble scale = vte_terminal_get_font_scale(term);
        vte_terminal_set_font_scale(term, scale + 0.1);
        return TRUE;
    }
    /* Ctrl+Minus: Zoom out */
    if ((ev->state & GDK_CONTROL_MASK) && ev->keyval == GDK_KEY_minus) {
        gdouble scale = vte_terminal_get_font_scale(term);
        if (scale > 0.3) vte_terminal_set_font_scale(term, scale - 0.1);
        return TRUE;
    }

    (void)widget;
    return FALSE;
}

static void apply_css(void) {
    GtkCssProvider *css = gtk_css_provider_new();
    const char *style =
        "window {"
        "  background-color: rgba(10, 10, 14, 0.88);"
        "  border-radius: 12px;"
        "}"
        "headerbar {"
        "  background-color: rgba(18, 18, 22, 0.92);"
        "  border-bottom: 1px solid rgba(255, 255, 255, 0.06);"
        "  min-height: 36px;"
        "  padding: 0 8px;"
        "}"
        "headerbar .title {"
        "  color: #a1a1aa;"
        "  font-family: 'Inter', sans-serif;"
        "  font-size: 13px;"
        "  font-weight: 500;"
        "}";
    gtk_css_provider_load_from_data(css, style, -1, NULL);
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(css),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(css);
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    apply_css();

    /* Window */
    GtkWidget *win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(win), "Terminal");
    gtk_window_set_default_size(GTK_WINDOW(win), 820, 520);
    gtk_widget_set_app_paintable(win, TRUE);

    GdkScreen *scr = gtk_widget_get_screen(win);
    GdkVisual *vis = gdk_screen_get_rgba_visual(scr);
    if (vis) gtk_widget_set_visual(win, vis);

    g_signal_connect(win, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    /* Terminal widget */
    GtkWidget *term = vte_terminal_new();
    VteTerminal *vte = VTE_TERMINAL(term);

    /* Colors — shadcn dark theme */
    GdkRGBA fg = {0.96, 0.96, 0.98, 1.0};      /* #f5f5f7 */
    GdkRGBA bg = {0.04, 0.04, 0.055, 0.88};     /* transparent dark */

    GdkRGBA palette[16] = {
        {0.14, 0.14, 0.16, 1},  /* black */
        {0.94, 0.33, 0.33, 1},  /* red */
        {0.40, 0.85, 0.55, 1},  /* green */
        {0.98, 0.78, 0.35, 1},  /* yellow */
        {0.23, 0.51, 0.96, 1},  /* blue */
        {0.69, 0.40, 0.96, 1},  /* magenta */
        {0.30, 0.82, 0.88, 1},  /* cyan */
        {0.80, 0.80, 0.82, 1},  /* white */
        {0.35, 0.35, 0.39, 1},  /* bright black */
        {1.00, 0.45, 0.45, 1},  /* bright red */
        {0.50, 0.95, 0.65, 1},  /* bright green */
        {1.00, 0.88, 0.45, 1},  /* bright yellow */
        {0.45, 0.65, 1.00, 1},  /* bright blue */
        {0.80, 0.55, 1.00, 1},  /* bright magenta */
        {0.45, 0.92, 0.98, 1},  /* bright cyan */
        {0.96, 0.96, 0.98, 1},  /* bright white */
    };

    vte_terminal_set_colors(vte, &fg, &bg, palette, 16);
    vte_terminal_set_cursor_blink_mode(vte, VTE_CURSOR_BLINK_ON);
    vte_terminal_set_scrollback_lines(vte, 10000);
    vte_terminal_set_mouse_autohide(vte, TRUE);

    /* Font */
    PangoFontDescription *font = pango_font_description_from_string("JetBrains Mono 11");
    if (!font) font = pango_font_description_from_string("Monospace 11");
    vte_terminal_set_font(vte, font);
    pango_font_description_free(font);

    /* Padding */
    vte_terminal_set_cell_height_scale(vte, 1.2);

    /* Spawn shell */
    char *shell = getenv("SHELL");
    if (!shell) shell = "/bin/bash";
    char *shell_argv[] = { shell, NULL };

    vte_terminal_spawn_async(vte,
        VTE_PTY_DEFAULT, NULL, shell_argv, NULL,
        G_SPAWN_DEFAULT, NULL, NULL, NULL, -1, NULL, NULL, NULL);

    g_signal_connect(vte, "child-exited", G_CALLBACK(on_child_exit), NULL);
    g_signal_connect(win, "key-press-event", G_CALLBACK(on_key_press), vte);

    gtk_container_add(GTK_CONTAINER(win), term);
    gtk_widget_show_all(win);
    gtk_main();

    return 0;
}
