/*
 * BlazeNeuro Window Manager
 * A minimal X11 window manager with compositing support.
 * Handles window placement, focus, move/resize, and EWMH hints.
 * Delegates visual effects (blur, shadows, rounded corners) to picom.
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
static Atom net_wm_state_fullscreen, net_wm_window_type;
static Atom net_wm_window_type_dock, net_wm_window_type_dialog;
static Atom net_wm_window_type_normal, net_wm_strut;
static Atom net_wm_strut_partial, net_active_window;
static Atom net_supporting_wm_check, net_client_list;
static Atom wm_protocols, wm_delete_window, wm_state;

/* Client tracking */
#define MAX_CLIENTS 256
static Window clients[MAX_CLIENTS];
static int nclients = 0;

/* ── EWMH Setup ────────────────────────────────────────── */
static void setup_ewmh(void) {
    net_supported           = XInternAtom(dpy, "_NET_SUPPORTED", False);
    net_wm_name             = XInternAtom(dpy, "_NET_WM_NAME", False);
    net_wm_state            = XInternAtom(dpy, "_NET_WM_STATE", False);
    net_wm_state_fullscreen = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
    net_wm_window_type      = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
    net_wm_window_type_dock = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DOCK", False);
    net_wm_window_type_dialog = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DIALOG", False);
    net_wm_window_type_normal = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_NORMAL", False);
    net_wm_strut            = XInternAtom(dpy, "_NET_WM_STRUT", False);
    net_wm_strut_partial    = XInternAtom(dpy, "_NET_WM_STRUT_PARTIAL", False);
    net_active_window       = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);
    net_supporting_wm_check = XInternAtom(dpy, "_NET_SUPPORTING_WM_CHECK", False);
    net_client_list         = XInternAtom(dpy, "_NET_CLIENT_LIST", False);
    wm_protocols            = XInternAtom(dpy, "WM_PROTOCOLS", False);
    wm_delete_window        = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
    wm_state                = XInternAtom(dpy, "WM_STATE", False);

    /* Create check window */
    Window check = XCreateSimpleWindow(dpy, root, 0, 0, 1, 1, 0, 0, 0);
    XChangeProperty(dpy, check, net_supporting_wm_check, XA_WINDOW, 32,
                    PropModeReplace, (unsigned char *)&check, 1);
    XChangeProperty(dpy, root, net_supporting_wm_check, XA_WINDOW, 32,
                    PropModeReplace, (unsigned char *)&check, 1);

    const char *wm_name = "BlazeNeuro";
    XChangeProperty(dpy, check, net_wm_name,
                    XInternAtom(dpy, "UTF8_STRING", False), 8,
                    PropModeReplace, (unsigned char *)wm_name, strlen(wm_name));

    Atom supported[] = {
        net_supported, net_wm_name, net_wm_state,
        net_wm_state_fullscreen, net_wm_window_type,
        net_active_window, net_client_list,
        net_wm_strut, net_wm_strut_partial
    };
    XChangeProperty(dpy, root, net_supported, XA_ATOM, 32,
                    PropModeReplace, (unsigned char *)supported,
                    sizeof(supported) / sizeof(Atom));
}

/* ── Client Management ─────────────────────────────────── */
static void add_client(Window w) {
    if (nclients < MAX_CLIENTS) {
        clients[nclients++] = w;
        XChangeProperty(dpy, root, net_client_list, XA_WINDOW, 32,
                        PropModeReplace, (unsigned char *)clients, nclients);
    }
}

static void remove_client(Window w) {
    for (int i = 0; i < nclients; i++) {
        if (clients[i] == w) {
            memmove(&clients[i], &clients[i + 1],
                    (nclients - i - 1) * sizeof(Window));
            nclients--;
            XChangeProperty(dpy, root, net_client_list, XA_WINDOW, 32,
                            PropModeReplace, (unsigned char *)clients, nclients);
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

    /* Center new windows, reserve space for topbar(32px) and dock(72px) */
    int topbar_h = 32;
    int dock_h = 72;
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
    wc.border_width = ev->border_width;
    wc.sibling = ev->above;
    wc.stack_mode = ev->detail;
    XConfigureWindow(dpy, ev->window, ev->value_mask, &wc);
}

static void handle_unmap(XUnmapEvent *ev) {
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

static void handle_key_press(XKeyEvent *ev) {
    KeySym sym = XkbKeycodeToKeysym(dpy, ev->keycode, 0, 0);

    if (ev->state & Mod1Mask) {
        if (sym == XK_F4) {
            /* Alt+F4: close window */
            Window focused;
            int revert;
            XGetInputFocus(dpy, &focused, &revert);
            if (focused != None && focused != root)
                send_delete(focused);
        } else if (sym == XK_Tab) {
            /* Alt+Tab: cycle windows */
            if (nclients > 1) {
                /* Move first to last, focus new first */
                Window first = clients[0];
                memmove(&clients[0], &clients[1], (nclients - 1) * sizeof(Window));
                clients[nclients - 1] = first;
                focus_window(clients[0]);
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
    fprintf(stderr, "BlazeNeuro WM: X error: %s\n", msg);
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
                 PropertyChangeMask | KeyPressMask);
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

    /* Grab global keys */
    XGrabKey(dpy, XKeysymToKeycode(dpy, XK_F4), Mod1Mask, root, True,
             GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, XKeysymToKeycode(dpy, XK_Tab), Mod1Mask, root, True,
             GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, XKeysymToKeycode(dpy, XK_space), Mod1Mask, root, True,
             GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, XKeysymToKeycode(dpy, XK_Return), Mod1Mask, root, True,
             GrabModeAsync, GrabModeAsync);

    scan_existing();

    printf("BlazeNeuro WM started (%dx%d)\n", sw, sh);

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
        }
    }

    XCloseDisplay(dpy);
    return 0;
}
