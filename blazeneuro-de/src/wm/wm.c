/*
 * BlazeNeuro Window Manager
 * A feature-rich X11 window manager with EWMH compliance.
 * Handles window placement, focus, move/resize, snapping,
 * minimize/restore, fullscreen, and keyboard shortcuts.
 * Delegates visual effects to picom compositor.
 */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/XKBlib.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

/* ── Globals ────────────────────────────────────────────── */
static Display *dpy;
static Window root;
static int screen;
static int sw, sh;
static int running = 1;

/* Dragging state */
static Window drag_win = None;
static int drag_start_x, drag_start_y;
static int drag_win_x, drag_win_y;
static int drag_win_w, drag_win_h;
static int drag_mode = 0; /* 0=none, 1=move, 2=resize */

/* EWMH atoms */
static Atom net_supported, net_wm_name, net_wm_state;
static Atom net_wm_state_fullscreen, net_wm_state_hidden;
static Atom net_wm_state_maximized_vert, net_wm_state_maximized_horz;
static Atom net_wm_window_type;
static Atom net_wm_window_type_dock, net_wm_window_type_dialog;
static Atom net_wm_window_type_normal, net_wm_strut;
static Atom net_wm_strut_partial, net_active_window;
static Atom net_supporting_wm_check, net_client_list;
static Atom net_close_window, net_wm_state_demands_attention;
static Atom net_current_desktop, net_number_of_desktops;
static Atom wm_protocols, wm_delete_window, wm_state;
static Atom wm_change_state;

/* Client tracking */
#define MAX_CLIENTS 256
typedef struct {
    Window win;
    int x, y, w, h;       /* saved geometry for restore */
    int is_fullscreen;
    int is_minimized;
    int is_maximized;
} Client;

static Client clients[MAX_CLIENTS];
static int nclients = 0;
static int topbar_h = 32;
static int dock_h = 72;

/* ── EWMH Setup ────────────────────────────────────────── */
static void setup_ewmh(void) {
    net_supported           = XInternAtom(dpy, "_NET_SUPPORTED", False);
    net_wm_name             = XInternAtom(dpy, "_NET_WM_NAME", False);
    net_wm_state            = XInternAtom(dpy, "_NET_WM_STATE", False);
    net_wm_state_fullscreen = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
    net_wm_state_hidden     = XInternAtom(dpy, "_NET_WM_STATE_HIDDEN", False);
    net_wm_state_maximized_vert = XInternAtom(dpy, "_NET_WM_STATE_MAXIMIZED_VERT", False);
    net_wm_state_maximized_horz = XInternAtom(dpy, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
    net_wm_state_demands_attention = XInternAtom(dpy, "_NET_WM_STATE_DEMANDS_ATTENTION", False);
    net_wm_window_type      = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
    net_wm_window_type_dock = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DOCK", False);
    net_wm_window_type_dialog = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DIALOG", False);
    net_wm_window_type_normal = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_NORMAL", False);
    net_wm_strut            = XInternAtom(dpy, "_NET_WM_STRUT", False);
    net_wm_strut_partial    = XInternAtom(dpy, "_NET_WM_STRUT_PARTIAL", False);
    net_active_window       = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);
    net_supporting_wm_check = XInternAtom(dpy, "_NET_SUPPORTING_WM_CHECK", False);
    net_client_list         = XInternAtom(dpy, "_NET_CLIENT_LIST", False);
    net_close_window        = XInternAtom(dpy, "_NET_CLOSE_WINDOW", False);
    net_current_desktop     = XInternAtom(dpy, "_NET_CURRENT_DESKTOP", False);
    net_number_of_desktops  = XInternAtom(dpy, "_NET_NUMBER_OF_DESKTOPS", False);
    wm_protocols            = XInternAtom(dpy, "WM_PROTOCOLS", False);
    wm_delete_window        = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
    wm_state                = XInternAtom(dpy, "WM_STATE", False);
    wm_change_state         = XInternAtom(dpy, "WM_CHANGE_STATE", False);

    /* Create check window */
    Window check = XCreateSimpleWindow(dpy, root, 0, 0, 1, 1, 0, 0, 0);
    XChangeProperty(dpy, check, net_supporting_wm_check, XA_WINDOW, 32,
                    PropModeReplace, (unsigned char *)&check, 1);
    XChangeProperty(dpy, root, net_supporting_wm_check, XA_WINDOW, 32,
                    PropModeReplace, (unsigned char *)&check, 1);

    const char *wm_name_str = "BlazeNeuro";
    XChangeProperty(dpy, check, net_wm_name,
                    XInternAtom(dpy, "UTF8_STRING", False), 8,
                    PropModeReplace, (unsigned char *)wm_name_str, strlen(wm_name_str));

    Atom supported[] = {
        net_supported, net_wm_name, net_wm_state,
        net_wm_state_fullscreen, net_wm_state_hidden,
        net_wm_state_maximized_vert, net_wm_state_maximized_horz,
        net_wm_window_type, net_active_window, net_client_list,
        net_wm_strut, net_wm_strut_partial, net_close_window,
        net_current_desktop, net_number_of_desktops
    };
    XChangeProperty(dpy, root, net_supported, XA_ATOM, 32,
                    PropModeReplace, (unsigned char *)supported,
                    sizeof(supported) / sizeof(Atom));

    /* Single desktop */
    long desktop = 0;
    long num_desktops = 1;
    XChangeProperty(dpy, root, net_current_desktop, XA_CARDINAL, 32,
                    PropModeReplace, (unsigned char *)&desktop, 1);
    XChangeProperty(dpy, root, net_number_of_desktops, XA_CARDINAL, 32,
                    PropModeReplace, (unsigned char *)&num_desktops, 1);
}

