/*
 * BlazeNeuro Files
 * A modern file manager using GTK3.
 * Sidebar + icon/list view with breadcrumb navigation.
 */

#include <gtk/gtk.h>
#include <gio/gio.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* ── Globals ────────────────────────────────────────────── */
static GtkWidget *icon_view;
static GtkWidget *path_label;
static GtkWidget *sidebar_list;
static GtkListStore *file_store;
static char current_path[4096];

enum {
    COL_ICON,
    COL_NAME,
    COL_PATH,
    COL_IS_DIR,
    NUM_COLS
};

/* ── Populate Files ─────────────────────────────────────── */
static void populate_files(const char *path) {
    gtk_list_store_clear(file_store);
    g_strlcpy(current_path, path, sizeof(current_path));
    gtk_label_set_text(GTK_LABEL(path_label), current_path);

    GDir *dir = g_dir_open(path, 0, NULL);
    if (!dir) return;

    const gchar *name;
    while ((name = g_dir_read_name(dir)) != NULL) {
        if (name[0] == '.') continue; /* hide dotfiles */

        gchar *full = g_build_filename(path, name, NULL);
        gboolean is_dir = g_file_test(full, G_FILE_TEST_IS_DIR);

        const char *icon_name = is_dir ? "folder" : "text-x-generic";

        /* Try to get a better icon based on extension */
        if (!is_dir) {
            gchar *name_lower = g_utf8_strdown(name, -1);
            if (g_str_has_suffix(name_lower, ".png") || g_str_has_suffix(name_lower, ".jpg") ||
                g_str_has_suffix(name_lower, ".jpeg") || g_str_has_suffix(name_lower, ".svg"))
                icon_name = "image-x-generic";
            else if (g_str_has_suffix(name_lower, ".mp3") || g_str_has_suffix(name_lower, ".wav") ||
                     g_str_has_suffix(name_lower, ".flac"))
                icon_name = "audio-x-generic";
            else if (g_str_has_suffix(name_lower, ".mp4") || g_str_has_suffix(name_lower, ".mkv"))
                icon_name = "video-x-generic";
            else if (g_str_has_suffix(name_lower, ".pdf"))
                icon_name = "application-pdf";
            else if (g_str_has_suffix(name_lower, ".c") || g_str_has_suffix(name_lower, ".py") ||
                     g_str_has_suffix(name_lower, ".js") || g_str_has_suffix(name_lower, ".h"))
                icon_name = "text-x-script";
            else if (g_str_has_suffix(name_lower, ".zip") || g_str_has_suffix(name_lower, ".tar") ||
                     g_str_has_suffix(name_lower, ".gz"))
                icon_name = "package-x-generic";
            g_free(name_lower);
        }

        GtkTreeIter iter;
        gtk_list_store_append(file_store, &iter);
        gtk_list_store_set(file_store, &iter,
                           COL_ICON, icon_name,
                           COL_NAME, name,
                           COL_PATH, full,
                           COL_IS_DIR, is_dir,
                           -1);
        g_free(full);
    }
    g_dir_close(dir);
}

/* ── Navigation ─────────────────────────────────────────── */
static void on_item_activated(GtkIconView *view, GtkTreePath *tree_path, gpointer data) {
    (void)data;
    GtkTreeModel *model = gtk_icon_view_get_model(view);
    GtkTreeIter iter;

    if (gtk_tree_model_get_iter(model, &iter, tree_path)) {
        gboolean is_dir;
        gchar *path;
        gtk_tree_model_get(model, &iter, COL_PATH, &path, COL_IS_DIR, &is_dir, -1);

        if (is_dir) {
            populate_files(path);
        } else {
            /* Open file with default app */
            gchar *uri = g_filename_to_uri(path, NULL, NULL);
            if (uri) {
                g_app_info_launch_default_for_uri(uri, NULL, NULL);
                g_free(uri);
            }
        }
        g_free(path);
    }
}

static void go_up(GtkWidget *widget, gpointer data) {
    (void)widget; (void)data;
    char *parent = g_path_get_dirname(current_path);
    populate_files(parent);
    g_free(parent);
}

static void go_home(GtkWidget *widget, gpointer data) {
    (void)widget; (void)data;
    const char *home = g_get_home_dir();
    populate_files(home);
}

