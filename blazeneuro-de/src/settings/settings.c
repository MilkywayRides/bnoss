/*
 * BlazeNeuro Settings
 * System settings panel with wallpaper, appearance, and display options.
 * Ubuntu-style category navigation with practical controls.
 */

#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../common/theme.h"

#define SETTINGS_CONF ".blazeneuro-settings.conf"

typedef struct {
    GtkWidget *theme_combo;
    GtkWidget *accent_combo;
    GtkWidget *animations_switch;
    GtkWidget *scale_combo;
    GtkWidget *refresh_combo;
    GtkWidget *sound_scale;
    GtkWidget *mic_scale;
    GtkWidget *sleep_combo;
    GtkWidget *battery_switch;
    GtkWidget *status_label;
} SettingsWidgets;

static void save_key_value(FILE *f, const char *k, const char *v) {
    fprintf(f, "%s=%s\n", k, v);
}

static void save_combo_value(FILE *f, const char *key, GtkWidget *combo) {
    gchar *val = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(combo));
    save_key_value(f, key, val ? val : "");
    g_free(val);
}

/* ── Apply Wallpaper ────────────────────────────────────── */
static void set_wallpaper(const char *path) {
    gchar *argv[] = { "feh", "--bg-fill", (gchar *)path, NULL };
    g_spawn_async(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, NULL);

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

/* ── UI Helpers ─────────────────────────────────────────── */
static GtkWidget *create_section(const char *title) {
    GtkWidget *frame = gtk_frame_new(NULL);
    gtk_style_context_add_class(gtk_widget_get_style_context(frame), "settings-section");

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 16);

    GtkWidget *label = gtk_label_new(title);
    gtk_style_context_add_class(gtk_widget_get_style_context(label), "section-title");
    gtk_label_set_xalign(GTK_LABEL(label), 0);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

    GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox), sep, FALSE, FALSE, 0);

    gtk_container_add(GTK_CONTAINER(frame), vbox);
    return frame;
}

static GtkWidget *section_content(GtkWidget *frame) {
    return gtk_bin_get_child(GTK_BIN(frame));
}

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

static void on_sidebar_row_selected(GtkListBox *box, GtkListBoxRow *row, gpointer data) {
    (void)box;
    if (!row) return;
    gtk_notebook_set_current_page(GTK_NOTEBOOK(data), gtk_list_box_row_get_index(row));
}

static GtkWidget *build_sidebar(GtkNotebook *book) {
    GtkWidget *list = gtk_list_box_new();
    gtk_widget_set_size_request(list, 180, -1);
    gtk_style_context_add_class(gtk_widget_get_style_context(list), "sidebar");

    const char *items[] = {"Appearance", "Display", "Sound", "Power", "About"};
    for (size_t i = 0; i < sizeof(items) / sizeof(items[0]); i++) {
        GtkWidget *row = gtk_list_box_row_new();
        GtkWidget *label = gtk_label_new(items[i]);
        gtk_label_set_xalign(GTK_LABEL(label), 0);
        gtk_container_add(GTK_CONTAINER(row), label);
        gtk_widget_set_margin_start(row, 8);
        gtk_widget_set_margin_end(row, 8);
        gtk_widget_set_margin_top(row, 2);
        gtk_widget_set_margin_bottom(row, 2);
        gtk_container_add(GTK_CONTAINER(list), row);
    }

    g_signal_connect(list, "row-selected", G_CALLBACK(on_sidebar_row_selected), book);

    gtk_list_box_select_row(GTK_LIST_BOX(list), gtk_list_box_get_row_at_index(GTK_LIST_BOX(list), 0));
    return list;
}

