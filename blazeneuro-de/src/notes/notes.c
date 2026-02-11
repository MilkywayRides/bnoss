/*
 * BlazeNeuro Notes
 * Lightweight notes app with autosave.
 */

#include <gtk/gtk.h>
#include <stdio.h>

static GtkWidget *text_view;
static char notes_path[512];

static void save_notes(void) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    GtkTextIter start, end;
    gtk_text_buffer_get_start_iter(buffer, &start);
    gtk_text_buffer_get_end_iter(buffer, &end);

    gchar *content = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
    FILE *f = fopen(notes_path, "w");
    if (f) {
        fputs(content, f);
        fclose(f);
    }
    g_free(content);
}

static gboolean autosave(gpointer data) {
    (void)data;
    save_notes();
    return TRUE;
}

static void on_destroy(GtkWidget *widget, gpointer data) {
    (void)widget;
    (void)data;
    save_notes();
    gtk_main_quit();
}

static void load_notes(void) {
    FILE *f = fopen(notes_path, "r");
    if (!f) return;

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    GtkTextIter iter;
    gtk_text_buffer_get_start_iter(buffer, &iter);

    char line[1024];
    while (fgets(line, sizeof(line), f)) {
        gtk_text_buffer_insert(buffer, &iter, line, -1);
    }
    fclose(f);
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    const char *home = g_get_home_dir();
    g_snprintf(notes_path, sizeof(notes_path), "%s/.blazeneuro-notes.txt", home);

    GtkWidget *win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(win), "Notes");
    gtk_window_set_default_size(GTK_WINDOW(win), 700, 500);
    g_signal_connect(win, "destroy", G_CALLBACK(on_destroy), NULL);

    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(win), scroll);

    text_view = gtk_text_view_new();
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD_CHAR);
    gtk_container_add(GTK_CONTAINER(scroll), text_view);

    load_notes();

    g_timeout_add_seconds(10, autosave, NULL);

    gtk_widget_show_all(win);
    gtk_main();

    return 0;
}
