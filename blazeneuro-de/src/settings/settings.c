/*
 * BlazeNeuro Settings
 * System settings panel with wallpaper, appearance, and display options.
 * shadcn card-based layout.
 */

#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* ── Shared Theme Loader ───────────────────────────────── */
static void load_theme(void) {
    GtkCssProvider *css = gtk_css_provider_new();
    const char *paths[] = {
        "/usr/local/share/blazeneuro/blazeneuro.css",
        "theme/blazeneuro.css",
        "../theme/blazeneuro.css",
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

/* ── Apply Wallpaper ────────────────────────────────────── */
static void set_wallpaper(const char *path) {
    gchar *argv[] = { "feh", "--bg-fill", (gchar *)path, NULL };
    g_spawn_async(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, NULL);

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
        if (filename) {
            set_wallpaper(filename);
            g_free(filename);
        }
    }
    gtk_widget_destroy(dialog);
}

/* ── Create Section (shadcn Card) ───────────────────────── */
static GtkWidget *create_section(const char *title) {
    GtkWidget *frame = gtk_frame_new(NULL);
    gtk_style_context_add_class(gtk_widget_get_style_context(frame), "settings-section");

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 16);

    GtkWidget *label = gtk_label_new(title);
    gtk_style_context_add_class(gtk_widget_get_style_context(label), "section-title");
    gtk_label_set_xalign(GTK_LABEL(label), 0);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

    /* Add separator below title */
    GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox), sep, FALSE, FALSE, 0);

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
    gtk_style_context_add_class(gtk_widget_get_style_context(label), "muted");
    gtk_label_set_xalign(GTK_LABEL(label), 0);
    gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 0);
    gtk_box_pack_end(GTK_BOX(hbox), control, FALSE, FALSE, 0);

    return hbox;
}

/* ── Main ───────────────────────────────────────────────── */
int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    load_theme();

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

    GtkWidget *wp_btn = gtk_button_new_with_label("Choose…");
    gtk_style_context_add_class(gtk_widget_get_style_context(wp_btn), "primary-btn");
    g_signal_connect(wp_btn, "clicked", G_CALLBACK(on_wallpaper_select), NULL);
    gtk_box_pack_start(GTK_BOX(app_content), create_row("Wallpaper", wp_btn), FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(content), appearance, FALSE, FALSE, 0);

    /* About section */
    GtkWidget *about = create_section("About");
    GtkWidget *about_content = section_content(about);

    GtkWidget *os_label = gtk_label_new("BlazeNeuro Desktop");
    gtk_style_context_add_class(gtk_widget_get_style_context(os_label), "muted");
    gtk_box_pack_start(GTK_BOX(about_content), create_row("System", os_label), FALSE, FALSE, 0);

    GtkWidget *ver_label = gtk_label_new("1.0");
    gtk_style_context_add_class(gtk_widget_get_style_context(ver_label), "muted");
    gtk_box_pack_start(GTK_BOX(about_content), create_row("Version", ver_label), FALSE, FALSE, 0);

    GtkWidget *de_label = gtk_label_new("BlazeNeuro DE");
    gtk_style_context_add_class(gtk_widget_get_style_context(de_label), "muted");
    gtk_box_pack_start(GTK_BOX(about_content), create_row("Desktop", de_label), FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(content), about, FALSE, FALSE, 0);

    gtk_widget_show_all(win);
    gtk_main();

    return 0;
}
