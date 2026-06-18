//Hello! i hope this works for you... I found it annoying that unreal groups Windows so i made this! 
//I tested it on KDE and ArchLinux.

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static Display *dpy;
static Atom transient_atom;
static Atom pid_atom;
static Atom wm_state_atom;
static Atom net_wm_state_atom;
static Atom net_wm_state_skip_taskbar_atom;
static Atom net_wm_state_hidden_atom;
static Atom net_active_window_atom;

static Window main_window = 0;
static int main_was_iconic = 0;

int x_error_handler(Display *d, XErrorEvent *e) {
    return 0;
}

int is_ue_window(Window win) {
    XClassHint hint;
    char *name = NULL;

    int class_ok = 0;
    int name_ok = 0;

    if (XGetClassHint(dpy, win, &hint)) {
        if (hint.res_class &&
            strcasecmp(hint.res_class, "UnrealEditor") == 0)
            class_ok = 1;

        if (hint.res_name) XFree(hint.res_name);
        if (hint.res_class) XFree(hint.res_class);
    }

    if (XFetchName(dpy, win, &name)) {
        if (name && strstr(name, "Unreal"))
            name_ok = 1;
        XFree(name);
    }

    return class_ok || name_ok;
}

int has_skip_taskbar(Window win) {
    Atom type;
    int format;
    unsigned long nitems, bytes;
    unsigned char *prop = NULL;

    if (XGetWindowProperty(dpy, win, net_wm_state_atom,
        0, 64, False, XA_ATOM,
        &type, &format, &nitems, &bytes, &prop) != Success || !prop)
        return 0;

    int found = 0;
    Atom *states = (Atom *)prop;
    for (unsigned long i = 0; i < nitems; i++) {
        if (states[i] == net_wm_state_skip_taskbar_atom) {
            found = 1;
            break;
        }
    }

    XFree(prop);
    return found;
}

int is_main_window(Window win) {
    if (!is_ue_window(win)) return 0;

    char *name = NULL;
    int result = 0;

    Atom utf8_atom = XInternAtom(dpy, "_NET_WM_NAME", False);
    Atom utf8_string = XInternAtom(dpy, "UTF8_STRING", False);
    Atom type;
    int format;
    unsigned long nitems, bytes;
    unsigned char *prop = NULL;

    if (XGetWindowProperty(dpy, win, utf8_atom,
        0, 256, False, utf8_string,
        &type, &format, &nitems, &bytes, &prop) == Success && prop && nitems > 0) {
        name = strdup((char *)prop);
        XFree(prop);
    } else {
        XFetchName(dpy, win, &name);
    }

    if (name) {
        const char *suffix = "Unreal Editor";
        size_t nlen = strlen(name);
        size_t slen = strlen(suffix);
        if (nlen >= slen && strcmp(name + nlen - slen, suffix) == 0)
            result = 1;
        free(name);
    }

    return result;
}

int is_sub_window(Window win) {
    if (!is_ue_window(win)) return 0;
    return !is_main_window(win);
}

int is_iconic(Window win) {
    Atom type;
    int format;
    unsigned long nitems, bytes;
    unsigned char *prop = NULL;

    if (XGetWindowProperty(dpy, win, wm_state_atom,
        0, 2, False, wm_state_atom,
        &type, &format, &nitems, &bytes, &prop) != Success || !prop)
        return 0;

    unsigned long state = *(unsigned long *)prop;
    XFree(prop);
    return state == IconicState;
}

void minimize_all_sub_windows(Window root) {
    Window r, p, *children;
    unsigned int n;

    if (!XQueryTree(dpy, root, &r, &p, &children, &n))
        return;

    for (unsigned int i = 0; i < n; i++) {
        if (!is_ue_window(children[i])) continue;
        if (children[i] == main_window) continue;
        if (!is_sub_window(children[i])) continue;

        XIconifyWindow(dpy, children[i], DefaultScreen(dpy));
    }

    XFlush(dpy);

    if (children) XFree(children);
}

// Raise all UE sub-windows above main
void raise_all_sub_windows(Window root) {
    Window r, p, *children;
    unsigned int n;

    if (!XQueryTree(dpy, root, &r, &p, &children, &n))
        return;

    for (unsigned int i = 0; i < n; i++) {
        if (!is_ue_window(children[i])) continue;
        if (children[i] == main_window) continue;
        if (!is_sub_window(children[i])) continue;

        XRaiseWindow(dpy, children[i]);
    }

    XFlush(dpy);

    if (children) XFree(children);
}

// Restore (map) all UE sub-windows
void restore_all_sub_windows(Window root) {
    Window r, p, *children;
    unsigned int n;

    if (!XQueryTree(dpy, root, &r, &p, &children, &n))
        return;

    for (unsigned int i = 0; i < n; i++) {
        if (!is_ue_window(children[i])) continue;
        if (children[i] == main_window) continue;
        if (!is_sub_window(children[i])) continue;

        XMapWindow(dpy, children[i]);
    }

    XFlush(dpy);

    if (children) XFree(children);
}

