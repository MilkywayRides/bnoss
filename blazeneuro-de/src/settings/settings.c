/*
 * BlazeNeuro Settings
 * System settings panel with wallpaper, appearance, and display options.
 */

#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* ── Apply Wallpaper ────────────────────────────────────── */
static void set_wallpaper(const char *path) {
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "feh --bg-fill '%s' &", path);
    system(cmd);

    /* Save preference */
    const char *home = g_get_home_dir();
    char conf_path[512];
    snprintf(conf_path, sizeof(conf_path), "%s/.blazeneuro-wallpaper", home);
    FILE *f = fopen(conf_path, "w");
    if (f) {
        fprintf(f, "%s\n", path);
        fclose(f);
    }
}

static void on_wallpaper_select(GtkWidget *widget, gpointer data) {
    (void)widget; (void)data;
    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        "Choose Wallpaper", NULL, GTK_FILE_CHOOSER_ACTION_OPEN,
        "Cancel", GTK_RESPONSE_CANCEL,
        "Set Wallpaper", GTK_RESPONSE_ACCEPT, NULL);

    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Images");
    gtk_file_filter_add_mime_type(filter, "image/png");
    gtk_file_filter_add_mime_type(filter, "image/jpeg");
    gtk_file_filter_add_mime_type(filter, "image/svg+xml");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        set_wallpaper(filename);
        g_free(filename);
    }
    gtk_widget_destroy(dialog);
}

/* ── Create Section ─────────────────────────────────────── */
static GtkWidget *create_section(const char *title) {
    GtkWidget *frame = gtk_frame_new(NULL);
    gtk_style_context_add_class(gtk_widget_get_style_context(frame), "settings-section");

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 16);

    GtkWidget *label = gtk_label_new(title);
    gtk_style_context_add_class(gtk_widget_get_style_context(label), "section-title");
    gtk_label_set_xalign(GTK_LABEL(label), 0);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

    gtk_container_add(GTK_CONTAINER(frame), vbox);
    return frame;
}

static GtkWidget *section_content(GtkWidget *frame) {
    GtkWidget *vbox = gtk_bin_get_child(GTK_BIN(frame));
    return vbox;
}

/* ── Settings Row ───────────────────────────────────────── */
static GtkWidget *create_row(const char *label_text, GtkWidget *control) {
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_margin_top(hbox, 4);
    gtk_widget_set_margin_bottom(hbox, 4);

    GtkWidget *label = gtk_label_new(label_text);
    gtk_style_context_add_class(gtk_widget_get_style_context(label), "setting-label");
    gtk_label_set_xalign(GTK_LABEL(label), 0);
    gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 0);
    gtk_box_pack_end(GTK_BOX(hbox), control, FALSE, FALSE, 0);

    return hbox;
}