/* ── Sidebar Navigation ────────────────────────────────── */
static void on_sidebar_select(GtkListBox *box, GtkListBoxRow *row, gpointer data) {
    (void)box; (void)data;
    if (!row) return;
    const char *path = g_object_get_data(G_OBJECT(row), "path");
    if (path) populate_files(path);
}

static GtkWidget *create_sidebar_row(const char *label, const char *icon_name, const char *path) {
    GtkWidget *row = gtk_list_box_row_new();
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 6);

    GtkWidget *icon = gtk_image_new_from_icon_name(icon_name, GTK_ICON_SIZE_MENU);
    GtkWidget *lbl = gtk_label_new(label);
    gtk_label_set_xalign(GTK_LABEL(lbl), 0);

    gtk_box_pack_start(GTK_BOX(hbox), icon, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), lbl, TRUE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(row), hbox);

    g_object_set_data_full(G_OBJECT(row), "path", g_strdup(path), g_free);
    return row;
}

/* ── CSS ────────────────────────────────────────────────── */
static void apply_css(void) {
    GtkCssProvider *css = gtk_css_provider_new();
    const char *style =
        "window {"
        "  background-color: rgba(14, 14, 18, 0.92);"
        "}"
        ".sidebar {"
        "  background-color: rgba(18, 18, 22, 0.95);"
        "  border-right: 1px solid rgba(255, 255, 255, 0.06);"
        "  padding: 8px 0;"
        "}"
        ".sidebar row {"
        "  color: #a1a1aa;"
        "  border-radius: 8px;"
        "  margin: 1px 6px;"
        "  padding: 4px 8px;"
        "}"
        ".sidebar row:selected {"
        "  background-color: rgba(59, 130, 246, 0.2);"
        "  color: #fafafa;"
        "}"
        ".sidebar row:hover {"
        "  background-color: rgba(255, 255, 255, 0.05);"
        "}"
        ".pathbar {"
        "  background-color: rgba(24, 24, 28, 0.9);"
        "  border-bottom: 1px solid rgba(255, 255, 255, 0.06);"
        "  padding: 8px 16px;"
        "  color: #a1a1aa;"
        "  font-family: 'Inter', sans-serif;"
        "  font-size: 13px;"
        "}"
        ".content-area {"
        "  background-color: transparent;"
        "}"
        "iconview {"
        "  background-color: transparent;"
        "  color: #e4e4e7;"
        "  font-family: 'Inter', sans-serif;"
        "  font-size: 12px;"
        "}"
        "iconview:selected {"
        "  background-color: rgba(59, 130, 246, 0.3);"
        "  border-radius: 8px;"
        "}"
        "button {"
        "  background-color: rgba(39, 39, 42, 0.8);"
        "  color: #fafafa;"
        "  border: 1px solid rgba(255, 255, 255, 0.1);"
        "  border-radius: 6px;"
        "  padding: 4px 12px;"
        "  font-family: 'Inter', sans-serif;"
        "  min-height: 28px;"
        "}"
        "button:hover {"
        "  background-color: rgba(63, 63, 70, 0.9);"
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
    gtk_window_set_title(GTK_WINDOW(win), "Files");
    gtk_window_set_default_size(GTK_WINDOW(win), 900, 600);
    gtk_widget_set_app_paintable(win, TRUE);

    GdkScreen *scr = gtk_widget_get_screen(win);
    GdkVisual *vis = gdk_screen_get_rgba_visual(scr);
    if (vis) gtk_widget_set_visual(win, vis);

    g_signal_connect(win, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    /* Main layout */
    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_container_add(GTK_CONTAINER(win), main_box);

    /* Sidebar */
    GtkWidget *sidebar_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sidebar_scroll),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(sidebar_scroll, 200, -1);
    gtk_style_context_add_class(gtk_widget_get_style_context(sidebar_scroll), "sidebar");

    sidebar_list = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(sidebar_list), GTK_SELECTION_SINGLE);
    g_signal_connect(sidebar_list, "row-selected", G_CALLBACK(on_sidebar_select), NULL);

    const char *home = g_get_home_dir();
    gchar *docs = g_build_filename(home, "Documents", NULL);
    gchar *down = g_build_filename(home, "Downloads", NULL);
    gchar *pics = g_build_filename(home, "Pictures", NULL);
    gchar *music = g_build_filename(home, "Music", NULL);
    gchar *videos = g_build_filename(home, "Videos", NULL);

    gtk_container_add(GTK_CONTAINER(sidebar_list), create_sidebar_row("Home", "go-home", home));
    gtk_container_add(GTK_CONTAINER(sidebar_list), create_sidebar_row("Documents", "folder-documents", docs));
    gtk_container_add(GTK_CONTAINER(sidebar_list), create_sidebar_row("Downloads", "folder-download", down));
    gtk_container_add(GTK_CONTAINER(sidebar_list), create_sidebar_row("Pictures", "folder-pictures", pics));
    gtk_container_add(GTK_CONTAINER(sidebar_list), create_sidebar_row("Music", "folder-music", music));
    gtk_container_add(GTK_CONTAINER(sidebar_list), create_sidebar_row("Videos", "folder-videos", videos));
    gtk_container_add(GTK_CONTAINER(sidebar_list), create_sidebar_row("Root", "drive-harddisk", "/"));

    g_free(docs); g_free(down); g_free(pics); g_free(music); g_free(videos);

    gtk_container_add(GTK_CONTAINER(sidebar_scroll), sidebar_list);
    gtk_box_pack_start(GTK_BOX(main_box), sidebar_scroll, FALSE, FALSE, 0);

    /* Right panel */
    GtkWidget *right_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(main_box), right_box, TRUE, TRUE, 0);

    /* Path bar */
    GtkWidget *pathbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_style_context_add_class(gtk_widget_get_style_context(pathbar), "pathbar");

    GtkWidget *up_btn = gtk_button_new_with_label("↑");
    g_signal_connect(up_btn, "clicked", G_CALLBACK(go_up), NULL);
    gtk_box_pack_start(GTK_BOX(pathbar), up_btn, FALSE, FALSE, 0);

    GtkWidget *home_btn = gtk_button_new_with_label("⌂");
    g_signal_connect(home_btn, "clicked", G_CALLBACK(go_home), NULL);
    gtk_box_pack_start(GTK_BOX(pathbar), home_btn, FALSE, FALSE, 0);

    path_label = gtk_label_new("");
    gtk_label_set_xalign(GTK_LABEL(path_label), 0);
    gtk_label_set_ellipsize(GTK_LABEL(path_label), PANGO_ELLIPSIZE_START);
    gtk_box_pack_start(GTK_BOX(pathbar), path_label, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(right_box), pathbar, FALSE, FALSE, 0);

    /* Icon view */
    file_store = gtk_list_store_new(NUM_COLS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_BOOLEAN);

    icon_view = gtk_icon_view_new_with_model(GTK_TREE_MODEL(file_store));
    gtk_icon_view_set_text_column(GTK_ICON_VIEW(icon_view), COL_NAME);
    gtk_icon_view_set_pixbuf_column(GTK_ICON_VIEW(icon_view), -1);
    gtk_icon_view_set_item_width(GTK_ICON_VIEW(icon_view), 90);
    gtk_icon_view_set_columns(GTK_ICON_VIEW(icon_view), -1);
    gtk_icon_view_set_spacing(GTK_ICON_VIEW(icon_view), 4);
    gtk_icon_view_set_margin(GTK_ICON_VIEW(icon_view), 12);

    /* Use a cell renderer for the icon name */
    GtkCellRenderer *pix_renderer = gtk_cell_renderer_pixbuf_new();
    g_object_set(pix_renderer, "stock-size", GTK_ICON_SIZE_DIALOG, NULL);
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(icon_view), pix_renderer, FALSE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(icon_view), pix_renderer,
                                   "icon-name", COL_ICON, NULL);

    g_signal_connect(icon_view, "item-activated", G_CALLBACK(on_item_activated), NULL);
    gtk_style_context_add_class(gtk_widget_get_style_context(icon_view), "content-area");

    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scroll), icon_view);
    gtk_box_pack_start(GTK_BOX(right_box), scroll, TRUE, TRUE, 0);

    /* Initial path */
    populate_files(home);

    gtk_widget_show_all(win);
    gtk_main();

    return 0;
}