/* ── Client Management ─────────────────────────────────── */
static void update_client_list(void) {
    Window wins[MAX_CLIENTS];
    int count = 0;
    for (int i = 0; i < nclients; i++)
        wins[count++] = clients[i].win;
    XChangeProperty(dpy, root, net_client_list, XA_WINDOW, 32,
                    PropModeReplace, (unsigned char *)wins, count);
}

static Client *find_client(Window w) {
    for (int i = 0; i < nclients; i++)
        if (clients[i].win == w) return &clients[i];
    return NULL;
}

static void add_client(Window w) {
    if (nclients < MAX_CLIENTS) {
        Client *c = &clients[nclients++];
        c->win = w;
        c->is_fullscreen = 0;
        c->is_minimized = 0;
        c->is_maximized = 0;

        /* Save initial geometry */
        XWindowAttributes wa;
        if (XGetWindowAttributes(dpy, w, &wa)) {
            c->x = wa.x; c->y = wa.y;
            c->w = wa.width; c->h = wa.height;
        }
        update_client_list();
    }
}

static void remove_client(Window w) {
    for (int i = 0; i < nclients; i++) {
        if (clients[i].win == w) {
            memmove(&clients[i], &clients[i + 1],
                    (nclients - i - 1) * sizeof(Client));
            nclients--;
            update_client_list();
            return;
        }
    }
}

static void set_active(Window w) {
    XChangeProperty(dpy, root, net_active_window, XA_WINDOW, 32,
                    PropModeReplace, (unsigned char *)&w, 1);
}

/* ── Window Type Check ─────────────────────────────────── */
static int is_dock(Window w) {
    Atom actual;
    int fmt;
    unsigned long n, left;
    unsigned char *data = NULL;

    if (XGetWindowProperty(dpy, w, net_wm_window_type, 0, 1, False,
                           XA_ATOM, &actual, &fmt, &n, &left, &data) == Success && data) {
        Atom type = *(Atom *)data;
        XFree(data);
        return type == net_wm_window_type_dock;
    }
    return 0;
}

/* ── Send WM_DELETE_WINDOW ─────────────────────────────── */
static int send_delete(Window w) {
    Atom *protocols;
    int n;
    int found = 0;

    if (XGetWMProtocols(dpy, w, &protocols, &n)) {
        for (int i = 0; i < n; i++) {
            if (protocols[i] == wm_delete_window) {
                found = 1;
                break;
            }
        }
        XFree(protocols);
    }

    if (found) {
        XEvent ev;
        memset(&ev, 0, sizeof(ev));
        ev.xclient.type = ClientMessage;
        ev.xclient.window = w;
        ev.xclient.message_type = wm_protocols;
        ev.xclient.format = 32;
        ev.xclient.data.l[0] = wm_delete_window;
        ev.xclient.data.l[1] = CurrentTime;
        XSendEvent(dpy, w, False, NoEventMask, &ev);
    } else {
        XKillClient(dpy, w);
    }
    return found;
}