/* ── CSS ────────────────────────────────────────────────── */
static void apply_css(void) {
    GtkCssProvider *css = gtk_css_provider_new();
    const char *style =
        "window {"
        "  background-color: rgba(10, 10, 14, 0.92);"
        "}"
        ".sidebar-settings {"
        "  background-color: rgba(18, 18, 22, 0.95);"
        "  border-right: 1px solid rgba(255, 255, 255, 0.06);"
        "  padding: 12px 0;"
        "}"
        ".sidebar-settings row {"
        "  color: #a1a1aa;"
        "  border-radius: 8px;"
        "  margin: 1px 8px;"
        "  padding: 8px 12px;"
        "}"
        ".sidebar-settings row:selected {"
        "  background-color: rgba(59, 130, 246, 0.2);"
        "  color: #fafafa;"
        "}"
        ".settings-section {"
        "  background-color: rgba(24, 24, 28, 0.8);"
        "  border: 1px solid rgba(255, 255, 255, 0.06);"
        "  border-radius: 12px;"
        "  margin: 8px 16px;"
        "}"
        ".section-title {"
        "  color: #fafafa;"
        "  font-family: 'Inter', sans-serif;"
        "  font-size: 15px;"
        "  font-weight: 600;"
        "  margin-bottom: 8px;"
        "}"
        ".setting-label {"
        "  color: #a1a1aa;"
        "  font-family: 'Inter', sans-serif;"
        "  font-size: 13px;"
        "}"
        "button {"
        "  background-color: rgba(39, 39, 42, 0.8);"
        "  color: #fafafa;"
        "  border: 1px solid rgba(255, 255, 255, 0.1);"
        "  border-radius: 6px;"
        "  padding: 6px 16px;"
        "  font-family: 'Inter', sans-serif;"
        "  min-height: 32px;"
        "}"
        "button:hover {"
        "  background-color: rgba(63, 63, 70, 0.9);"
        "}"
        ".primary-btn {"
        "  background-color: rgba(59, 130, 246, 0.8);"
        "}"
        ".primary-btn:hover {"
        "  background-color: rgba(59, 130, 246, 1.0);"
        "}"
        "label.header {"
        "  color: #fafafa;"
        "  font-family: 'Inter', sans-serif;"
        "  font-size: 22px;"
        "  font-weight: 700;"
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

    GtkWidget *win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(win), "Settings");
    gtk_window_set_default_size(GTK_WINDOW(win), 800, 560);
    gtk_widget_set_app_paintable(win, TRUE);

    GdkScreen *scr = gtk_widget_get_screen(win);
    GdkVisual *vis = gdk_screen_get_rgba_visual(scr);
    if (vis) gtk_widget_set_visual(win, vis);

    g_signal_connect(win, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    /* Main content */
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(win), vbox);

    /* Header */
    GtkWidget *header_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_margin_start(header_box, 24);
    gtk_widget_set_margin_top(header_box, 20);
    gtk_widget_set_margin_bottom(header_box, 12);
    GtkWidget *header = gtk_label_new("Settings");
    gtk_style_context_add_class(gtk_widget_get_style_context(header), "header");
    gtk_box_pack_start(GTK_BOX(header_box), header, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), header_box, FALSE, FALSE, 0);

    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_box_pack_start(GTK_BOX(vbox), scroll, TRUE, TRUE, 0);

    GtkWidget *content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(scroll), content);

    /* Appearance section */
    GtkWidget *appearance = create_section("Appearance");
    GtkWidget *app_content = section_content(appearance);

    GtkWidget *wp_btn = gtk_button_new_with_label("Choose...");
    gtk_style_context_add_class(gtk_widget_get_style_context(wp_btn), "primary-btn");
    g_signal_connect(wp_btn, "clicked", G_CALLBACK(on_wallpaper_select), NULL);
    gtk_box_pack_start(GTK_BOX(app_content), create_row("Wallpaper", wp_btn), FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(content), appearance, FALSE, FALSE, 0);

    /* About section */
    GtkWidget *about = create_section("About");
    GtkWidget *about_content = section_content(about);

    GtkWidget *os_label = gtk_label_new("BlazeNeuro Desktop");
    gtk_style_context_add_class(gtk_widget_get_style_context(os_label), "setting-label");
    gtk_box_pack_start(GTK_BOX(about_content), create_row("System", os_label), FALSE, FALSE, 0);

    GtkWidget *ver_label = gtk_label_new("1.0");
    gtk_style_context_add_class(gtk_widget_get_style_context(ver_label), "setting-label");
    gtk_box_pack_start(GTK_BOX(about_content), create_row("Version", ver_label), FALSE, FALSE, 0);

    GtkWidget *de_label = gtk_label_new("BlazeNeuro DE");
    gtk_style_context_add_class(gtk_widget_get_style_context(de_label), "setting-label");
    gtk_box_pack_start(GTK_BOX(about_content), create_row("Desktop", de_label), FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(content), about, FALSE, FALSE, 0);

    gtk_widget_show_all(win);
    gtk_main();

    return 0;
}
