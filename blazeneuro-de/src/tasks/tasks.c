/*
 * BlazeNeuro Task Viewer
 * Simple process list from /proc with periodic refresh.
 */

#include <gtk/gtk.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

enum {
    COL_PID,
    COL_NAME,
    NUM_COLS
};

static GtkListStore *store;

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

        GtkTreeIter iter;
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
                           COL_PID, ent->d_name,
                           COL_NAME, name[0] ? name : "(unknown)",
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
    (void)widget;
    (void)data;
    refresh_processes();
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    GtkWidget *win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(win), "Task Viewer");
    gtk_window_set_default_size(GTK_WINDOW(win), 640, 420);
    g_signal_connect(win, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 12);
    gtk_container_add(GTK_CONTAINER(win), vbox);

    GtkWidget *refresh_btn = gtk_button_new_with_label("Refresh");
    g_signal_connect(refresh_btn, "clicked", G_CALLBACK(on_refresh), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), refresh_btn, FALSE, FALSE, 0);

    store = gtk_list_store_new(NUM_COLS, G_TYPE_STRING, G_TYPE_STRING);

    GtkWidget *tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();

    GtkTreeViewColumn *col_pid = gtk_tree_view_column_new_with_attributes(
        "PID", renderer, "text", COL_PID, NULL);
    GtkTreeViewColumn *col_name = gtk_tree_view_column_new_with_attributes(
        "Process", renderer, "text", COL_NAME, NULL);

    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), col_pid);
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