/* ── Focus ──────────────────────────────────────────────── */
static void focus_window(Window w) {
    XSetInputFocus(dpy, w, RevertToPointerRoot, CurrentTime);
    XRaiseWindow(dpy, w);
    set_active(w);
}

/* ── Fullscreen Toggle ──────────────────────────────────── */
static void toggle_fullscreen(Window w) {
    Client *c = find_client(w);
    if (!c) return;

    if (c->is_fullscreen) {
        /* Restore */
        XMoveResizeWindow(dpy, w, c->x, c->y, c->w, c->h);
        c->is_fullscreen = 0;

        /* Remove fullscreen state */
        XDeleteProperty(dpy, w, net_wm_state);
    } else {
        /* Save geometry and go fullscreen */
        XWindowAttributes wa;
        XGetWindowAttributes(dpy, w, &wa);
        c->x = wa.x; c->y = wa.y;
        c->w = wa.width; c->h = wa.height;

        XMoveResizeWindow(dpy, w, 0, 0, sw, sh);
        XRaiseWindow(dpy, w);
        c->is_fullscreen = 1;

        /* Set fullscreen state */
        XChangeProperty(dpy, w, net_wm_state, XA_ATOM, 32,
                        PropModeReplace,
                        (unsigned char *)&net_wm_state_fullscreen, 1);
    }
}

/* ── Maximize Toggle ────────────────────────────────────── */
static void toggle_maximize(Window w) {
    Client *c = find_client(w);
    if (!c) return;

    if (c->is_maximized) {
        XMoveResizeWindow(dpy, w, c->x, c->y, c->w, c->h);
        c->is_maximized = 0;
    } else {
        XWindowAttributes wa;
        XGetWindowAttributes(dpy, w, &wa);
        c->x = wa.x; c->y = wa.y;
        c->w = wa.width; c->h = wa.height;

        XMoveResizeWindow(dpy, w, 0, topbar_h, sw, sh - topbar_h - dock_h);
        c->is_maximized = 1;
    }
    XRaiseWindow(dpy, w);
}

/* ── Minimize / Restore ─────────────────────────────────── */
static void minimize_window(Window w) {
    Client *c = find_client(w);
    if (!c) return;

    XUnmapWindow(dpy, w);
    c->is_minimized = 1;

    /* Set WM_STATE to IconicState */
    long state[] = { 3 /* IconicState */, None };
    XChangeProperty(dpy, w, wm_state, wm_state, 32,
                    PropModeReplace, (unsigned char *)state, 2);

    /* Set _NET_WM_STATE_HIDDEN */
    XChangeProperty(dpy, w, net_wm_state, XA_ATOM, 32,
                    PropModeReplace,
                    (unsigned char *)&net_wm_state_hidden, 1);
}

static void restore_window(Window w) {
    Client *c = find_client(w);
    if (!c || !c->is_minimized) return;

    XMapWindow(dpy, w);
    c->is_minimized = 0;

    /* Set WM_STATE to NormalState */
    long state[] = { 1 /* NormalState */, None };
    XChangeProperty(dpy, w, wm_state, wm_state, 32,
                    PropModeReplace, (unsigned char *)state, 2);

    /* Remove hidden state */
    XDeleteProperty(dpy, w, net_wm_state);
    focus_window(w);
}

/* ── Window Snapping (left/right half) ──────────────────── */
static void snap_window(Window w, int direction) {
    /* direction: 0=left, 1=right */
    Client *c = find_client(w);
    if (!c) return;

    /* Save geometry if not already snapped/maximized */
    if (!c->is_maximized) {
        XWindowAttributes wa;
        XGetWindowAttributes(dpy, w, &wa);
        c->x = wa.x; c->y = wa.y;
        c->w = wa.width; c->h = wa.height;
    }

    int avail_h = sh - topbar_h - dock_h;
    int half_w = sw / 2;

    if (direction == 0) {
        /* Snap left */
        XMoveResizeWindow(dpy, w, 0, topbar_h, half_w, avail_h);
    } else {
        /* Snap right */
        XMoveResizeWindow(dpy, w, half_w, topbar_h, half_w, avail_h);
    }
    c->is_maximized = 0; /* snapping is not maximizing */
    XRaiseWindow(dpy, w);
}

