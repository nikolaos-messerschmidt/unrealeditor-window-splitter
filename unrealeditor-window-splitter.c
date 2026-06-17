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

    //printf("Window split for: 0x%lx\n", win); //uncomment if you want to see when windows split :)
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

    transient_atom = XInternAtom(dpy, "WM_TRANSIENT_FOR", False);
    pid_atom = XInternAtom(dpy, "_NET_WM_PID", False);

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

        while (XPending(dpy)) {
            XEvent ev;
            XNextEvent(dpy, &ev);

            Window win = 0;

            if (ev.type == CreateNotify)
                win = ev.xcreatewindow.window;
            else if (ev.type == MapNotify)
                win = ev.xmap.window;
            else
                continue;

            process_window(win);
        }

        usleep(50000);
    }

    XCloseDisplay(dpy);
    return 0;
}

//Nikolaos Messerschmidt 17:15 17.06.26