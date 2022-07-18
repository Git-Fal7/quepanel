/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * qmpanel - a minimal Qt-based desktop panel
 *
 * Copyright: 2011 Razor team
 *            2020 John Lindgren
 * Authors:
 *   Alexander Sokoloff <sokoloff.a@gmail.com>
 *   John Lindgren <john@jlindgren.net>
 *
 * This program or library is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 *
 * END_COMMON_COPYRIGHT_HEADER */

#include "trayicon.h"
#include "systray.h"
#include "../../utils.h"

#include <KWindowInfo>
#include <QDebug>
#include <QPainter>
#include <QStyle>
#include <QTimer>
#include <QX11Info>

#include <X11/Xutil.h>
#include <X11/extensions/Xcomposite.h>

#define XEMBED_EMBEDDED_NOTIFY 0

static Atom xEmbed = None;
static bool xError;

int windowErrorHandler(Display * d, XErrorEvent * e)
{
    xError = true;
    if (e->error_code != BadWindow)
    {
        char str[100];
        XGetErrorText(d, e->error_code, str, sizeof(str));
        qWarning() << "Caught X error:" << str;
    }
    return 0;
}

TrayIcon::TrayIcon(Window iconId, QWidget * parent)
    : QWidget(parent),
      mIconSize(style()->pixelMetric(QStyle::PM_ButtonIconSize)),
      mIconSizeDevPx(mIconSize * devicePixelRatioF()), mIconId(iconId),
      mAppName(KWindowInfo(iconId, 0, NET::WM2WindowClass).windowClassName()),
      mDisplay(QX11Info::display())
{
    if (xEmbed == None)
        xEmbed = XInternAtom(mDisplay, "_XEMBED", false);

    setFixedSize(mIconSize, mIconSize);
}

void TrayIcon::initIcon()
{
    if (mWindowId)
        return;

    XWindowAttributes attr{};
    if (!XGetWindowAttributes(mDisplay, mIconId, &attr))
    {
        qWarning() << "Can't get icon window attrs";
        deleteLater();
        return;
    }

    XSetWindowAttributes set_attr{};
    set_attr.colormap = attr.colormap;
    set_attr.background_pixel = 0;
    set_attr.border_pixel = 0;

    auto mask = CWColormap | CWBackPixel | CWBorderPixel;
    mWindowId =
        XCreateWindow(mDisplay, winId(), 0, 0, mIconSizeDevPx, mIconSizeDevPx,
                      0, attr.depth, InputOutput, attr.visual, mask, &set_attr);

    xError = false;
    auto old = XSetErrorHandler(windowErrorHandler);
    XReparentWindow(mDisplay, mIconId, mWindowId, 0, 0);
    XSync(mDisplay, false);
    XSetErrorHandler(old);

    if (xError)
    {
        qWarning() << "Can't reparent icon window";
        deleteLater();
        return;
    }

    XEvent e{};
    e.xclient.type = ClientMessage;
    e.xclient.send_event = True;
    e.xclient.message_type = xEmbed;
    e.xclient.window = mIconId;
    e.xclient.format = 32;
    e.xclient.data.l[0] = CurrentTime;
    e.xclient.data.l[1] = XEMBED_EMBEDDED_NOTIFY;
    e.xclient.data.l[2] = 0;
    e.xclient.data.l[3] = mWindowId;
    e.xclient.data.l[4] = 0;
    XSendEvent(mDisplay, mIconId, false, 0xFFFFFF, &e);

    XSelectInput(mDisplay, mIconId, StructureNotifyMask);

    mDamage = XDamageCreate(mDisplay, mIconId, XDamageReportRawRectangles);
    XCompositeRedirectWindow(mDisplay, mWindowId, CompositeRedirectManual);

    XMapWindow(mDisplay, mIconId);
    XMapRaised(mDisplay, mWindowId);
    XResizeWindow(mDisplay, mIconId, mIconSizeDevPx, mIconSizeDevPx);

    update();
}

void TrayIcon::attemptResize()
{
    XResizeWindow(mDisplay, mIconId, mIconSizeDevPx, mIconSizeDevPx);

    update();
}

TrayIcon::~TrayIcon()
{
    if (!mWindowId)
        return;

    XSelectInput(mDisplay, mIconId, NoEventMask);

    if (mDamage)
        XDamageDestroy(mDisplay, mDamage);

    xError = false;
    auto old = XSetErrorHandler(windowErrorHandler);
    XUnmapWindow(mDisplay, mIconId);
    XReparentWindow(mDisplay, mIconId, QX11Info::appRootWindow(), 0, 0);
    XDestroyWindow(mDisplay, mWindowId);
    XSync(mDisplay, False);
    XSetErrorHandler(old);
}

void TrayIcon::showEvent(QShowEvent * event)
{
    QWidget::showEvent(event);
    QTimer::singleShot(0, this, &TrayIcon::initIcon);
}

void TrayIcon::paintEvent(QPaintEvent *)
{
    if (!mWindowId)
        return;

    XWindowAttributes attr{}, attr2{};
    if (!XGetWindowAttributes(mDisplay, mIconId, &attr) ||
        !XGetWindowAttributes(mDisplay, mWindowId, &attr2))
    {
        return;
    }

    // GtkStatusIcon calls XResizeWindow() on itself after embedding,
    // which causes a race condition with our own XResizeWindow() in
    // initIcon().  The GTK bug will probably never be fixed (since GTK2
    // is end-of-life and GtkStatusIcon is deprecated in GTK3), so try
    // to work around it by resizing the icon window once more.
    if ((attr.width != mIconSizeDevPx || attr.height != mIconSizeDevPx) &&
        !mAttemptedResize)
    {
        QTimer::singleShot(0, this, &TrayIcon::attemptResize);
        mAttemptedResize = true;
    }

    int w = std::min(attr.width, attr2.width);
    int h = std::min(attr.height, attr2.height);

    AutoPtr<XImage> ximage(
        XGetImage(mDisplay, mIconId, 0, 0, w, h, AllPlanes, ZPixmap),
        [](XImage * image) { XDestroyImage(image); });

    if (!ximage)
        return;

    QImage image((const uchar *)ximage->data, ximage->width, ximage->height,
                 ximage->bytes_per_line, QImage::Format_ARGB32_Premultiplied);
    image.setDevicePixelRatio(devicePixelRatioF());

    QRect drawRect(QPoint(), image.size() / devicePixelRatioF());
    drawRect.moveCenter(rect().center());
    QPainter painter(this);
    painter.drawImage(drawRect.topLeft(), image);
}

void TrayIcon::windowDestroyed(Window w)
{
    // damage is destroyed if its parent window was destroyed
    if (mIconId == w)
        mDamage = 0;
}