/* ── Show Desktop (minimize all) ────────────────────────── */
static int desktop_shown = 0;

static void toggle_show_desktop(void) {
    if (desktop_shown) {
        /* Restore all */
        for (int i = 0; i < nclients; i++) {
            if (clients[i].is_minimized) {
                restore_window(clients[i].win);
            }
        }
        desktop_shown = 0;
    } else {
        /* Minimize all non-dock windows */
        for (int i = 0; i < nclients; i++) {
            if (!clients[i].is_minimized) {
                minimize_window(clients[i].win);
            }
        }
        desktop_shown = 1;
    }
}

/* ── Event Handlers ─────────────────────────────────────── */
static void handle_map_request(XMapRequestEvent *ev) {
    Window w = ev->window;

    /* Get window hints for size */
    XWindowAttributes wa;
    XGetWindowAttributes(dpy, w, &wa);

    /* Check if dock */
    if (is_dock(w)) {
        XMapWindow(dpy, w);
        return;
    }

    /* Set border to 0 for cleaner CSD look */
    XSetWindowBorderWidth(dpy, w, 0);

    /* Center new windows, reserve space for topbar and dock */
    int avail_w = sw;
    int avail_h = sh - topbar_h - dock_h;

    int win_w = wa.width > 0 ? wa.width : avail_w * 2 / 3;
    int win_h = wa.height > 0 ? wa.height : avail_h * 2 / 3;

    if (win_w > avail_w) win_w = avail_w;
    if (win_h > avail_h) win_h = avail_h;

    int x = (avail_w - win_w) / 2;
    int y = topbar_h + (avail_h - win_h) / 2;

    XMoveResizeWindow(dpy, w, x, y, win_w, win_h);
    XSelectInput(dpy, w, EnterWindowMask | FocusChangeMask |
                 PropertyChangeMask | StructureNotifyMask);

    /* Grab Alt+Click for move, Alt+RightClick for resize */
    XGrabButton(dpy, 1, Mod1Mask, w, True,
                ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
                GrabModeAsync, GrabModeAsync, None, None);
    XGrabButton(dpy, 3, Mod1Mask, w, True,
                ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
                GrabModeAsync, GrabModeAsync, None, None);

    /* Set WM_STATE to NormalState */
    long state[] = { 1 /* NormalState */, None };
    XChangeProperty(dpy, w, wm_state, wm_state, 32,
                    PropModeReplace, (unsigned char *)state, 2);

    XMapWindow(dpy, w);
    add_client(w);
    focus_window(w);
}

static void handle_configure_request(XConfigureRequestEvent *ev) {
    XWindowChanges wc;
    wc.x = ev->x;
    wc.y = ev->y;
    wc.width = ev->width;
    wc.height = ev->height;
    wc.border_width = 0; /* Always 0 for CSD */
    wc.sibling = ev->above;
    wc.stack_mode = ev->detail;
    XConfigureWindow(dpy, ev->window, ev->value_mask, &wc);
}

static void handle_unmap(XUnmapEvent *ev) {
    /* Don't remove if we minimized it ourselves */
    Client *c = find_client(ev->window);
    if (c && c->is_minimized) return;
    remove_client(ev->window);
}

static void handle_destroy(XDestroyWindowEvent *ev) {
    remove_client(ev->window);
}

static void handle_button_press(XButtonEvent *ev) {
    if (ev->subwindow == None) return;

    /* Focus on click */
    focus_window(ev->subwindow);

    if (ev->state & Mod1Mask) {
        drag_win = ev->subwindow;
        drag_start_x = ev->x_root;
        drag_start_y = ev->y_root;

        XWindowAttributes wa;
        XGetWindowAttributes(dpy, drag_win, &wa);
        drag_win_x = wa.x;
        drag_win_y = wa.y;
        drag_win_w = wa.width;
        drag_win_h = wa.height;

        drag_mode = (ev->button == 1) ? 1 : 2;
    }
}

