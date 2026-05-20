/*
 * BlazeNeuro Lock Screen
 * A secure, Shadcn-inspired lock screen using GTK3.
 * Handles password verification via PAM.
 */

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <security/pam_appl.h>
#include <pwd.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "../common/theme.h"

typedef struct {
    GtkWidget *entry;
    GtkWidget *label;
    GtkWidget *spinner;
    const char *username;
} LockData;

/* ── PAM Conversation Function ─────────────────────────── */
static int pam_conv_func(int num_msg, const struct pam_message **msg,
                         struct pam_response **resp, void *appdata_ptr) {
    char *password = (char *)appdata_ptr;
    struct pam_response *reply = calloc(num_msg, sizeof(struct pam_response));
    if (!reply) return PAM_BUF_ERR;

    for (int i = 0; i < num_msg; i++) {
        if (msg[i]->msg_style == PAM_PROMPT_ECHO_OFF || msg[i]->msg_style == PAM_PROMPT_ECHO_ON) {
            reply[i].resp = strdup(password);
        }
    }
    *resp = reply;
    return PAM_SUCCESS;
}

static gboolean verify_password(const char *username, const char *password) {
    pam_handle_t *pamh = NULL;
    struct pam_conv conv = { pam_conv_func, (void *)password };
    int retval;

    retval = pam_start("login", username, &conv, &pamh);
    if (retval == PAM_SUCCESS) {
        retval = pam_authenticate(pamh, 0);
        if (retval == PAM_SUCCESS) {
            retval = pam_acct_mgmt(pamh, 0);
        }
    }
    pam_end(pamh, retval);
    return (retval == PAM_SUCCESS);
}

/* ── Handlers ───────────────────────────────────────────── */
static void on_unlock_attempt(GtkEntry *entry, gpointer data) {
    LockData *ld = (LockData *)data;
    const char *password = gtk_entry_get_text(entry);

    gtk_widget_set_sensitive(ld->entry, FALSE);
    gtk_spinner_start(GTK_SPINNER(ld->spinner));
    gtk_label_set_text(GTK_LABEL(ld->label), "Verifying...");

    /* Simple blocking verify for now (production should use a thread) */
    if (verify_password(ld->username, password)) {
        gtk_main_quit();
    } else {
        gtk_label_set_text(GTK_LABEL(ld->label), "Invalid password. Try again.");
        gtk_entry_set_text(entry, "");
        gtk_widget_set_sensitive(ld->entry, TRUE);
        gtk_spinner_stop(GTK_SPINNER(ld->spinner));
        gtk_widget_grab_focus(ld->entry);
    }
}

/* ── Main ───────────────────────────────────────────────── */
int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    blazeneuro_load_theme();

    /* Get current username */
    struct passwd *pw = getpwuid(getuid());
    const char *username = pw ? pw->pw_name : "user";

    GtkWidget *win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(win), "Lock Screen");
    gtk_window_set_decorated(GTK_WINDOW(win), FALSE);
    gtk_window_fullscreen(GTK_WINDOW(win));
    gtk_window_set_keep_above(GTK_WINDOW(win), TRUE);

    /* Center content */
    GtkWidget *overlay = gtk_overlay_new();
    gtk_container_add(GTK_CONTAINER(win), overlay);

    /* Background image or solid color */
    GtkWidget *bg = gtk_drawing_area_new();
    gtk_style_context_add_class(gtk_widget_get_style_context(bg), "app-window");
    gtk_container_add(GTK_CONTAINER(overlay), bg);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 24);
    gtk_widget_set_halign(vbox, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(vbox, GTK_ALIGN_CENTER);
    gtk_overlay_add_overlay(GTK_OVERLAY(overlay), vbox);

    /* User Profile */
    GtkWidget *user_label = gtk_label_new(username);
    gtk_style_context_add_class(gtk_widget_get_style_context(user_label), "header");
    gtk_box_pack_start(GTK_BOX(vbox), user_label, FALSE, FALSE, 0);

    /* Password Entry */
    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(entry), FALSE);
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Enter password");
    gtk_widget_set_size_request(entry, 280, 42);
    gtk_box_pack_start(GTK_BOX(vbox), entry, FALSE, FALSE, 0);

    /* Status Label & Spinner */
    GtkWidget *status_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_halign(status_hbox, GTK_ALIGN_CENTER);
    
    GtkWidget *spinner = gtk_spinner_new();
    gtk_box_pack_start(GTK_BOX(status_hbox), spinner, FALSE, FALSE, 0);

    GtkWidget *label = gtk_label_new("Locked");
    gtk_style_context_add_class(gtk_widget_get_style_context(label), "muted");
    gtk_box_pack_start(GTK_BOX(status_hbox), label, FALSE, FALSE, 0);
    
    gtk_box_pack_start(GTK_BOX(vbox), status_hbox, FALSE, FALSE, 0);

    LockData ld = { entry, label, spinner, username };
    g_signal_connect(entry, "activate", G_CALLBACK(on_unlock_attempt), &ld);

    /* Grab keyboard and mouse */
    gtk_widget_show_all(win);
    GdkDisplay *display = gtk_widget_get_display(win);
    GdkSeat *seat = gdk_display_get_default_seat(display);
    gdk_seat_grab(seat, gtk_widget_get_window(win), GDK_SEAT_CAPABILITY_ALL, TRUE, NULL, NULL, NULL, NULL);

    gtk_main();

    return 0;
}
