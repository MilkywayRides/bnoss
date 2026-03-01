/*
 * BlazeNeuro Files
 * Modern file manager with sidebar, icon view, and context menus.
 */

#include <gtk/gtk.h>
#include <gio/gio.h>
#include <glib/gstdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>

#include "../common/theme.h"
#include "../common/titlebar.h"

/* ── Globals ────────────────────────────────────────────── */
static GtkWidget *icon_view;
static GtkWidget *path_label;
static GtkWidget *sidebar_list;
static GtkWidget *main_window;
static GtkListStore *file_store;
static char current_path[4096];
static gboolean show_hidden = FALSE;

enum {
    COL_ICON,
    COL_NAME,
    COL_PATH,
    COL_IS_DIR,
    NUM_COLS
};

/* ── Forward declarations ───────────────────────────────── */
static void populate_files(const char *path);

/* ── Populate Files ─────────────────────────────────────── */
static void populate_files(const char *path) {
    gtk_list_store_clear(file_store);
    g_strlcpy(current_path, path, sizeof(current_path));
    gtk_label_set_text(GTK_LABEL(path_label), current_path);

    GDir *dir = g_dir_open(path, 0, NULL);
    if (!dir) return;

    const gchar *name;
    while ((name = g_dir_read_name(dir)) != NULL) {
        if (!show_hidden && name[0] == '.') continue;

        gchar *full = g_build_filename(path, name, NULL);
        gboolean is_dir = g_file_test(full, G_FILE_TEST_IS_DIR);

        const char *icon_name = is_dir ? "folder" : "text-x-generic";

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
            else if (g_str_has_suffix(name_lower, ".deb"))
                icon_name = "application-x-deb";
            else if (g_str_has_suffix(name_lower, ".sh"))
                icon_name = "text-x-script";
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
    populate_files(g_get_home_dir());
}

/* ── Context Menu Helpers ───────────────────────────────── */
static gchar *get_selected_path(void) {
    GList *selected = gtk_icon_view_get_selected_items(GTK_ICON_VIEW(icon_view));
    if (!selected) return NULL;

    GtkTreePath *tree_path = (GtkTreePath *)selected->data;
    GtkTreeModel *model = gtk_icon_view_get_model(GTK_ICON_VIEW(icon_view));
    GtkTreeIter iter;
    gchar *path = NULL;

    if (gtk_tree_model_get_iter(model, &iter, tree_path)) {
        gtk_tree_model_get(model, &iter, COL_PATH, &path, -1);
    }

    g_list_free_full(selected, (GDestroyNotify)gtk_tree_path_free);
    return path;
}

static gboolean get_selected_is_dir(void) {
    GList *selected = gtk_icon_view_get_selected_items(GTK_ICON_VIEW(icon_view));
    if (!selected) return FALSE;

    GtkTreePath *tree_path = (GtkTreePath *)selected->data;
    GtkTreeModel *model = gtk_icon_view_get_model(GTK_ICON_VIEW(icon_view));
    GtkTreeIter iter;
    gboolean is_dir = FALSE;

    if (gtk_tree_model_get_iter(model, &iter, tree_path)) {
        gtk_tree_model_get(model, &iter, COL_IS_DIR, &is_dir, -1);
    }

    g_list_free_full(selected, (GDestroyNotify)gtk_tree_path_free);
    return is_dir;
}

/* ── Context Menu Actions ───────────────────────────────── */
static void ctx_open(GtkWidget *w, gpointer d) {
    (void)w; (void)d;
    gchar *path = get_selected_path();
    if (!path) return;
    if (get_selected_is_dir()) {
        populate_files(path);
    } else {
        gchar *uri = g_filename_to_uri(path, NULL, NULL);
        if (uri) { g_app_info_launch_default_for_uri(uri, NULL, NULL); g_free(uri); }
    }
    g_free(path);
}

static void ctx_open_in_terminal(GtkWidget *w, gpointer d) {
    (void)w; (void)d;
    gchar *path = get_selected_path();
    const char *dir = path ? path : current_path;
    gchar *target = get_selected_is_dir() ? g_strdup(dir) : g_path_get_dirname(dir);
    gchar *cmd = g_strdup_printf("cd '%s' && blazeneuro-terminal", target);
    g_spawn_command_line_async(cmd, NULL);
    g_free(cmd); g_free(target);
    if (path) g_free(path);
}

static void ctx_copy_path(GtkWidget *w, gpointer d) {
    (void)w; (void)d;
    gchar *path = get_selected_path();
    if (path) {
        GtkClipboard *clip = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
        gtk_clipboard_set_text(clip, path, -1);
        g_free(path);
    }
}

static void ctx_rename(GtkWidget *w, gpointer d) {
    (void)w; (void)d;
    gchar *path = get_selected_path();
    if (!path) return;

    gchar *basename = g_path_get_basename(path);
    gchar *dirname = g_path_get_dirname(path);

    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Rename", GTK_WINDOW(main_window), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "Cancel", GTK_RESPONSE_CANCEL,
        "Rename", GTK_RESPONSE_ACCEPT, NULL);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(content), 12);

    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry), basename);
    gtk_box_pack_start(GTK_BOX(content), entry, FALSE, FALSE, 8);
    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        const char *new_name = gtk_entry_get_text(GTK_ENTRY(entry));
        if (new_name && new_name[0]) {
            gchar *new_path = g_build_filename(dirname, new_name, NULL);
            g_rename(path, new_path);
            g_free(new_path);
            populate_files(current_path);
        }
    }
    gtk_widget_destroy(dialog);
    g_free(basename); g_free(dirname); g_free(path);
}