static void handle_button_release(XButtonEvent *ev) {
    (void)ev;
    drag_win = None;
    drag_mode = 0;
}

static void handle_motion(XMotionEvent *ev) {
    if (drag_win == None) return;

    /* Coalesce motion events — drain and use latest */
    while (XCheckTypedWindowEvent(dpy, ev->window, MotionNotify, (XEvent *)ev));

    int dx = ev->x_root - drag_start_x;
    int dy = ev->y_root - drag_start_y;

    if (drag_mode == 1) {
        /* Move */
        XMoveWindow(dpy, drag_win, drag_win_x + dx, drag_win_y + dy);
    } else if (drag_mode == 2) {
        /* Resize */
        int nw = drag_win_w + dx;
        int nh = drag_win_h + dy;
        if (nw < 100) nw = 100;
        if (nh < 60) nh = 60;
        XResizeWindow(dpy, drag_win, nw, nh);
    }
}

/* ── Client Message Handler (EWMH) ─────────────────────── */
static void handle_client_message(XClientMessageEvent *ev) {
    /* _NET_CLOSE_WINDOW */
    if (ev->message_type == net_close_window) {
        send_delete(ev->window);
        return;
    }

    /* _NET_ACTIVE_WINDOW — raise and focus */
    if (ev->message_type == net_active_window) {
        Client *c = find_client(ev->window);
        if (c && c->is_minimized)
            restore_window(ev->window);
        else
            focus_window(ev->window);
        return;
    }

    /* _NET_WM_STATE toggle */
    if (ev->message_type == net_wm_state) {
        Atom action_atom = (Atom)ev->data.l[1];
        /* int action = ev->data.l[0]; */ /* 0=remove, 1=add, 2=toggle */

        if (action_atom == net_wm_state_fullscreen) {
            toggle_fullscreen(ev->window);
        } else if (action_atom == net_wm_state_hidden) {
            Client *c = find_client(ev->window);
            if (c && c->is_minimized)
                restore_window(ev->window);
            else
                minimize_window(ev->window);
        } else if (action_atom == net_wm_state_maximized_vert ||
                   action_atom == net_wm_state_maximized_horz) {
            toggle_maximize(ev->window);
        }
        return;
    }

    /* WM_CHANGE_STATE — minimize request (e.g., gtk_window_iconify) */
    if (ev->message_type == wm_change_state) {
        if (ev->data.l[0] == 3 /* IconicState */) {
            minimize_window(ev->window);
        }
        return;
    }
}

