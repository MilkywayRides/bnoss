/*
 * BlazeNeuro Desktop View
 * Provides desktop wallpaper rendering + rich right-click context menu.
 */

#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../common/theme.h"

static GtkWidget *desktop_win;
static GtkWidget *desktop_image;

/* ── Wallpaper ──────────────────────────────────────────── */
static void save_wallpaper_path(const char *path) {
    const char *home = g_get_home_dir();
    char conf_path[512];
    g_snprintf(conf_path, sizeof(conf_path), "%s/.blazeneuro-wallpaper", home);

    FILE *f = fopen(conf_path, "w");
    if (!f) return;
    fprintf(f, "%s\n", path);
    fclose(f);
}

static gboolean apply_wallpaper_file(const char *path) {
    if (!path || !path[0]) return FALSE;

    GError *err = NULL;
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(path, &err);
    if (!pixbuf) {
        if (err) g_error_free(err);
        return FALSE;
    }

    GdkDisplay *display = gdk_display_get_default();
    GdkMonitor *mon = gdk_display_get_primary_monitor(display);
    if (!mon) {
        g_object_unref(pixbuf);
        return FALSE;
    }
    GdkRectangle geom;
    gdk_monitor_get_geometry(mon, &geom);

    GdkPixbuf *scaled = gdk_pixbuf_scale_simple(
        pixbuf, geom.width, geom.height, GDK_INTERP_BILINEAR);

    g_object_unref(pixbuf);
    if (!scaled) return FALSE;

    gtk_image_set_from_pixbuf(GTK_IMAGE(desktop_image), scaled);
    g_object_unref(scaled);

    gchar *argv[] = { "feh", "--bg-fill", (gchar *)path, NULL };
    g_spawn_async(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, NULL);

    save_wallpaper_path(path);
    return TRUE;
}

static void load_initial_wallpaper(void) {
    const char *home = g_get_home_dir();
    char conf_path[512];
    g_snprintf(conf_path, sizeof(conf_path), "%s/.blazeneuro-wallpaper", home);

    FILE *f = fopen(conf_path, "r");
    if (f) {
        char path[1024] = {0};
        if (fgets(path, sizeof(path), f)) {
            path[strcspn(path, "\n")] = '\0';
            if (apply_wallpaper_file(path)) {
                fclose(f);
                return;
            }
        }
        fclose(f);
    }

    apply_wallpaper_file("/usr/local/share/blazeneuro/assets/wallpaper.png");
}

/* ── Launch Helpers ─────────────────────────────────────── */
static void launch_cmd(const char *cmd) {
    gchar *argv[] = { "sh", "-c", (gchar *)cmd, NULL };
    g_spawn_async(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, NULL);
}

static void open_terminal(GtkWidget *w, gpointer d) { (void)w; (void)d; launch_cmd("blazeneuro-terminal"); }
static void open_files(GtkWidget *w, gpointer d) { (void)w; (void)d; launch_cmd("blazeneuro-files"); }
static void open_settings(GtkWidget *w, gpointer d) { (void)w; (void)d; launch_cmd("blazeneuro-settings"); }
static void open_calculator(GtkWidget *w, gpointer d) { (void)w; (void)d; launch_cmd("blazeneuro-calculator"); }
static void open_notes(GtkWidget *w, gpointer d) { (void)w; (void)d; launch_cmd("blazeneuro-notes"); }
static void open_tasks(GtkWidget *w, gpointer d) { (void)w; (void)d; launch_cmd("blazeneuro-taskviewer"); }
static void open_browser(GtkWidget *w, gpointer d) { (void)w; (void)d; launch_cmd("chromium || chromium-browser || firefox"); }
static void open_launcher(GtkWidget *w, gpointer d) { (void)w; (void)d; launch_cmd("blazeneuro-launcher"); }

static void choose_wallpaper(GtkWidget *widget, gpointer data) {
    (void)widget; (void)data;

    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        "Choose Wallpaper",
        GTK_WINDOW(desktop_win),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "Cancel", GTK_RESPONSE_CANCEL,
        "Set Wallpaper", GTK_RESPONSE_ACCEPT,
        NULL);

    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Images");
    gtk_file_filter_add_mime_type(filter, "image/png");
    gtk_file_filter_add_mime_type(filter, "image/jpeg");
    gtk_file_filter_add_mime_type(filter, "image/svg+xml");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        if (filename) {
            apply_wallpaper_file(filename);
            g_free(filename);
        }
    }
    gtk_widget_destroy(dialog);
}

/* ── Menu Item Helper ───────────────────────────────────── */
static GtkWidget *make_menu_item(const char *icon_name, const char *label,
                                  GCallback callback) {
    GtkWidget *item = gtk_menu_item_new();
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);

    if (icon_name) {
        GtkWidget *icon = gtk_image_new_from_icon_name(icon_name, GTK_ICON_SIZE_MENU);
        gtk_box_pack_start(GTK_BOX(hbox), icon, FALSE, FALSE, 0);
    }

    GtkWidget *lbl = gtk_label_new(label);
    gtk_label_set_xalign(GTK_LABEL(lbl), 0);
    gtk_box_pack_start(GTK_BOX(hbox), lbl, TRUE, TRUE, 0);

    gtk_container_add(GTK_CONTAINER(item), hbox);

    if (callback) {
        g_signal_connect(item, "activate", callback, NULL);
    }
    return item;
}