static void save_settings(GtkButton *button, gpointer user_data) {
    (void)button;
    SettingsWidgets *w = (SettingsWidgets *)user_data;

    const char *home = g_get_home_dir();
    char conf_path[512];
    snprintf(conf_path, sizeof(conf_path), "%s/%s", home, SETTINGS_CONF);

    FILE *f = fopen(conf_path, "w");
    if (!f) {
        gtk_label_set_text(GTK_LABEL(w->status_label), "Unable to save settings.");
        return;
    }

    save_combo_value(f, "theme", w->theme_combo);
    save_combo_value(f, "accent", w->accent_combo);
    save_key_value(f, "animations", gtk_switch_get_active(GTK_SWITCH(w->animations_switch)) ? "1" : "0");
    save_combo_value(f, "scale", w->scale_combo);
    save_combo_value(f, "refresh", w->refresh_combo);

    char buf[32];
    snprintf(buf, sizeof(buf), "%.0f", gtk_range_get_value(GTK_RANGE(w->sound_scale)));
    save_key_value(f, "volume", buf);
    snprintf(buf, sizeof(buf), "%.0f", gtk_range_get_value(GTK_RANGE(w->mic_scale)));
    save_key_value(f, "microphone", buf);
    save_combo_value(f, "sleep", w->sleep_combo);
    save_key_value(f, "battery_saver", gtk_switch_get_active(GTK_SWITCH(w->battery_switch)) ? "1" : "0");

    fclose(f);
    gtk_label_set_text(GTK_LABEL(w->status_label), "Settings saved. Changes apply to new sessions.");
}

static GtkWidget *build_appearance_page(SettingsWidgets *widgets) {
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    GtkWidget *appearance = create_section("Appearance");
    GtkWidget *content = section_content(appearance);

    GtkWidget *wp_btn = gtk_button_new_with_label("Choose…");
    gtk_style_context_add_class(gtk_widget_get_style_context(wp_btn), "primary-btn");
    g_signal_connect(wp_btn, "clicked", G_CALLBACK(on_wallpaper_select), NULL);
    gtk_box_pack_start(GTK_BOX(content), create_row("Wallpaper", wp_btn), FALSE, FALSE, 0);

    widgets->theme_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widgets->theme_combo), "Dark");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widgets->theme_combo), "Light");
    gtk_combo_box_set_active(GTK_COMBO_BOX(widgets->theme_combo), 0);
    gtk_box_pack_start(GTK_BOX(content), create_row("Theme", widgets->theme_combo), FALSE, FALSE, 0);

    widgets->accent_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widgets->accent_combo), "Slate");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widgets->accent_combo), "Blue");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widgets->accent_combo), "Purple");
    gtk_combo_box_set_active(GTK_COMBO_BOX(widgets->accent_combo), 0);
    gtk_box_pack_start(GTK_BOX(content), create_row("Accent color", widgets->accent_combo), FALSE, FALSE, 0);

    widgets->animations_switch = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(widgets->animations_switch), TRUE);
    gtk_box_pack_start(GTK_BOX(content), create_row("Enable animations", widgets->animations_switch), FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(page), appearance, FALSE, FALSE, 0);
    return page;
}

static GtkWidget *build_display_page(SettingsWidgets *widgets) {
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *display = create_section("Display");
    GtkWidget *content = section_content(display);

    widgets->scale_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widgets->scale_combo), "100%");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widgets->scale_combo), "125%");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widgets->scale_combo), "150%");
    gtk_combo_box_set_active(GTK_COMBO_BOX(widgets->scale_combo), 0);
    gtk_box_pack_start(GTK_BOX(content), create_row("Scale", widgets->scale_combo), FALSE, FALSE, 0);

    widgets->refresh_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widgets->refresh_combo), "60 Hz");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widgets->refresh_combo), "120 Hz");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widgets->refresh_combo), "144 Hz");
    gtk_combo_box_set_active(GTK_COMBO_BOX(widgets->refresh_combo), 0);
    gtk_box_pack_start(GTK_BOX(content), create_row("Refresh rate", widgets->refresh_combo), FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(page), display, FALSE, FALSE, 0);
    return page;
}

static GtkWidget *build_sound_page(SettingsWidgets *widgets) {
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *sound = create_section("Sound");
    GtkWidget *content = section_content(sound);

    widgets->sound_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
    gtk_range_set_value(GTK_RANGE(widgets->sound_scale), 75);
    gtk_widget_set_size_request(widgets->sound_scale, 220, -1);
    gtk_box_pack_start(GTK_BOX(content), create_row("Output volume", widgets->sound_scale), FALSE, FALSE, 0);

    widgets->mic_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
    gtk_range_set_value(GTK_RANGE(widgets->mic_scale), 65);
    gtk_widget_set_size_request(widgets->mic_scale, 220, -1);
    gtk_box_pack_start(GTK_BOX(content), create_row("Input volume", widgets->mic_scale), FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(page), sound, FALSE, FALSE, 0);
    return page;
}

