#include "vidra/platform/window.hpp"
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/shape.h>
#include <X11/keysym.h>
#include <stdexcept>

namespace vidra::platform {
namespace {
Atom A(Display* d, const char* n) { return XInternAtom(d, n, False); }

class X11Window final : public Window {
 public:
  X11Window(Role r, const std::string& t, int w, int h) : role_(r) {
    dpy_ = XOpenDisplay(nullptr);
    if (!dpy_) throw std::runtime_error("XOpenDisplay falhou");
    const int sc = DefaultScreen(dpy_);
    del_ = A(dpy_, "WM_DELETE_WINDOW");
    prot_ = A(dpy_, "WM_PROTOCOLS");
    st_ = A(dpy_, "_NET_WM_STATE");
    XSetWindowAttributes a{};
    a.background_pixmap = None;
    a.event_mask = ExposureMask | StructureNotifyMask | PointerMotionMask | ButtonPressMask |
                   ButtonReleaseMask | KeyPressMask | KeyReleaseMask | FocusChangeMask;
    win_ = XCreateWindow(dpy_, RootWindow(dpy_, sc), 0, 0, unsigned(w), unsigned(h), 0,
                         CopyFromParent, InputOutput, CopyFromParent,
                         CWBackPixmap | CWEventMask, &a);
    XStoreName(dpy_, win_, t.c_str());
    XSetWMProtocols(dpy_, win_, &del_, 1);
    XClassHint ch{const_cast<char*>("vidra"), const_cast<char*>("Vidra")};
    XSetClassHint(dpy_, win_, &ch);
    XSizeHints sh{};
    sh.flags = PSize | PMinSize;
    sh.min_width = 420; sh.min_height = 320; sh.width = w; sh.height = h;
    XSetWMNormalHints(dpy_, win_, &sh);
    const Atom ty = A(dpy_, "_NET_WM_WINDOW_TYPE");
    const Atom val = A(dpy_, r == Role::Dialog ? "_NET_WM_WINDOW_TYPE_DIALOG"
                                               : "_NET_WM_WINDOW_TYPE_NORMAL");
    XChangeProperty(dpy_, win_, ty, XA_ATOM, 32, PropModeReplace,
                    reinterpret_cast<unsigned char*>(const_cast<Atom*>(&val)), 1);
    int xi = 0, e1 = 0, e2 = 0;
    if (XQueryExtension(dpy_, "XInputExtension", &xi, &e1, &e2)) {
      unsigned char b[XIMaskLen(XI_LASTEVENT)]{};
      XISetMask(b, XI_Motion);
      XISetMask(b, XI_ButtonPress);
      XISetMask(b, XI_ButtonRelease);
      XIEventMask m{XIAllDevices, sizeof(b), b};
      XISelectEvents(dpy_, win_, &m, 1);
    }
    XMapWindow(dpy_, win_);
    XFlush(dpy_);
  }

  ~X11Window() override {
    if (dpy_ && win_) XDestroyWindow(dpy_, win_);
    if (dpy_) XCloseDisplay(dpy_);
  }