static void handle_key_press(XKeyEvent *ev) {
    KeySym sym = XkbKeycodeToKeysym(dpy, ev->keycode, 0, 0);

    /* Get focused window */
    Window focused = None;
    int revert;
    XGetInputFocus(dpy, &focused, &revert);
    if (focused == root || focused == None) focused = None;

    if (ev->state & Mod1Mask) {
        if (sym == XK_F4) {
            /* Alt+F4: close window */
            if (focused != None)
                send_delete(focused);
        } else if (sym == XK_Tab) {
            /* Alt+Tab: cycle windows (skip minimized) */
            if (nclients > 1) {
                /* Find next non-minimized window */
                int attempts = nclients;
                do {
                    Client first = clients[0];
                    memmove(&clients[0], &clients[1], (nclients - 1) * sizeof(Client));
                    clients[nclients - 1] = first;
                    attempts--;
                } while (attempts > 0 && clients[0].is_minimized);

                if (!clients[0].is_minimized)
                    focus_window(clients[0].win);
            } else if (nclients == 1 && !clients[0].is_minimized) {
                focus_window(clients[0].win);
            }
        } else if (sym == XK_space) {
            /* Alt+Space: launch app launcher */
            if (fork() == 0) {
                execlp("blazeneuro-launcher", "blazeneuro-launcher", NULL);
                exit(0);
            }
        } else if (sym == XK_Return) {
            /* Alt+Enter: launch terminal */
            if (fork() == 0) {
                execlp("blazeneuro-terminal", "blazeneuro-terminal", NULL);
                exit(0);
            }
        } else if (sym == XK_F11) {
            /* Alt+F11: toggle fullscreen */
            if (focused != None)
                toggle_fullscreen(focused);
        } else if (sym == XK_F9) {
            /* Alt+F9: minimize */
            if (focused != None)
                minimize_window(focused);
        } else if (sym == XK_F10) {
            /* Alt+F10: toggle maximize */
            if (focused != None)
                toggle_maximize(focused);
        }
    }

    if (ev->state & Mod4Mask) {
        /* Super key shortcuts */
        if (sym == XK_Left && focused != None) {
            /* Super+Left: snap left */
            snap_window(focused, 0);
        } else if (sym == XK_Right && focused != None) {
            /* Super+Right: snap right */
            snap_window(focused, 1);
        } else if (sym == XK_Up && focused != None) {
            /* Super+Up: maximize */
            toggle_maximize(focused);
        } else if (sym == XK_Down && focused != None) {
            /* Super+Down: restore or minimize */
            Client *c = find_client(focused);
            if (c && c->is_maximized)
                toggle_maximize(focused);
            else
                minimize_window(focused);
        } else if (sym == XK_d || sym == XK_D) {
            /* Super+D: show desktop */
            toggle_show_desktop();
        } else if (sym == XK_e || sym == XK_E) {
            /* Super+E: open file manager */
            if (fork() == 0) {
                execlp("blazeneuro-files", "blazeneuro-files", NULL);
                exit(0);
            }
        } else if (sym == XK_l || sym == XK_L) {
            /* Super+L: lock screen */
            if (fork() == 0) {
                execlp("loginctl", "loginctl", "lock-session", NULL);
                exit(0);
            }
        }
    }
}

/* ── Enter Notify — focus follows mouse ─────────────────── */
static void handle_enter(XCrossingEvent *ev) {
    if (ev->window != root) {
        Client *c = find_client(ev->window);
        if (c && !c->is_minimized) {
            /* Only set input focus, don't raise */
            XSetInputFocus(dpy, ev->window, RevertToPointerRoot, CurrentTime);
            set_active(ev->window);
        }
    }
}