static void ctx_delete(GtkWidget *w, gpointer d) {
    (void)w; (void)d;
    gchar *path = get_selected_path();
    if (!path) return;

    gchar *basename = g_path_get_basename(path);
    gchar *msg = g_strdup_printf("Delete \"%s\"?", basename);

    GtkWidget *dialog = gtk_message_dialog_new(
        GTK_WINDOW(main_window), GTK_DIALOG_MODAL,
        GTK_MESSAGE_WARNING, GTK_BUTTONS_NONE, "%s", msg);
    gtk_dialog_add_buttons(GTK_DIALOG(dialog),
                           "Cancel", GTK_RESPONSE_CANCEL,
                           "Delete", GTK_RESPONSE_ACCEPT, NULL);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        if (get_selected_is_dir()) {
            gchar *cmd = g_strdup_printf("rm -rf '%s'", path);
            g_spawn_command_line_async(cmd, NULL);
            g_free(cmd);
        } else {
            g_unlink(path);
        }
        populate_files(current_path);
    }
    gtk_widget_destroy(dialog);
    g_free(msg); g_free(basename); g_free(path);
}

static void ctx_new_folder(GtkWidget *w, gpointer d) {
    (void)w; (void)d;

    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "New Folder", GTK_WINDOW(main_window), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "Cancel", GTK_RESPONSE_CANCEL,
        "Create", GTK_RESPONSE_ACCEPT, NULL);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(content), 12);

    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry), "New Folder");
    gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1);
    gtk_box_pack_start(GTK_BOX(content), entry, FALSE, FALSE, 8);
    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        const char *name = gtk_entry_get_text(GTK_ENTRY(entry));
        if (name && name[0]) {
            gchar *new_dir = g_build_filename(current_path, name, NULL);
            g_mkdir_with_parents(new_dir, 0755);
            g_free(new_dir);
            populate_files(current_path);
        }
    }
    gtk_widget_destroy(dialog);
}

static void ctx_new_file(GtkWidget *w, gpointer d) {
    (void)w; (void)d;

    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "New File", GTK_WINDOW(main_window), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "Cancel", GTK_RESPONSE_CANCEL,
        "Create", GTK_RESPONSE_ACCEPT, NULL);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(content), 12);

    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry), "untitled.txt");
    gtk_editable_select_region(GTK_EDITABLE(entry), 0, 8);
    gtk_box_pack_start(GTK_BOX(content), entry, FALSE, FALSE, 8);
    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        const char *name = gtk_entry_get_text(GTK_ENTRY(entry));
        if (name && name[0]) {
            gchar *new_file = g_build_filename(current_path, name, NULL);
            FILE *f = fopen(new_file, "w");
            if (f) fclose(f);
            g_free(new_file);
            populate_files(current_path);
        }
    }
    gtk_widget_destroy(dialog);
}

static void ctx_toggle_hidden(GtkWidget *w, gpointer d) {
    (void)w; (void)d;
    show_hidden = !show_hidden;
    populate_files(current_path);
}

static void ctx_refresh(GtkWidget *w, gpointer d) {
    (void)w; (void)d;
    populate_files(current_path);
}

static void ctx_properties(GtkWidget *w, gpointer d) {
    (void)w; (void)d;
    gchar *path = get_selected_path();
    if (!path) return;

    struct stat st;
    if (stat(path, &st) != 0) { g_free(path); return; }

    gchar *basename = g_path_get_basename(path);
    gchar *size_str;
    if (st.st_size < 1024)
        size_str = g_strdup_printf("%ld B", (long)st.st_size);
    else if (st.st_size < 1024 * 1024)
        size_str = g_strdup_printf("%.1f KB", st.st_size / 1024.0);
    else
        size_str = g_strdup_printf("%.1f MB", st.st_size / (1024.0 * 1024.0));

    gchar *info = g_strdup_printf(
        "Name: %s\nSize: %s\nType: %s\nPath: %s",
        basename, size_str,
        S_ISDIR(st.st_mode) ? "Directory" : "File",
        path);

    GtkWidget *dialog = gtk_message_dialog_new(
        GTK_WINDOW(main_window), GTK_DIALOG_MODAL,
        GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE, "%s", info);
    gtk_window_set_title(GTK_WINDOW(dialog), "Properties");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    g_free(info); g_free(size_str); g_free(basename); g_free(path);
}