int ue_still_alive(Window root) {
    Window r, p, *children;
    unsigned int n;

    if (!XQueryTree(dpy, root, &r, &p, &children, &n))
        return 0;

    int found = 0;

    for (unsigned int i = 0; i < n; i++) {
        if (is_ue_window(children[i])) {
            found = 1;
            break;
        }
    }

    if (children)
        XFree(children);

    return found;
}

void try_get_pid(Window win) {
    static pid_t ue_pid = -1;

    if (ue_pid != -1)
        return;

    Atom type;
    int format;
    unsigned long nitems, bytes;
    unsigned char *prop = NULL;

    if (XGetWindowProperty(dpy, win, pid_atom,
        0, 1, False, XA_CARDINAL,
        &type, &format, &nitems, &bytes, &prop) == Success && prop)
    {
        ue_pid = *(pid_t*)prop;
        XFree(prop);

        printf("Editor PID = %d\n", ue_pid);
    }
}

void process_window(Window win) {
    if (!is_ue_window(win))
        return;

    try_get_pid(win);

    XWindowAttributes attr;
    if (!XGetWindowAttributes(dpy, win, &attr))
        return;

    usleep(20000);

    XDeleteProperty(dpy, win, transient_atom);
    XFlush(dpy);

    if (main_window == 0 && is_main_window(win)) {
        main_window = win;
        printf("Main window tracked: 0x%lx\n", main_window);

        XSelectInput(dpy, main_window, PropertyChangeMask | StructureNotifyMask);
    }

}

int scan_for_ue(Window win) {
    Window r, p, *children;
    unsigned int n;

    if (!XQueryTree(dpy, win, &r, &p, &children, &n))
        return 0;

    int found = 0;

    for (unsigned int i = 0; i < n; i++) {
        if (is_ue_window(children[i])) {
            process_window(children[i]);
            found = 1;
        }
    }

    if (children)
        XFree(children);

    return found;
}

int main() {
    XSetErrorHandler(x_error_handler);

    dpy = XOpenDisplay(NULL);
    if (!dpy) {
        fprintf(stderr, "no display\n");
        return 1;
    }

    Window root = DefaultRootWindow(dpy);

    transient_atom            = XInternAtom(dpy, "WM_TRANSIENT_FOR", False);
    pid_atom                  = XInternAtom(dpy, "_NET_WM_PID", False);
    wm_state_atom             = XInternAtom(dpy, "WM_STATE", False);
    net_wm_state_atom         = XInternAtom(dpy, "_NET_WM_STATE", False);
    net_wm_state_skip_taskbar_atom = XInternAtom(dpy, "_NET_WM_STATE_SKIP_TASKBAR", False);
    net_wm_state_hidden_atom  = XInternAtom(dpy, "_NET_WM_STATE_HIDDEN", False);
    net_active_window_atom    = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);

    printf("Waiting for Editor \n");

    int tries = 0;

    while (!scan_for_ue(root)) {
        usleep(500000);
        tries++;

        if (tries > 30) {
            printf("Editor not found! bye bye :( \n");
            return 0;
        }
    }

    printf("Editor found! listening...\n");

    XSelectInput(dpy, root,
        SubstructureNotifyMask |
        StructureNotifyMask
    );

    int missing_frames = 0;

    while (1) {

        if (!ue_still_alive(root)) {
            missing_frames++;
        } else {
            missing_frames = 0;
        }

        if (missing_frames > 10) {
            printf("Editor shut down! bye bye :( \n");
            break;
        }

        if (main_window != 0) {
            int iconic_now = is_iconic(main_window);

            if (iconic_now && !main_was_iconic) {
                printf("Main window minimized, hiding sub-windows...\n");
                minimize_all_sub_windows(root);
                main_was_iconic = 1;
            } else if (!iconic_now && main_was_iconic) {
                printf("Main window restored, showing sub-windows...\n");
                restore_all_sub_windows(root);
                main_was_iconic = 0;
            }
        }

        while (XPending(dpy)) {
            XEvent ev;
            XNextEvent(dpy, &ev);

            if (ev.type == CreateNotify || ev.type == MapNotify) {
                Window win = (ev.type == CreateNotify)
                    ? ev.xcreatewindow.window
                    : ev.xmap.window;

                process_window(win);

                if (main_window == 0 && is_main_window(win)) {
                    main_window = win;
                    printf("Main window re-tracked: 0x%lx\n", main_window);
                    XSelectInput(dpy, main_window, PropertyChangeMask);
                }
            }

            else if (ev.type == ConfigureNotify) {
                if (ev.xconfigure.window == main_window) {
                    raise_all_sub_windows(root);
                }
            }

            else if (ev.type == DestroyNotify) {
                if (ev.xdestroywindow.window == main_window) {
                    printf("Main window destroyed, resetting...\n");
                    main_window = 0;
                    main_was_iconic = 0;
                }
            }

            else if (ev.type == ClientMessage) {
                XClientMessageEvent *cm = (XClientMessageEvent *)&ev;
                if (cm->message_type == net_active_window_atom) {
                    Window requester = cm->window;
                    if (is_sub_window(requester)) {
                        printf("Blocked focus-steal from sub-window 0x%lx\n", requester);
                        continue;
                    }
                }
            }
        }

        usleep(50000);
    }

    XCloseDisplay(dpy);
    return 0;
}

//Nikolaos Messerschmidt 22:19 18.06.26