/* ── Signal Handler ─────────────────────────────────────── */
static void sigchld_handler(int sig) {
    (void)sig;
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

static void sigterm_handler(int sig) {
    (void)sig;
    running = 0;
}

/* ── Manage Existing Windows ────────────────────────────── */
static void scan_existing(void) {
    Window d1, d2, *wins = NULL;
    unsigned int n;

    if (XQueryTree(dpy, root, &d1, &d2, &wins, &n)) {
        for (unsigned int i = 0; i < n; i++) {
            XWindowAttributes wa;
            if (XGetWindowAttributes(dpy, wins[i], &wa) &&
                wa.map_state == IsViewable && !wa.override_redirect) {
                XSetWindowBorderWidth(dpy, wins[i], 0);
                XSelectInput(dpy, wins[i], EnterWindowMask | FocusChangeMask |
                             PropertyChangeMask | StructureNotifyMask);
                XGrabButton(dpy, 1, Mod1Mask, wins[i], True,
                            ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
                            GrabModeAsync, GrabModeAsync, None, None);
                XGrabButton(dpy, 3, Mod1Mask, wins[i], True,
                            ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
                            GrabModeAsync, GrabModeAsync, None, None);
                add_client(wins[i]);
            }
        }
        if (wins) XFree(wins);
    }
}

/* ── Main ───────────────────────────────────────────────── */
static int xerror(Display *d, XErrorEvent *ev) {
    (void)d;
    char msg[256];
    XGetErrorText(d, ev->error_code, msg, sizeof(msg));
    fprintf(stderr, "BlazeNeuro WM: X error: %s (request %d, resourceid %ld)\n",
            msg, ev->request_code, ev->resourceid);
    return 0;
}

static int xerror_start(Display *d, XErrorEvent *ev) {
    (void)d; (void)ev;
    fprintf(stderr, "BlazeNeuro WM: Another window manager is running!\n");
    exit(1);
    return -1;
}

int main(void) {
    dpy = XOpenDisplay(NULL);
    if (!dpy) {
        fprintf(stderr, "BlazeNeuro WM: Cannot open display\n");
        return 1;
    }

    /* Check for other WMs */
    XSetErrorHandler(xerror_start);
    screen = DefaultScreen(dpy);
    root = RootWindow(dpy, screen);
    sw = DisplayWidth(dpy, screen);
    sh = DisplayHeight(dpy, screen);

    XSelectInput(dpy, root,
                 SubstructureRedirectMask | SubstructureNotifyMask |
                 ButtonPressMask | PointerMotionMask |
                 PropertyChangeMask | KeyPressMask |
                 EnterWindowMask);
    XSync(dpy, False);
    XSetErrorHandler(xerror);

    /* Set cursor */
    Cursor cursor = XCreateFontCursor(dpy, XC_left_ptr);
    XDefineCursor(dpy, root, cursor);

    /* Setup */
    signal(SIGCHLD, sigchld_handler);
    signal(SIGTERM, sigterm_handler);
    signal(SIGINT, sigterm_handler);

    setup_ewmh();

    /* Grab Alt key shortcuts */
    XGrabKey(dpy, XKeysymToKeycode(dpy, XK_F4), Mod1Mask, root, True,
             GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, XKeysymToKeycode(dpy, XK_Tab), Mod1Mask, root, True,
             GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, XKeysymToKeycode(dpy, XK_space), Mod1Mask, root, True,
             GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, XKeysymToKeycode(dpy, XK_Return), Mod1Mask, root, True,
             GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, XKeysymToKeycode(dpy, XK_F9), Mod1Mask, root, True,
             GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, XKeysymToKeycode(dpy, XK_F10), Mod1Mask, root, True,
             GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, XKeysymToKeycode(dpy, XK_F11), Mod1Mask, root, True,
             GrabModeAsync, GrabModeAsync);

    /* Grab Super key shortcuts */
    XGrabKey(dpy, XKeysymToKeycode(dpy, XK_Left), Mod4Mask, root, True,
             GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, XKeysymToKeycode(dpy, XK_Right), Mod4Mask, root, True,
             GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, XKeysymToKeycode(dpy, XK_Up), Mod4Mask, root, True,
             GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, XKeysymToKeycode(dpy, XK_Down), Mod4Mask, root, True,
             GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, XKeysymToKeycode(dpy, XK_d), Mod4Mask, root, True,
             GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, XKeysymToKeycode(dpy, XK_D), Mod4Mask, root, True,
             GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, XKeysymToKeycode(dpy, XK_e), Mod4Mask, root, True,
             GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, XKeysymToKeycode(dpy, XK_E), Mod4Mask, root, True,
             GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, XKeysymToKeycode(dpy, XK_l), Mod4Mask, root, True,
             GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, XKeysymToKeycode(dpy, XK_L), Mod4Mask, root, True,
             GrabModeAsync, GrabModeAsync);

    scan_existing();

    printf("BlazeNeuro WM started (%dx%d)\n", sw, sh);
    printf("  Alt+Tab: switch windows\n");
    printf("  Alt+F4: close | Alt+F9: minimize | Alt+F10: maximize | Alt+F11: fullscreen\n");
    printf("  Super+Left/Right: snap | Super+Up: maximize | Super+D: show desktop\n");
    printf("  Super+E: files | Super+L: lock | Alt+Space: launcher | Alt+Enter: terminal\n");

    /* Event loop */
    XEvent ev;
    while (running) {
        XNextEvent(dpy, &ev);
        switch (ev.type) {
            case MapRequest:       handle_map_request(&ev.xmaprequest); break;
            case ConfigureRequest: handle_configure_request(&ev.xconfigurerequest); break;
            case UnmapNotify:      handle_unmap(&ev.xunmap); break;
            case DestroyNotify:    handle_destroy(&ev.xdestroywindow); break;
            case ButtonPress:      handle_button_press(&ev.xbutton); break;
            case ButtonRelease:    handle_button_release(&ev.xbutton); break;
            case MotionNotify:     handle_motion(&ev.xmotion); break;
            case KeyPress:         handle_key_press(&ev.xkey); break;
            case EnterNotify:      handle_enter(&ev.xcrossing); break;
            case ClientMessage:    handle_client_message(&ev.xclient); break;
        }
    }

    XCloseDisplay(dpy);
    return 0;
}
