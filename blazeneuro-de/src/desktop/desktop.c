/*
 * BlazeNeuro Desktop View
 * Provides desktop wallpaper rendering + right-click context menu.
 */

#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static GtkWidget *desktop_win;
static GtkWidget *desktop_image;

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

    gint screen_w = gdk_screen_width();
    gint screen_h = gdk_screen_height();
    GdkPixbuf *scaled = gdk_pixbuf_scale_simple(
        pixbuf, screen_w, screen_h, GDK_INTERP_BILINEAR);

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

static void choose_wallpaper(GtkWidget *widget, gpointer data) {
    (void)widget;
    (void)data;

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
        apply_wallpaper_file(filename);
        g_free(filename);
    }
    gtk_widget_destroy(dialog);
}

static void launch_cmd(const char *cmd) {
    gchar *argv[] = { "sh", "-c", (gchar *)cmd, NULL };
    g_spawn_async(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, NULL);
}

static void open_terminal(GtkWidget *widget, gpointer data) {
    (void)widget; (void)data;
    launch_cmd("blazeneuro-terminal");
}

static void open_files(GtkWidget *widget, gpointer data) {
    (void)widget; (void)data;
    launch_cmd("blazeneuro-files");
}

static void open_settings(GtkWidget *widget, gpointer data) {
    (void)widget; (void)data;
    launch_cmd("blazeneuro-settings");
}

static gboolean on_button_press(GtkWidget *widget, GdkEventButton *ev, gpointer data) {
    (void)widget;
    (void)data;

    if (ev->type == GDK_BUTTON_PRESS && ev->button == 3) {
        GtkWidget *menu = gtk_menu_new();

        GtkWidget *files_item = gtk_menu_item_new_with_label("Open Files");
        GtkWidget *terminal_item = gtk_menu_item_new_with_label("Open Terminal");
        GtkWidget *settings_item = gtk_menu_item_new_with_label("Open Settings");
        GtkWidget *sep = gtk_separator_menu_item_new();
        GtkWidget *wallpaper_item = gtk_menu_item_new_with_label("Change Wallpaper...");

        g_signal_connect(files_item, "activate", G_CALLBACK(open_files), NULL);
        g_signal_connect(terminal_item, "activate", G_CALLBACK(open_terminal), NULL);
        g_signal_connect(settings_item, "activate", G_CALLBACK(open_settings), NULL);
        g_signal_connect(wallpaper_item, "activate", G_CALLBACK(choose_wallpaper), NULL);

        gtk_menu_shell_append(GTK_MENU_SHELL(menu), files_item);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), terminal_item);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), settings_item);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), sep);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), wallpaper_item);

        gtk_widget_show_all(menu);
        gtk_menu_popup_at_pointer(GTK_MENU(menu), (GdkEvent *)ev);
        return TRUE;
    }

    return FALSE;
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

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
