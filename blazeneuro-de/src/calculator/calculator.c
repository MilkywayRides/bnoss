/*
 * BlazeNeuro Calculator
 * Modern calculator with shadcn-styled button grid.
 * Supports basic arithmetic operations.
 */

#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

static GtkWidget *display_label;
static GtkWidget *expr_label;
static char expression[512] = "";
static char display_text[128] = "0";
static double stored_value = 0;
static char pending_op = 0;
static int new_input = 1;

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

static void update_display(void) {
    gtk_label_set_text(GTK_LABEL(display_label), display_text);
    gtk_label_set_text(GTK_LABEL(expr_label), expression);
}

static double parse_display(void) {
    return g_ascii_strtod(display_text, NULL);
}

static double evaluate(double a, char op, double b) {
    switch (op) {
        case '+': return a + b;
        case '-': return a - b;
        case '*': return a * b;
        case '/': return (b != 0.0) ? a / b : 0.0;
        default: return b;
    }
}

static void on_digit(GtkWidget *widget, gpointer data) {
    (void)widget;
    const char *digit = (const char *)data;

    if (new_input) {
        g_strlcpy(display_text, digit, sizeof(display_text));
        new_input = 0;
    } else {
        if (strcmp(display_text, "0") == 0 && strcmp(digit, "0") == 0) return;
        if (strcmp(display_text, "0") == 0 && strcmp(digit, ".") != 0) {
            g_strlcpy(display_text, digit, sizeof(display_text));
        } else {
            if (strcmp(digit, ".") == 0 && strchr(display_text, '.')) return;
            g_strlcat(display_text, digit, sizeof(display_text));
        }
    }
    update_display();
}

static void on_operator(GtkWidget *widget, gpointer data) {
    (void)widget;
    char op = *(const char *)data;

    double current = parse_display();
    if (pending_op && !new_input) {
        stored_value = evaluate(stored_value, pending_op, current);
        g_snprintf(display_text, sizeof(display_text), "%.10g", stored_value);
    } else {
        stored_value = current;
    }

    pending_op = op;
    new_input = 1;

    g_snprintf(expression, sizeof(expression), "%.10g %c", stored_value, op);
    update_display();
}

static void on_equals(GtkWidget *widget, gpointer data) {
    (void)widget; (void)data;
    if (!pending_op) return;

    double current = parse_display();
    double result = evaluate(stored_value, pending_op, current);

    g_snprintf(expression, sizeof(expression), "%.10g %c %.10g =", stored_value, pending_op, current);
    g_snprintf(display_text, sizeof(display_text), "%.10g", result);

    stored_value = result;
    pending_op = 0;
    new_input = 1;
    update_display();
}

static void on_clear(GtkWidget *widget, gpointer data) {
    (void)widget; (void)data;
    g_strlcpy(display_text, "0", sizeof(display_text));
    expression[0] = '\0';
    stored_value = 0;
    pending_op = 0;
    new_input = 1;
    update_display();
}

static void on_backspace(GtkWidget *widget, gpointer data) {
    (void)widget; (void)data;
    size_t len = strlen(display_text);
    if (len > 1) {
        display_text[len - 1] = '\0';
    } else {
        g_strlcpy(display_text, "0", sizeof(display_text));
        new_input = 1;
    }
    update_display();
}

static void on_negate(GtkWidget *widget, gpointer data) {
    (void)widget; (void)data;
    double val = parse_display();
    g_snprintf(display_text, sizeof(display_text), "%.10g", -val);
    update_display();
}

/* ── Button Helper ──────────────────────────────────────── */
static GtkWidget *make_btn(const char *label, const char *css_class,
                           GCallback callback, gpointer data) {
    GtkWidget *btn = gtk_button_new_with_label(label);
    gtk_style_context_add_class(gtk_widget_get_style_context(btn), css_class);
    gtk_widget_set_hexpand(btn, TRUE);
    gtk_widget_set_vexpand(btn, TRUE);
    g_signal_connect(btn, "clicked", callback, data);
    return btn;
}

