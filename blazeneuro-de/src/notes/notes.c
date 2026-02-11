/*
 * BlazeNeuro Notes
 * Lightweight notes app with autosave and word count.
 * shadcn dark theme.
 */

#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>

static GtkWidget *text_view;
static GtkWidget *word_count_label;
static char notes_path[512];

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

/* ── Save / Load ────────────────────────────────────────── */
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
    (void)widget; (void)data;
    save_notes();
    gtk_main_quit();
}

static void load_notes(void) {
    FILE *f = fopen(notes_path, "r");
    if (!f) return;

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    GtkTextIter iter;
    gtk_text_buffer_get_start_iter(buffer, &iter);

    char line[4096];
    while (fgets(line, sizeof(line), f)) {
        gtk_text_buffer_insert(buffer, &iter, line, -1);
    }
    fclose(f);
}

/* ── Word Count ─────────────────────────────────────────── */
static void update_word_count(GtkTextBuffer *buffer) {
    GtkTextIter start, end;
    gtk_text_buffer_get_start_iter(buffer, &start);
    gtk_text_buffer_get_end_iter(buffer, &end);

    gchar *text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
    int chars = g_utf8_strlen(text, -1);
    int words = 0;
    gboolean in_word = FALSE;

    for (const char *p = text; *p; p = g_utf8_next_char(p)) {
        gunichar c = g_utf8_get_char(p);
        if (g_unichar_isspace(c)) {
            in_word = FALSE;
        } else if (!in_word) {
            in_word = TRUE;
            words++;
        }
    }
    g_free(text);

    char buf[128];
    g_snprintf(buf, sizeof(buf), "%d words  •  %d characters", words, chars);
    gtk_label_set_text(GTK_LABEL(word_count_label), buf);
}

static void on_buffer_changed(GtkTextBuffer *buffer, gpointer data) {
    (void)data;
    update_word_count(buffer);
}

/* ── Main ───────────────────────────────────────────────── */
int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    load_theme();

    const char *home = g_get_home_dir();
    g_snprintf(notes_path, sizeof(notes_path), "%s/.blazeneuro-notes.txt", home);

    GtkWidget *win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(win), "Notes");
    gtk_window_set_default_size(GTK_WINDOW(win), 700, 500);
    gtk_widget_set_app_paintable(win, TRUE);

    GdkScreen *scr = gtk_widget_get_screen(win);
    GdkVisual *vis = gdk_screen_get_rgba_visual(scr);
    if (vis) gtk_widget_set_visual(win, vis);

    g_signal_connect(win, "destroy", G_CALLBACK(on_destroy), NULL);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(win), vbox);

    /* Header */
    GtkWidget *header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_margin_start(header, 16);
    gtk_widget_set_margin_top(header, 12);
    gtk_widget_set_margin_bottom(header, 8);
    GtkWidget *title = gtk_label_new("Notes");
    gtk_style_context_add_class(gtk_widget_get_style_context(title), "header");
    gtk_box_pack_start(GTK_BOX(header), title, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), header, FALSE, FALSE, 0);

    /* Text editor */
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_box_pack_start(GTK_BOX(vbox), scroll, TRUE, TRUE, 0);

    text_view = gtk_text_view_new();
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD_CHAR);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(text_view), 16);
    gtk_text_view_set_right_margin(GTK_TEXT_VIEW(text_view), 16);
    gtk_text_view_set_top_margin(GTK_TEXT_VIEW(text_view), 8);
    gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(text_view), 8);
    gtk_style_context_add_class(gtk_widget_get_style_context(text_view), "notes-text");
    gtk_container_add(GTK_CONTAINER(scroll), text_view);

    /* Status bar */
    GtkWidget *statusbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_style_context_add_class(gtk_widget_get_style_context(statusbar), "notes-statusbar");

    word_count_label = gtk_label_new("0 words  •  0 characters");
    gtk_label_set_xalign(GTK_LABEL(word_count_label), 0);
    gtk_widget_set_margin_start(word_count_label, 4);
    gtk_box_pack_start(GTK_BOX(statusbar), word_count_label, TRUE, TRUE, 0);

    GtkWidget *autosave_label = gtk_label_new("Autosave: 10s");
    gtk_label_set_xalign(GTK_LABEL(autosave_label), 1);
    gtk_widget_set_margin_end(autosave_label, 4);
    gtk_box_pack_end(GTK_BOX(statusbar), autosave_label, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(vbox), statusbar, FALSE, FALSE, 0);

    /* Wire up word count */
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    g_signal_connect(buffer, "changed", G_CALLBACK(on_buffer_changed), NULL);

    load_notes();
    update_word_count(buffer);

    g_timeout_add_seconds(10, autosave, NULL);

    gtk_widget_show_all(win);
    gtk_main();

    return 0;
}