/* ── Build Context Menu ─────────────────────────────────── */
static GtkWidget *make_ctx_item(const char *icon_name, const char *label, GCallback cb) {
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

    if (cb) g_signal_connect(item, "activate", cb, NULL);
    return item;
}

static gboolean on_button_press(GtkWidget *widget, GdkEventButton *ev, gpointer data) {
    (void)widget; (void)data;

    if (ev->type != GDK_BUTTON_PRESS || ev->button != 3) return FALSE;

    GtkWidget *menu = gtk_menu_new();
    gchar *sel_path = get_selected_path();

    if (sel_path) {
        /* Item-specific context menu */
        gtk_menu_shell_append(GTK_MENU_SHELL(menu),
            make_ctx_item("document-open", "Open", G_CALLBACK(ctx_open)));
        gtk_menu_shell_append(GTK_MENU_SHELL(menu),
            make_ctx_item("utilities-terminal", "Open in Terminal", G_CALLBACK(ctx_open_in_terminal)));
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
        gtk_menu_shell_append(GTK_MENU_SHELL(menu),
            make_ctx_item("edit-copy", "Copy Path", G_CALLBACK(ctx_copy_path)));
        gtk_menu_shell_append(GTK_MENU_SHELL(menu),
            make_ctx_item("gtk-edit", "Rename…", G_CALLBACK(ctx_rename)));
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
        gtk_menu_shell_append(GTK_MENU_SHELL(menu),
            make_ctx_item("dialog-information", "Properties", G_CALLBACK(ctx_properties)));
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
        gtk_menu_shell_append(GTK_MENU_SHELL(menu),
            make_ctx_item("edit-delete", "Delete", G_CALLBACK(ctx_delete)));

        g_free(sel_path);
    } else {
        /* Background context menu (no item selected) */
        gtk_menu_shell_append(GTK_MENU_SHELL(menu),
            make_ctx_item("folder-new", "New Folder…", G_CALLBACK(ctx_new_folder)));
        gtk_menu_shell_append(GTK_MENU_SHELL(menu),
            make_ctx_item("document-new", "New File…", G_CALLBACK(ctx_new_file)));
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
        gtk_menu_shell_append(GTK_MENU_SHELL(menu),
            make_ctx_item("utilities-terminal", "Open Terminal Here",
                          G_CALLBACK(ctx_open_in_terminal)));
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
        gtk_menu_shell_append(GTK_MENU_SHELL(menu),
            make_ctx_item("view-refresh", "Refresh", G_CALLBACK(ctx_refresh)));
        gtk_menu_shell_append(GTK_MENU_SHELL(menu),
            make_ctx_item(show_hidden ? "view-visible" : "view-hidden",
                          show_hidden ? "Hide Hidden Files" : "Show Hidden Files",
                          G_CALLBACK(ctx_toggle_hidden)));
    }

    gtk_widget_show_all(menu);
    gtk_menu_popup_at_pointer(GTK_MENU(menu), (GdkEvent *)ev);
    return TRUE;
}

/* ── Sidebar ────────────────────────────────────────────── */
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

/* ── Main ───────────────────────────────────────────────── */
int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    blazeneuro_load_theme();

    main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(main_window), "Files");
    gtk_window_set_default_size(GTK_WINDOW(main_window), 900, 600);
    gtk_widget_set_app_paintable(main_window, TRUE);

    GdkScreen *scr = gtk_widget_get_screen(main_window);
    GdkVisual *vis = gdk_screen_get_rgba_visual(scr);
    if (vis) gtk_widget_set_visual(main_window, vis);

    g_signal_connect(main_window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    /* Main layout */
    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

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
    gtk_style_context_add_class(gtk_widget_get_style_context(path_label), "muted");
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
    gtk_icon_view_set_selection_mode(GTK_ICON_VIEW(icon_view), GTK_SELECTION_SINGLE);

    /* Cell renderer for icons */
    GtkCellRenderer *pix_renderer = gtk_cell_renderer_pixbuf_new();
    g_object_set(pix_renderer, "stock-size", GTK_ICON_SIZE_DIALOG, NULL);
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(icon_view), pix_renderer, FALSE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(icon_view), pix_renderer,
                                   "icon-name", COL_ICON, NULL);

    g_signal_connect(icon_view, "item-activated", G_CALLBACK(on_item_activated), NULL);
    gtk_style_context_add_class(gtk_widget_get_style_context(icon_view), "content-area");

    /* Right-click context menu */
    gtk_widget_add_events(icon_view, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(icon_view, "button-press-event", G_CALLBACK(on_button_press), NULL);

    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scroll), icon_view);
    gtk_box_pack_start(GTK_BOX(right_box), scroll, TRUE, TRUE, 0);

    /* Initial path */
    populate_files(home);

    /* Add titlebar + content */
    blazeneuro_add_titlebar(main_window, "Files", main_box);

    gtk_widget_show_all(main_window);
    gtk_main();

    return 0;
}
