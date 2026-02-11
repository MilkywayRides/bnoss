/*
 * BlazeNeuro Task Viewer
 * Process viewer with styled tree view and kill-process support.
 * shadcn dark theme.
 */

#include <gtk/gtk.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <stdlib.h>

#include "../common/theme.h"

enum {
    COL_PID,
    COL_NAME,
    COL_PID_INT,
    NUM_COLS
};

static GtkListStore *store;
static GtkWidget *tree;
static gboolean is_numeric_name(const char *name) {
    if (!name || !name[0]) return FALSE;
    for (const char *p = name; *p; ++p) {
        if (!g_ascii_isdigit(*p)) return FALSE;
    }
    return TRUE;
}

static void refresh_processes(void) {
    gtk_list_store_clear(store);

    DIR *proc = opendir("/proc");
    if (!proc) return;

    struct dirent *ent;
    while ((ent = readdir(proc)) != NULL) {
        if (!is_numeric_name(ent->d_name)) continue;

        char path[256];
        g_snprintf(path, sizeof(path), "/proc/%s/comm", ent->d_name);

        FILE *f = fopen(path, "r");
        if (!f) continue;

        char name[256] = {0};
        if (!fgets(name, sizeof(name), f)) {
            fclose(f);
            continue;
        }
        fclose(f);

        name[strcspn(name, "\n")] = '\0';

        int pid = atoi(ent->d_name);

        GtkTreeIter iter;
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
                           COL_PID, ent->d_name,
                           COL_NAME, name[0] ? name : "(unknown)",
                           COL_PID_INT, pid,
                           -1);
    }

    closedir(proc);
}

static gboolean on_timer(gpointer data) {
    (void)data;
    refresh_processes();
    return TRUE;
}

static void on_refresh(GtkWidget *widget, gpointer data) {
    (void)widget; (void)data;
    refresh_processes();
}

static void on_kill(GtkWidget *widget, gpointer data) {
    (void)widget; (void)data;

    GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
    GtkTreeIter iter;
    GtkTreeModel *model;

    if (gtk_tree_selection_get_selected(sel, &model, &iter)) {
        int pid;
        gtk_tree_model_get(model, &iter, COL_PID_INT, &pid, -1);
        if (pid > 1) {
            kill(pid, SIGTERM);
            refresh_processes();
        }
    }
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    blazeneuro_load_theme();

    GtkWidget *win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(win), "Task Viewer");
    gtk_window_set_default_size(GTK_WINDOW(win), 680, 480);
    gtk_widget_set_app_paintable(win, TRUE);

    GdkScreen *scr = gtk_widget_get_screen(win);
    GdkVisual *vis = gdk_screen_get_rgba_visual(scr);
    if (vis) gtk_widget_set_visual(win, vis);

    g_signal_connect(win, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(win), vbox);

    /* Header */
    GtkWidget *header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_style_context_add_class(gtk_widget_get_style_context(header), "task-header");

    GtkWidget *title = gtk_label_new("Task Viewer");
    gtk_style_context_add_class(gtk_widget_get_style_context(title), "section-title");
    gtk_box_pack_start(GTK_BOX(header), title, TRUE, TRUE, 0);
    gtk_label_set_xalign(GTK_LABEL(title), 0);

    GtkWidget *refresh_btn = gtk_button_new_with_label("Refresh");
    g_signal_connect(refresh_btn, "clicked", G_CALLBACK(on_refresh), NULL);
    gtk_box_pack_end(GTK_BOX(header), refresh_btn, FALSE, FALSE, 0);

    GtkWidget *kill_btn = gtk_button_new_with_label("End Process");
    gtk_style_context_add_class(gtk_widget_get_style_context(kill_btn), "destructive-action");
    g_signal_connect(kill_btn, "clicked", G_CALLBACK(on_kill), NULL);
    gtk_box_pack_end(GTK_BOX(header), kill_btn, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(vbox), header, FALSE, FALSE, 0);

    /* Tree view */
    store = gtk_list_store_new(NUM_COLS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT);

    tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), TRUE);
    gtk_tree_selection_set_mode(
        gtk_tree_view_get_selection(GTK_TREE_VIEW(tree)),
        GTK_SELECTION_SINGLE);

    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();

    GtkTreeViewColumn *col_pid = gtk_tree_view_column_new_with_attributes(
        "PID", renderer, "text", COL_PID, NULL);
    gtk_tree_view_column_set_min_width(col_pid, 80);
    gtk_tree_view_column_set_sort_column_id(col_pid, COL_PID_INT);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), col_pid);

    GtkTreeViewColumn *col_name = gtk_tree_view_column_new_with_attributes(
        "Process", renderer, "text", COL_NAME, NULL);
    gtk_tree_view_column_set_expand(col_name, TRUE);
    gtk_tree_view_column_set_sort_column_id(col_name, COL_NAME);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), col_name);

    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scroll), tree);
    gtk_box_pack_start(GTK_BOX(vbox), scroll, TRUE, TRUE, 0);

    refresh_processes();
    g_timeout_add_seconds(5, on_timer, NULL);

    gtk_widget_show_all(win);
    gtk_main();

    return 0;
}