  Role role() const override { return role_; }
  Rect geometry() const override {
    XWindowAttributes a{};
    XGetWindowAttributes(dpy_, win_, &a);
    return {a.x, a.y, a.width, a.height};
  }
  void* nativeDisplay() const override { return dpy_; }
  uint64_t nativeHandle() const override { return win_; }
  void setTitle(const std::string& t) override { XStoreName(dpy_, win_, t.c_str()); XFlush(dpy_); }
  void resize(int w, int h) override { XResizeWindow(dpy_, win_, unsigned(w), unsigned(h)); XFlush(dpy_); }
  void move(int x, int y) override { XMoveWindow(dpy_, win_, x, y); XFlush(dpy_); }
  void minimize() override { XIconifyWindow(dpy_, win_, DefaultScreen(dpy_)); XFlush(dpy_); }
  void maximize() override {
    XEvent e{};
    e.xclient.type = ClientMessage;
    e.xclient.window = win_;
    e.xclient.message_type = st_;
    e.xclient.format = 32;
    e.xclient.data.l[0] = 1;
    e.xclient.data.l[1] = long(A(dpy_, "_NET_WM_STATE_MAXIMIZED_HORZ"));
    e.xclient.data.l[2] = long(A(dpy_, "_NET_WM_STATE_MAXIMIZED_VERT"));
    XSendEvent(dpy_, RootWindow(dpy_, DefaultScreen(dpy_)), False,
               SubstructureNotifyMask | SubstructureRedirectMask, &e);
    XFlush(dpy_);
  }
  void raise() override { XRaiseWindow(dpy_, win_); XFlush(dpy_); }
  void close() override { closed_ = true; }
  void setInputRegion(const int* r, int n) override {
    XShapeCombineRectangles(dpy_, win_, ShapeInput, 0, 0,
                            reinterpret_cast<XRectangle*>(const_cast<int*>(r)), n, ShapeSet, Unsorted);
    XFlush(dpy_);
  }

  void pump(std::function<void(const Event&)> h) override {
    while (XPending(dpy_)) {
      XEvent ev;
      XNextEvent(dpy_, &ev);
      Event o{};
      switch (ev.type) {
        case MotionNotify: o.type = Event::Type::Move; o.x = float(ev.xmotion.x); o.y = float(ev.xmotion.y); break;
        case ButtonPress: o.type = Event::Type::Button; o.pressed = true; o.button = ev.xbutton.button; o.x = float(ev.xbutton.x); o.y = float(ev.xbutton.y); break;
        case ButtonRelease: o.type = Event::Type::Button; o.pressed = false; o.button = ev.xbutton.button; o.x = float(ev.xbutton.x); o.y = float(ev.xbutton.y); break;
        case KeyPress: {
          o.type = Event::Type::Key; o.pressed = true;
          o.key = int(XLookupKeysym(&ev.xkey, 0)); o.mods = ev.xkey.state;
          char b[8]{};
          const int n = XLookupString(&ev.xkey, b, sizeof(b) - 1, nullptr, nullptr);
          if (n > 0) o.text.assign(b, size_t(n));
          break;
        }
        case KeyRelease: o.type = Event::Type::Key; o.pressed = false; o.key = int(XLookupKeysym(&ev.xkey, 0)); break;
        case ConfigureNotify: o.type = Event::Type::Resize; o.x = float(ev.xconfigure.width); o.y = float(ev.xconfigure.height); break;
        case FocusIn: o.type = Event::Type::Focus; o.pressed = true; break;
        case FocusOut: o.type = Event::Type::Focus; o.pressed = false; break;
        case ClientMessage:
          if (Atom(ev.xclient.data.l[0]) == del_) { o.type = Event::Type::Close; closed_ = true; }
          else continue;
          break;
        case GenericEvent: {
          XGenericEventCookie* c = &ev.xcookie;
          if (XGetEventData(dpy_, c) && c->evtype == XI_Motion) {
            auto* d = static_cast<XIDeviceEvent*>(c->data);
            o.type = Event::Type::Move; o.x = float(d->event_x); o.y = float(d->event_y);
            XFreeEventData(dpy_, c);
          } else { if (c->data) XFreeEventData(dpy_, c); continue; }
          break;
        }
        default: continue;
      }
      h(o);
    }
  }

  bool shouldClose() const override { return closed_; }

 private:
  Display* dpy_ = nullptr;
  Window win_ = 0;
  Atom del_ = 0, prot_ = 0, st_ = 0;
  Role role_;
  bool closed_ = false;
};

} // namespace

std::unique_ptr<Window> createWindow(Role r, const std::string& t, int w, int h) {
  return std::make_unique<X11Window>(r, t, w, h);
}
} // namespace vidra::platform