static GtkWidget *build_power_page(SettingsWidgets *widgets) {
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *power = create_section("Power");
    GtkWidget *content = section_content(power);

    widgets->sleep_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widgets->sleep_combo), "Never");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widgets->sleep_combo), "15 minutes");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widgets->sleep_combo), "30 minutes");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widgets->sleep_combo), "1 hour");
    gtk_combo_box_set_active(GTK_COMBO_BOX(widgets->sleep_combo), 2);
    gtk_box_pack_start(GTK_BOX(content), create_row("Blank screen", widgets->sleep_combo), FALSE, FALSE, 0);

    widgets->battery_switch = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(widgets->battery_switch), FALSE);
    gtk_box_pack_start(GTK_BOX(content), create_row("Battery saver", widgets->battery_switch), FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(page), power, FALSE, FALSE, 0);
    return page;
}

static GtkWidget *build_about_page(void) {
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *about = create_section("About");
    GtkWidget *content = section_content(about);

    GtkWidget *os_label = gtk_label_new("BlazeNeuro Desktop");
    gtk_style_context_add_class(gtk_widget_get_style_context(os_label), "muted");
    gtk_box_pack_start(GTK_BOX(content), create_row("System", os_label), FALSE, FALSE, 0);

    GtkWidget *ver_label = gtk_label_new("1.1");
    gtk_style_context_add_class(gtk_widget_get_style_context(ver_label), "muted");
    gtk_box_pack_start(GTK_BOX(content), create_row("Version", ver_label), FALSE, FALSE, 0);

    GtkWidget *de_label = gtk_label_new("BlazeNeuro DE");
    gtk_style_context_add_class(gtk_widget_get_style_context(de_label), "muted");
    gtk_box_pack_start(GTK_BOX(content), create_row("Desktop", de_label), FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(page), about, FALSE, FALSE, 0);
    return page;
}

/* ── Main ───────────────────────────────────────────────── */
int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    blazeneuro_load_theme();

    SettingsWidgets widgets = {0};

    GtkWidget *win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(win), "Settings");
    gtk_window_set_default_size(GTK_WINDOW(win), 940, 620);
    blazeneuro_style_window(win, "app-settings");

    g_signal_connect(win, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(win), main_vbox);

    GtkWidget *header_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_margin_start(header_box, 24);
    gtk_widget_set_margin_top(header_box, 20);
    gtk_widget_set_margin_bottom(header_box, 12);
    GtkWidget *header = gtk_label_new("Settings");
    gtk_style_context_add_class(gtk_widget_get_style_context(header), "header");
    gtk_box_pack_start(GTK_BOX(header_box), header, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(main_vbox), header_box, FALSE, FALSE, 0);

    GtkWidget *body = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(main_vbox), body, TRUE, TRUE, 0);

    GtkWidget *notebook = gtk_notebook_new();
    gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook), FALSE);
    gtk_notebook_set_show_border(GTK_NOTEBOOK(notebook), FALSE);

    GtkWidget *sidebar = build_sidebar(GTK_NOTEBOOK(notebook));
    gtk_box_pack_start(GTK_BOX(body), sidebar, FALSE, FALSE, 0);

    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scroll), notebook);
    gtk_box_pack_start(GTK_BOX(body), scroll, TRUE, TRUE, 0);

    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), build_appearance_page(&widgets), NULL);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), build_display_page(&widgets), NULL);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), build_sound_page(&widgets), NULL);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), build_power_page(&widgets), NULL);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), build_about_page(), NULL);

    GtkWidget *footer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_margin_start(footer, 24);
    gtk_widget_set_margin_end(footer, 24);
    gtk_widget_set_margin_top(footer, 8);
    gtk_widget_set_margin_bottom(footer, 12);

    widgets.status_label = gtk_label_new("Ready");
    gtk_style_context_add_class(gtk_widget_get_style_context(widgets.status_label), "muted");
    gtk_label_set_xalign(GTK_LABEL(widgets.status_label), 0);
    gtk_box_pack_start(GTK_BOX(footer), widgets.status_label, TRUE, TRUE, 0);

    GtkWidget *save_btn = gtk_button_new_with_label("Save Changes");
    gtk_style_context_add_class(gtk_widget_get_style_context(save_btn), "primary-btn");
    g_signal_connect(save_btn, "clicked", G_CALLBACK(save_settings), &widgets);
    gtk_box_pack_end(GTK_BOX(footer), save_btn, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(main_vbox), footer, FALSE, FALSE, 0);

    gtk_widget_show_all(win);
    gtk_main();

    return 0;
}
