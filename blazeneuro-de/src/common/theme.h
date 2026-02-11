/*
 * BlazeNeuro Common Theme Loader
 * Shared header for all GTK apps to load the unified  CSS theme.
 * Include this in every GTK app instead of duplicating load_theme().
 */

#ifndef BLAZENEURO_THEME_H
#define BLAZENEURO_THEME_H

#include <gtk/gtk.h>

static inline void blazeneuro_load_theme(void) {
    GtkCssProvider *css = gtk_css_provider_new();
    const char *paths[] = {
        "/usr/local/share/blazeneuro/blazeneuro.css",
        "theme/blazeneuro.css",
        "../theme/blazeneuro.css",
        "../../theme/blazeneuro.css",
        NULL
    };
    for (int i = 0; paths[i]; i++) {
        if (g_file_test(paths[i], G_FILE_TEST_EXISTS)) {
            gtk_css_provider_load_from_path(css, paths[i], NULL);
            break;
        }
    }
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(css),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(css);
}

/**
 * Set up RGBA visual for transparent windows.
 * Call after gtk_init() but before creating widgets.
 */
static inline void blazeneuro_setup_rgba(GtkWidget *win) {
    GdkScreen *scr = gtk_widget_get_screen(win);
    GdkVisual *vis = gdk_screen_get_rgba_visual(scr);
    if (vis) gtk_widget_set_visual(win, vis);
    gtk_widget_set_app_paintable(win, TRUE);
}

#endif /* BLAZENEURO_THEME_H */