static GtkWidget *make_menu_item_with_shortcut(const char *icon_name,
                                                const char *label,
                                                const char *shortcut,
                                                GCallback callback) {
    GtkWidget *item = gtk_menu_item_new();
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);

    if (icon_name) {
        GtkWidget *icon = gtk_image_new_from_icon_name(icon_name, GTK_ICON_SIZE_MENU);
        gtk_box_pack_start(GTK_BOX(hbox), icon, FALSE, FALSE, 0);
    }

    GtkWidget *lbl = gtk_label_new(label);
    gtk_label_set_xalign(GTK_LABEL(lbl), 0);
    gtk_box_pack_start(GTK_BOX(hbox), lbl, TRUE, TRUE, 0);

    GtkWidget *sc = gtk_label_new(shortcut);
    gtk_style_context_add_class(gtk_widget_get_style_context(sc), "muted");
    gtk_box_pack_end(GTK_BOX(hbox), sc, FALSE, FALSE, 0);

    gtk_container_add(GTK_CONTAINER(item), hbox);

    if (callback) {
        g_signal_connect(item, "activate", callback, NULL);
    }
    return item;
}

/* ── Desktop Context Menu ───────────────────────────────── */
static gboolean on_button_press(GtkWidget *widget, GdkEventButton *ev, gpointer data) {
    (void)widget;
    (void)data;

    if (ev->type == GDK_BUTTON_PRESS && ev->button == 3) {
        GtkWidget *menu = gtk_menu_new();

        /* ── Applications submenu ──────────────────────── */
        GtkWidget *apps_item = make_menu_item("system-run", "Applications", NULL);
        GtkWidget *apps_sub = gtk_menu_new();
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(apps_item), apps_sub);

        gtk_menu_shell_append(GTK_MENU_SHELL(apps_sub),
            make_menu_item("utilities-terminal", "Terminal", G_CALLBACK(open_terminal)));
        gtk_menu_shell_append(GTK_MENU_SHELL(apps_sub),
            make_menu_item("system-file-manager", "Files", G_CALLBACK(open_files)));
        gtk_menu_shell_append(GTK_MENU_SHELL(apps_sub),
            make_menu_item("accessories-calculator", "Calculator", G_CALLBACK(open_calculator)));
        gtk_menu_shell_append(GTK_MENU_SHELL(apps_sub),
            make_menu_item("accessories-text-editor", "Notes", G_CALLBACK(open_notes)));
        gtk_menu_shell_append(GTK_MENU_SHELL(apps_sub),
            make_menu_item("web-browser", "Browser", G_CALLBACK(open_browser)));
        gtk_menu_shell_append(GTK_MENU_SHELL(apps_sub),
            make_menu_item("utilities-system-monitor", "Task Viewer", G_CALLBACK(open_tasks)));

        gtk_menu_shell_append(GTK_MENU_SHELL(menu), apps_item);

        /* ── Separator ─────────────────────────────────── */
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

        /* ── Quick launch items ─────────────────────────── */
        gtk_menu_shell_append(GTK_MENU_SHELL(menu),
            make_menu_item_with_shortcut("utilities-terminal", "Open Terminal",
                                         "Alt+Enter", G_CALLBACK(open_terminal)));
        gtk_menu_shell_append(GTK_MENU_SHELL(menu),
            make_menu_item_with_shortcut("system-file-manager", "Open Files",
                                         "Super+E", G_CALLBACK(open_files)));
        gtk_menu_shell_append(GTK_MENU_SHELL(menu),
            make_menu_item_with_shortcut("system-search", "App Launcher",
                                         "Alt+Space", G_CALLBACK(open_launcher)));

        /* ── Separator ─────────────────────────────────── */
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

        /* ── Desktop actions ────────────────────────────── */
        gtk_menu_shell_append(GTK_MENU_SHELL(menu),
            make_menu_item("preferences-desktop-wallpaper", "Change Wallpaper…",
                           G_CALLBACK(choose_wallpaper)));
        gtk_menu_shell_append(GTK_MENU_SHELL(menu),
            make_menu_item("preferences-system", "Settings",
                           G_CALLBACK(open_settings)));

        gtk_widget_show_all(menu);
        gtk_menu_popup_at_pointer(GTK_MENU(menu), (GdkEvent *)ev);
        return TRUE;
    }

    return FALSE;
}

/* ── Main ───────────────────────────────────────────────── */
int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    blazeneuro_load_theme();

    desktop_win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(desktop_win), "BlazeNeuro Desktop");
    gtk_window_set_type_hint(GTK_WINDOW(desktop_win), GDK_WINDOW_TYPE_HINT_DESKTOP);
    gtk_window_fullscreen(GTK_WINDOW(desktop_win));
    gtk_window_set_decorated(GTK_WINDOW(desktop_win), FALSE);
    gtk_window_set_resizable(GTK_WINDOW(desktop_win), FALSE);
    gtk_widget_add_events(desktop_win, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(desktop_win, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(desktop_win, "button-press-event", G_CALLBACK(on_button_press), NULL);

    desktop_image = gtk_image_new();
    gtk_container_add(GTK_CONTAINER(desktop_win), desktop_image);

    load_initial_wallpaper();

    gtk_widget_show_all(desktop_win);
    gtk_main();
    return 0;
}
