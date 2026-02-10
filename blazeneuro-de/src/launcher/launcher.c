/*
 * BlazeNeuro Launcher
 * Spotlight-like application launcher overlay.
 * Press Alt+Space to open, type to search, Enter to launch.
 */

#include <gtk/gtk.h>
#include <gio/gio.h>
#include <string.h>
#include <stdlib.h>

/* ── App Entry ──────────────────────────────────────────── */
typedef struct {
    char *name;
    char *exec;
    char *icon;
} AppEntry;

static GList *all_apps = NULL;
static GtkWidget *results_box;
static GtkWidget *search_entry;

/* ── Load Desktop Entries ───────────────────────────────── */
static void load_apps(void) {
    GList *apps = g_app_info_get_all();
    for (GList *l = apps; l != NULL; l = l->next) {
        GAppInfo *info = G_APP_INFO(l->data);
        if (!g_app_info_should_show(info)) continue;

        AppEntry *entry = g_malloc0(sizeof(AppEntry));
        entry->name = g_strdup(g_app_info_get_display_name(info));
        entry->exec = g_strdup(g_app_info_get_id(info));

        GIcon *gicon = g_app_info_get_icon(info);
        if (gicon) {
            gchar *icon_str = g_icon_to_string(gicon);
            entry->icon = icon_str ? icon_str : g_strdup("application-x-executable");
        } else {
            entry->icon = g_strdup("application-x-executable");
        }

        all_apps = g_list_append(all_apps, entry);
    }
    g_list_free_full(apps, g_object_unref);
}

/* ── Create Result Row ──────────────────────────────────── */
static GtkWidget *create_result(AppEntry *entry) {
    GtkWidget *btn = gtk_button_new();
    gtk_style_context_add_class(gtk_widget_get_style_context(btn), "result-btn");

    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 4);

    GtkWidget *icon = gtk_image_new_from_icon_name(entry->icon, GTK_ICON_SIZE_LARGE_TOOLBAR);
    gtk_image_set_pixel_size(GTK_IMAGE(icon), 32);
    gtk_box_pack_start(GTK_BOX(hbox), icon, FALSE, FALSE, 0);

    GtkWidget *label = gtk_label_new(entry->name);
    gtk_label_set_xalign(GTK_LABEL(label), 0);
    gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 0);

    gtk_container_add(GTK_CONTAINER(btn), hbox);

    g_object_set_data(G_OBJECT(btn), "app-id", entry->exec);
    return btn;
}

/* ── Launch Selected App ────────────────────────────────── */
static void launch_result(GtkWidget *btn, gpointer data) {
    (void)data;
    const char *app_id = g_object_get_data(G_OBJECT(btn), "app-id");
    if (app_id) {
        GDesktopAppInfo *info = g_desktop_app_info_new(app_id);
        if (info) {
            g_app_info_launch(G_APP_INFO(info), NULL, NULL, NULL);
            g_object_unref(info);
        }
    }
    gtk_main_quit();
}

/* ── Search Filter ──────────────────────────────────────── */
static void on_search_changed(GtkEditable *editable, gpointer data) {
    (void)data;
    const char *query = gtk_entry_get_text(GTK_ENTRY(editable));

    /* Clear old results */
    GList *children = gtk_container_get_children(GTK_CONTAINER(results_box));
    for (GList *l = children; l; l = l->next)
        gtk_widget_destroy(GTK_WIDGET(l->data));
    g_list_free(children);

    if (strlen(query) == 0) return;

    char *query_lower = g_utf8_strdown(query, -1);
    int count = 0;

    for (GList *l = all_apps; l && count < 8; l = l->next) {
        AppEntry *entry = l->data;
        char *name_lower = g_utf8_strdown(entry->name, -1);

        if (strstr(name_lower, query_lower)) {
            GtkWidget *row = create_result(entry);
            g_signal_connect(row, "clicked", G_CALLBACK(launch_result), NULL);
            gtk_box_pack_start(GTK_BOX(results_box), row, FALSE, FALSE, 0);
            count++;
        }
        g_free(name_lower);
    }
    g_free(query_lower);

    gtk_widget_show_all(results_box);
}

/* ── Key Handler ────────────────────────────────────────── */
static gboolean on_key_press(GtkWidget *widget, GdkEventKey *ev, gpointer data) {
    (void)widget; (void)data;
    if (ev->keyval == GDK_KEY_Escape) {
        gtk_main_quit();
        return TRUE;
    }
    if (ev->keyval == GDK_KEY_Return) {
        /* Launch first result */
        GList *children = gtk_container_get_children(GTK_CONTAINER(results_box));
        if (children) {
            launch_result(GTK_WIDGET(children->data), NULL);
            g_list_free(children);
            return TRUE;
        }
        g_list_free(children);
    }
    return FALSE;
}

/* ── CSS ────────────────────────────────────────────────── */
static void apply_css(void) {
    GtkCssProvider *css = gtk_css_provider_new();
    const char *style =
        "window {"
        "  background-color: rgba(10, 10, 14, 0.85);"
        "}"
        ".search-box {"
        "  background-color: rgba(24, 24, 28, 0.95);"
        "  border: 1px solid rgba(255, 255, 255, 0.1);"
        "  border-radius: 12px;"
        "  padding: 12px 16px;"
        "  color: #fafafa;"
        "  font-family: 'Inter', sans-serif;"
        "  font-size: 16px;"
        "  caret-color: #3b82f6;"
        "}"
        ".search-box:focus {"
        "  border-color: rgba(59, 130, 246, 0.5);"
        "}"
        ".result-btn {"
        "  background-color: transparent;"
        "  border: none;"
        "  border-radius: 8px;"
        "  color: #e4e4e7;"
        "  padding: 6px 12px;"
        "  font-family: 'Inter', sans-serif;"
        "  font-size: 14px;"
        "}"
        ".result-btn:hover {"
        "  background-color: rgba(59, 130, 246, 0.15);"
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
    load_apps();

    /* Overlay window */
    GtkWidget *win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(win), "Launcher");
    gtk_window_set_decorated(GTK_WINDOW(win), FALSE);
    gtk_window_set_position(GTK_WINDOW(win), GTK_WIN_POS_CENTER);
    gtk_window_set_default_size(GTK_WINDOW(win), 600, 60);
    gtk_window_set_resizable(GTK_WINDOW(win), FALSE);
    gtk_widget_set_app_paintable(win, TRUE);

    GdkScreen *scr = gtk_widget_get_screen(win);
    GdkVisual *vis = gdk_screen_get_rgba_visual(scr);
    if (vis) gtk_widget_set_visual(win, vis);

    g_signal_connect(win, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(win, "key-press-event", G_CALLBACK(on_key_press), NULL);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 16);
    gtk_container_add(GTK_CONTAINER(win), vbox);

    /* Search entry */
    search_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(search_entry), "Search applications...");
    gtk_style_context_add_class(gtk_widget_get_style_context(search_entry), "search-box");
    g_signal_connect(search_entry, "changed", G_CALLBACK(on_search_changed), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), search_entry, FALSE, FALSE, 0);

    /* Results */
    results_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_box_pack_start(GTK_BOX(vbox), results_box, TRUE, TRUE, 0);

    gtk_widget_show_all(win);
    gtk_widget_grab_focus(search_entry);
    gtk_main();

    return 0;
}