/* ── Main ───────────────────────────────────────────────── */
int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    load_theme();

    GtkWidget *win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(win), "Calculator");
    gtk_window_set_default_size(GTK_WINDOW(win), 320, 480);
    gtk_window_set_resizable(GTK_WINDOW(win), FALSE);
    gtk_widget_set_app_paintable(win, TRUE);

    GdkScreen *scr = gtk_widget_get_screen(win);
    GdkVisual *vis = gdk_screen_get_rgba_visual(scr);
    if (vis) gtk_widget_set_visual(win, vis);

    g_signal_connect(win, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 12);
    gtk_container_add(GTK_CONTAINER(win), vbox);

    /* Display area */
    GtkWidget *display_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_style_context_add_class(gtk_widget_get_style_context(display_box), "calc-display");
    gtk_widget_set_margin_bottom(display_box, 12);

    expr_label = gtk_label_new("");
    gtk_style_context_add_class(gtk_widget_get_style_context(expr_label), "calc-display-expr");
    gtk_label_set_xalign(GTK_LABEL(expr_label), 1.0);
    gtk_box_pack_start(GTK_BOX(display_box), expr_label, FALSE, FALSE, 0);

    display_label = gtk_label_new("0");
    gtk_label_set_xalign(GTK_LABEL(display_label), 1.0);
    gtk_box_pack_start(GTK_BOX(display_box), display_label, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(vbox), display_box, FALSE, FALSE, 0);

    /* Button grid */
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 6);
    gtk_box_pack_start(GTK_BOX(vbox), grid, TRUE, TRUE, 0);

    /* Persistent operator strings (need stable pointers) */
    static const char op_add[] = "+";
    static const char op_sub[] = "-";
    static const char op_mul[] = "*";
    static const char op_div[] = "/";

    static const char d0[] = "0"; static const char d1[] = "1";
    static const char d2[] = "2"; static const char d3[] = "3";
    static const char d4[] = "4"; static const char d5[] = "5";
    static const char d6[] = "6"; static const char d7[] = "7";
    static const char d8[] = "8"; static const char d9[] = "9";
    static const char dot[] = ".";

    /* Row 0: C  ⌫  ±  ÷ */
    gtk_grid_attach(GTK_GRID(grid), make_btn("C", "calc-btn-clear", G_CALLBACK(on_clear), NULL), 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), make_btn("⌫", "calc-btn-op", G_CALLBACK(on_backspace), NULL), 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), make_btn("±", "calc-btn-op", G_CALLBACK(on_negate), NULL), 2, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), make_btn("÷", "calc-btn-op", G_CALLBACK(on_operator), (gpointer)op_div), 3, 0, 1, 1);

    /* Row 1: 7 8 9 × */
    gtk_grid_attach(GTK_GRID(grid), make_btn("7", "calc-btn", G_CALLBACK(on_digit), (gpointer)d7), 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), make_btn("8", "calc-btn", G_CALLBACK(on_digit), (gpointer)d8), 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), make_btn("9", "calc-btn", G_CALLBACK(on_digit), (gpointer)d9), 2, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), make_btn("×", "calc-btn-op", G_CALLBACK(on_operator), (gpointer)op_mul), 3, 1, 1, 1);

    /* Row 2: 4 5 6 - */
    gtk_grid_attach(GTK_GRID(grid), make_btn("4", "calc-btn", G_CALLBACK(on_digit), (gpointer)d4), 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), make_btn("5", "calc-btn", G_CALLBACK(on_digit), (gpointer)d5), 1, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), make_btn("6", "calc-btn", G_CALLBACK(on_digit), (gpointer)d6), 2, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), make_btn("−", "calc-btn-op", G_CALLBACK(on_operator), (gpointer)op_sub), 3, 2, 1, 1);

    /* Row 3: 1 2 3 + */
    gtk_grid_attach(GTK_GRID(grid), make_btn("1", "calc-btn", G_CALLBACK(on_digit), (gpointer)d1), 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), make_btn("2", "calc-btn", G_CALLBACK(on_digit), (gpointer)d2), 1, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), make_btn("3", "calc-btn", G_CALLBACK(on_digit), (gpointer)d3), 2, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), make_btn("+", "calc-btn-op", G_CALLBACK(on_operator), (gpointer)op_add), 3, 3, 1, 1);

    /* Row 4: 0(wide) . = */
    gtk_grid_attach(GTK_GRID(grid), make_btn("0", "calc-btn", G_CALLBACK(on_digit), (gpointer)d0), 0, 4, 2, 1);
    gtk_grid_attach(GTK_GRID(grid), make_btn(".", "calc-btn", G_CALLBACK(on_digit), (gpointer)dot), 2, 4, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), make_btn("=", "calc-btn-equals", G_CALLBACK(on_equals), NULL), 3, 4, 1, 1);

    gtk_widget_show_all(win);
    gtk_main();

    return 0;
}
