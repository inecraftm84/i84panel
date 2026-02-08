#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

Window menu_win = None;

void draw_menu_content(Display *d, Window m, int hover) {
    GC mgc = XCreateGC(d, m, 0, 0);
    char *items[] = {"XTERM", "CHANGE BG", "KILL ALL"};
    XClearWindow(d, m);
    for (int i = 0; i < 3; i++) {
        if (i == hover) {
            XSetForeground(d, mgc, 0x3366FF);
            XFillRectangle(d, m, mgc, 0, i * 30, 150, 30);
            XSetForeground(d, mgc, 0xFFFFFF);
        } else {
            XSetForeground(d, mgc, 0x000000);
        }
        XDrawString(d, m, mgc, 15, 20 + (i * 30), items[i], strlen(items[i]));
    }
    XFreeGC(d, mgc);
}

int main() {
    Display *d = XOpenDisplay(NULL);
    if (!d) return 1;
    srand(time(NULL));
    int s = DefaultScreen(d), sw = DisplayWidth(d, s), sh = DisplayHeight(d, s), ph = 32;
    Window root = RootWindow(d, s);

    XSetWindowAttributes sattr;
    sattr.background_pixel = 0xDDDDDD;
    sattr.override_redirect = True;

    Window w = XCreateWindow(d, root, 0, sh - ph, sw, ph, 0, 
                             CopyFromParent, InputOutput, CopyFromParent,
                             CWBackPixel | CWOverrideRedirect, &sattr);

    XClassHint *ch = XAllocClassHint();
    ch->res_name = "i84panel";
    ch->res_class = "i84panel";
    XSetClassHint(d, w, ch);
    XFree(ch);

    Atom t = XInternAtom(d, "_NET_WM_WINDOW_TYPE", False);
    Atom dk = XInternAtom(d, "_NET_WM_WINDOW_TYPE_DOCK", False);
    XChangeProperty(d, w, t, XA_ATOM, 32, PropModeReplace, (unsigned char *)&dk, 1);
    
    long st_v[12] = {0, 0, 0, ph, 0, 0, 0, 0, 0, 0, 0, sw - 1};
    Atom st = XInternAtom(d, "_NET_WM_STRUT_PARTIAL", False);
    XChangeProperty(d, w, st, XA_CARDINAL, 32, PropModeReplace, (unsigned char *)st_v, 12);

    XSelectInput(d, w, ExposureMask | ButtonPressMask);
    XMapRaised(d, w);
    GC gc = XCreateGC(d, w, 0, 0);

    while (1) {
        while (XPending(d)) {
            XEvent ev; XNextEvent(d, &ev);
            if (ev.type == ButtonPress && ev.xany.window == w) {
                if (ev.xbutton.x < 70) {
                    if (menu_win == None) {
                        menu_win = XCreateSimpleWindow(d, root, 5, sh - ph - 90, 150, 90, 1, 0, 0xFFFFFF);
                        XSetWindowAttributes at; at.override_redirect = True;
                        XChangeWindowAttributes(d, menu_win, CWOverrideRedirect, &at);
                        XSelectInput(d, menu_win, ExposureMask | ButtonPressMask | PointerMotionMask);
                        XMapRaised(d, menu_win);
                    } else { XDestroyWindow(d, menu_win); menu_win = None; }
                } else {
                    Window r_ret, p_ret, *children; unsigned int n_win = 0;
                    XQueryTree(d, root, &r_ret, &p_ret, &children, &n_win);
                    int win_count = 0, target_idx = (ev.xbutton.x - 80) / 105;
                    for (int i = 0; i < n_win; i++) {
                        XWindowAttributes wa; XGetWindowAttributes(d, children[i], &wa);
                        char *nm; XFetchName(d, children[i], &nm);
                        if (nm && children[i] != w && children[i] != menu_win && wa.width > 50) {
                            if (win_count == target_idx) {
                                if (wa.map_state == IsViewable) XUnmapWindow(d, children[i]);
                                else XMapRaised(d, children[i]);
                                XFree(nm); break;
                            }
                            win_count++;
                        }
                        if (nm) XFree(nm);
                    }
                    if (n_win > 0) XFree(children);
                }
            }
            if (menu_win != None && ev.xany.window == menu_win) {
                if (ev.type == MotionNotify) draw_menu_content(d, menu_win, ev.xmotion.y / 30);
                if (ev.type == ButtonPress) {
                    int idx = ev.xbutton.y / 30;
                    if (idx == 0) { if (fork() == 0) { execlp("xterm", "xterm", NULL); _exit(0); } }
                    else if (idx == 1) { XSetWindowBackground(d, root, (rand() % 0xFFFFFF)); XClearWindow(d, root); }
                    else if (idx == 2) {
                        Window r_ret, p_ret, *children; unsigned int n_win = 0;
                        XQueryTree(d, root, &r_ret, &p_ret, &children, &n_win);
                        for (int i = 0; i < n_win; i++) {
                            XWindowAttributes wa; XGetWindowAttributes(d, children[i], &wa);
                            XClassHint hint;
                            int has_hint = XGetClassHint(d, children[i], &hint);
                            int protect = 0;
                            if (has_hint) {
                                if (hint.res_name && (strcmp(hint.res_name, "i84panel") == 0 || strstr(hint.res_name, "Xorg"))) protect = 1;
                                XFree(hint.res_name); XFree(hint.res_class);
                            }
                            if (children[i] != w && children[i] != menu_win && wa.width > 50 && !protect) XKillClient(d, children[i]);
                        }
                        if (n_win > 0) XFree(children);
                    }
                    XDestroyWindow(d, menu_win); menu_win = None;
                }
                if (ev.type == Expose) draw_menu_content(d, menu_win, -1);
            }
        }
        XClearWindow(d, w);
        XSetForeground(d, gc, 0xBBBBBB);
        XFillRectangle(d, w, gc, 5, 4, 65, 24);
        XSetForeground(d, gc, 0x000000);
        XDrawString(d, w, gc, 18, 21, "START", 5);

        Window r_ret, p_ret, *children; unsigned int n_win = 0;
        XQueryTree(d, root, &r_ret, &p_ret, &children, &n_win);
        int win_count = 0;
        for (int i = 0; i < n_win && win_count < 12; i++) {
            XWindowAttributes wa; XGetWindowAttributes(d, children[i], &wa);
            char *nm; XFetchName(d, children[i], &nm);
            if (nm && children[i] != w && children[i] != menu_win && wa.width > 50) {
                XSetForeground(d, gc, (wa.map_state != IsViewable) ? 0x888888 : 0x000000);
                XDrawRectangle(d, w, gc, 80 + (win_count * 105), 4, 100, 24);
                XDrawString(d, w, gc, 85 + (win_count * 105), 21, nm, (strlen(nm) > 12 ? 12 : strlen(nm)));
                XFree(nm); win_count++;
            }
        }
        if (n_win > 0) XFree(children);

        time_t raw; time(&raw);
        char ts[10]; strftime(ts, 10, "%H:%M:%S", localtime(&raw));
        XSetForeground(d, gc, 0x000000);
        XDrawString(d, w, gc, sw - 75, 21, ts, 8);
        XFlush(d);
        usleep(200000);
    }
    return 0;
}
