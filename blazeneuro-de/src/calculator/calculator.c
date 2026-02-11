/*
 * BlazeNeuro Calculator
 * Lightweight desktop calculator.
 */

#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdio.h>

static GtkWidget *entry_a;
static GtkWidget *entry_b;
static GtkWidget *result_label;

static void update_result(double value) {
    char buf[128];
    g_snprintf(buf, sizeof(buf), "Result: %.6g", value);
    gtk_label_set_text(GTK_LABEL(result_label), buf);
}

static gboolean read_inputs(double *a, double *b) {
    const char *text_a = gtk_entry_get_text(GTK_ENTRY(entry_a));
    const char *text_b = gtk_entry_get_text(GTK_ENTRY(entry_b));

    char *end_a = NULL;
    char *end_b = NULL;
    *a = g_ascii_strtod(text_a, &end_a);
    *b = g_ascii_strtod(text_b, &end_b);

    if (!text_a[0] || !text_b[0] || (end_a && *end_a != '\0') || (end_b && *end_b != '\0')) {
        gtk_label_set_text(GTK_LABEL(result_label), "Result: Invalid input");
        return FALSE;
    }
    return TRUE;
}

static void on_add(GtkWidget *widget, gpointer data) {
    (void)widget; (void)data;
    double a, b;
    if (read_inputs(&a, &b)) update_result(a + b);
}

static void on_sub(GtkWidget *widget, gpointer data) {
    (void)widget; (void)data;
    double a, b;
    if (read_inputs(&a, &b)) update_result(a - b);
}

static void on_mul(GtkWidget *widget, gpointer data) {
    (void)widget; (void)data;
    double a, b;
    if (read_inputs(&a, &b)) update_result(a * b);
}

static void on_div(GtkWidget *widget, gpointer data) {
    (void)widget; (void)data;
    double a, b;
    if (!read_inputs(&a, &b)) return;
    if (b == 0.0) {
        gtk_label_set_text(GTK_LABEL(result_label), "Result: Division by zero");
        return;
    }
    update_result(a / b);
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    GtkWidget *win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(win), "Calculator");
    gtk_window_set_default_size(GTK_WINDOW(win), 360, 220);
    g_signal_connect(win, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 16);
    gtk_container_add(GTK_CONTAINER(win), vbox);

    entry_a = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_a), "First number");
    gtk_box_pack_start(GTK_BOX(vbox), entry_a, FALSE, FALSE, 0);

    entry_b = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_b), "Second number");
    gtk_box_pack_start(GTK_BOX(vbox), entry_b, FALSE, FALSE, 0);

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 6);
    gtk_box_pack_start(GTK_BOX(vbox), grid, FALSE, FALSE, 0);

    GtkWidget *add_btn = gtk_button_new_with_label("+");
    GtkWidget *sub_btn = gtk_button_new_with_label("-");
    GtkWidget *mul_btn = gtk_button_new_with_label("×");
    GtkWidget *div_btn = gtk_button_new_with_label("÷");

    g_signal_connect(add_btn, "clicked", G_CALLBACK(on_add), NULL);
    g_signal_connect(sub_btn, "clicked", G_CALLBACK(on_sub), NULL);
    g_signal_connect(mul_btn, "clicked", G_CALLBACK(on_mul), NULL);
    g_signal_connect(div_btn, "clicked", G_CALLBACK(on_div), NULL);

    gtk_grid_attach(GTK_GRID(grid), add_btn, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), sub_btn, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), mul_btn, 2, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), div_btn, 3, 0, 1, 1);

    result_label = gtk_label_new("Result: 0");
    gtk_label_set_xalign(GTK_LABEL(result_label), 0.0f);
    gtk_box_pack_start(GTK_BOX(vbox), result_label, FALSE, FALSE, 0);

    gtk_widget_show_all(win);
    gtk_main();
    return 0;
}
