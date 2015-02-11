/********************************************************************
 KWindowSystem
 This file is part of the KDE project.

Copyright (C) 2014 Thomas Lübking <thomas.luebking@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/

#include <iostream>

#include <QApplication>
#include <QDesktopWidget>
#include <QDialog>

#include <QMetaObject>
#include <QMouseEvent>
#include <QX11Info>

#include <KWindowSystem>
#include <NETRootInfo>


class WindowPicker : public QDialog
{
public:
    WindowPicker() : QDialog(0, Qt::X11BypassWindowManagerHint) {
        setGeometry(-1000,-1000,1,1);
        exec();
    }
    WId pick() {
        QList<WId> stack = KWindowSystem::stackingOrder();
        for (int i = stack.count()-1; i > -1; --i) {
            if (KWindowInfo(stack.at(i), NET::WMFrameExtents|NET::WMGeometry).frameGeometry().contains(spot))
                return stack.at(i);
        }
        return 0;
    }
    QPoint spot;
protected:
    void focusInEvent(QFocusEvent *fe) {
        QDialog::focusInEvent(fe);
        grabMouse(Qt::CrossCursor);
    }
    void showEvent(QShowEvent *e) {
        QDialog::showEvent(e);
        setFocus();
    }
    void mouseReleaseEvent(QMouseEvent *me) {
        spot = me->globalPos();
        close();
    }
};


#ifdef Q_OS_WIN32 // windows of course needs an extra invitation.
    #include <io.h>
    #define IS_A_TTY(_I_) _isatty(_I_)
#else
    #include <unistd.h>
    #define IS_A_TTY(_I_) isatty(_I_)
#endif

#define CHAR(_S_) _S_.toLocal8Bit().data()

#define FINISH a.processEvents(); exit(0)
#define INFO(_C_, _F_) if (command == _C_) { std::cout << CHAR(toString(KWindowSystem::_F_())) << std::endl; FINISH; }
#define REQUIRE_WID if (argc < 3) printHelp("nowindow", command); const int wid = window(QString::fromLocal8Bit(argv[2]))
#define WIN_FUNC(_C_, _F_) if (command == _C_) { REQUIRE_WID; KWindowSystem::_F_(wid); FINISH; }

void printHelp(QString topic = QString(), QString parameter = QString())
{
    static const char *setHelp = "* set <window id> desktop <desktop id>|all\n  sets a window on a particular or \"all\" virtual desktops\n"
        "* set <window id> urgent\n  make window blink in the taskbar\n"
        "* unset <window id> urgent\n  stop window blinking in the taskbar\n"
        "* set <window id> <states>\n"
        "* unset <window id> <states>\n"
        "* toggle <window id> <states>\n"
        "  sets or unsets certain states of a window. <states> is a comma separated list.\n";
    static const char *statesHelp = "    valid states are:\n    ------------\n"
        "    sticky,maximized,maximized_vertically,maximized_horizontally,shaded,skiptaskbar,skippager,hidden,fullscreen,keepabove,keepbelow,minimized";
    static const char *deskHelp = "Virtual desktop management\n          ---------\n"
        "* desktop list\n  print list of virtual desktops \"n: <name>\"\n\n"
        "* desktop active\n  print the number of the currently active virtual desktop\n"
        "* desktop activate <desktop id>\n  move to a virtual desktop\n\n"

        "* desktop count\n  print the amount of virtual desktops\n"
        "* desktop setCount <number>\n  set amount of virtual desktops\n\n"

        "* desktop showing\n  print true or false, depending on whether the desktop is currently shown\n"
        "* desktop show\n"
        "* desktop hide\n\n"

        "* desktop name <number>\n  print the name of a virtual desktop\n"
        "* desktop rename <desktop id> <new name>\n\n"

        "* desktop move <desktop id> <desktop id>\n  move desktop to another position\n"
        "* desktop swap <desktop id> <desktop id>\n  swap number of position desktops\n\n"

        "* desktop add [<desktop id> [<new name>]]\n  add a virtual desktop at the optional position of <desktop id> with the optional name <new name>\n"
        "   (windows and names of present desktops are preserved)\n"
        "* desktop remove <desktop id>\n  remove a virtual desktop (windows and names of other remaining desktops are preserved)\n"
        "        ---------\n"
        "The <desktop id> must refer to either a valid number (counting starts with 1) or the name of a virtual desktop\n"
        "NOTICE that if it is a name, ONLY THE FIRST matching (exact and case sensitive!) virtual desktop is affected";
    if (topic.isEmpty() || topic == "unknowncommand") {
        std::cout << "\nUsage:\n-------------------------------\n"
        "* isComposited\n  print true or false, depending on whether a compositor is active\n"
        "* active\n  print the currently active <window id>\n"
        "* id [active]\n  print the id of the active or to be picked window\n\n"
        "* activate <windowid>\n"
        "* lower <windowid>\n"
        "* raise <windowid>\n"
        "* minimize <windowid>\n"
        "* unminimize <windowid>\n"
        "* close <windowid>\n"
        << setHelp << statesHelp <<
        "\n\n\n  " << deskHelp << std::endl;
    } else if (topic == "falsedesk") {
        std::cout << "No such desktop: " << CHAR(parameter) << std::endl;
    } else if (topic == "falsewindow") {
        std::cout << "No such window: " << CHAR(parameter) << std::endl;
    } else if (topic == "nowindow") {
        std::cout << "The (sub)command " << CHAR(parameter) << " expects a window ID or \"active\" as next argument" << std::endl;
    } else if (topic == "desktop") {
        std::cout << deskHelp << std::endl;
    } else if (topic == "deskcount") {
        std::cout << "\"desktop setCount <NUMBER>\" expects a number > 0 as parameter" << std::endl;
    } else if (topic == "set") {
        std::cout << setHelp << statesHelp << std::endl;
    } else if (topic == "states") {
        std::cout << statesHelp << std::endl;
    } else if (topic == "setdesktop") {
        std::cout << "\"set <window id> desktop <DESKTOP ID>\" expects the number or name of a desktop as last parameter" << std::endl;
    } else if (topic == "setgeometry") {
        std::cout << "\"set <window id> geometry <GEOMETRY>\" expects an X11 conformant geometry string [=][<width>{xX}<height>][{+-}<xoffset>{+-}<yoffset>]" << std::endl;
    } else {
        std::cout << "error: " << CHAR(topic) << std::endl;
    }

    exit(1);
}

inline QString toString(WId w) { return QString::number(w); }
inline QString toString(int i) { return QString::number(i); }
inline QString toString(bool b) { return b ? "true" : "false"; }
inline QString toString(const QRect &r) {
    QString ret = QString::number(r.width()) + "x" + QString::number(r.width());
    if (r.x() > -1)
        ret += '+';
    ret += QString::number(r.x());
    if (r.y() > -1)
        ret += '+';
    ret += QString::number(r.y());
    return ret;
}

int virtualDesktop(QString deskId)
{
    bool ok;
    int desk = deskId.toInt(&ok);
    if (!ok) {
        desk = 0;
        for (int i = 1; i <= KWindowSystem::numberOfDesktops(); ++i) {
            if (KWindowSystem::desktopName(i) == deskId) {
                desk = i;
                break;
            }
        }
    }
    return desk < 0 ? 0 : desk;
}

WId window(QString string)
{
    bool ok;
    WId wid = string.toUInt(&ok);
    if (!ok) {
        if ((ok = (string == "active"))) {
            return KWindowSystem::activeWindow();
        } else if ((ok = (string == "pick"))) {
            return WindowPicker().pick();
        } else {
            const QList<WId> stack = KWindowSystem::stackingOrder();
            QList<WId> className, title, classPartial, classNamePartial, titlePartial;
            foreach (const WId &wid, stack) {
                KWindowInfo info(wid, NET::WMVisibleName, NET::WM2WindowClass);
                if (!string.compare(info.windowClassClass(), Qt::CaseInsensitive))
                    return wid; // matches class - very good
                if (!string.compare(info.windowClassName(), Qt::CaseInsensitive))
                    className << wid;
                else if (!string.compare(info.visibleName(), Qt::CaseInsensitive))
                    title << wid;
                else if (info.windowClassClass().contains(string.toLocal8Bit()))
                    classPartial << wid;
                else if (info.windowClassName().contains(string.toLocal8Bit()))
                    classNamePartial << wid;
                else if (info.visibleName().contains(string.toLocal8Bit()))
                    titlePartial << wid;
            }
            if (!className.isEmpty())
                return className.first();
            if (!title.isEmpty())
                return title.first();
            if (!classPartial.isEmpty())
                return classPartial.first();
            if (!classNamePartial.isEmpty())
                return classNamePartial.first();
            if (!titlePartial.isEmpty())
                return titlePartial.first();
        }
    }
    if (ok && KWindowSystem::hasWId(wid))
        return wid;
    printHelp("falsewindow", toString(wid));
    return 0; // for gcc - printHelp will exit(1)
}

static const char *wmTypes[16] = {
    "Normal", "Desktop", "Dock", "Toolbar", "Menu", "Dialog", "Override", "TopMenu",
    "Utility", "Splash", "DropdownMenu", "PopupMenu", "Tooltip", "Notification", "ComboBox", "DNDIcon"
};

const char *wmType(int i)
{
    if (i < 0 || i > 15)
        return "Unknown";
    return wmTypes[i];
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        printHelp();
    }

    QApplication a(argc, argv); // required to talk to the X11 server

    QString command = QString::fromLocal8Bit(argv[1]);
    INFO("active", activeWindow)
    INFO("isComposited", compositingActive)

    if (command == "list") {
        const QList<WId> stack = KWindowSystem::stackingOrder();
        foreach (const WId &wid, stack) {
            KWindowInfo info(wid, NET::WMWindowType|NET::WMVisibleName|NET::WMDesktop|NET::WMGeometry|NET::WMState|NET::XAWMState, NET::WM2WindowClass);
            std::cout << CHAR(toString(wid)) << " | " << CHAR(info.visibleNameWithState()) << " | " <<
                         info.windowClassClass().data() << " | " << info.windowClassName().data() << " | " <<
                         wmType(info.windowType(NET::AllTypesMask)) << " | " << CHAR(toString(info.desktop())) << " | "  << CHAR(toString(info.geometry())) << std::endl;
        }
        FINISH;
    }

    WIN_FUNC("activate", forceActiveWindow)
    WIN_FUNC("lower", lowerWindow)
    WIN_FUNC("raise", raiseWindow)
    WIN_FUNC("minimize", minimizeWindow)
    WIN_FUNC("unminimize", unminimizeWindow)
    if (command == "close") {
        REQUIRE_WID;
        NETRootInfo(QX11Info::display(), NET::CloseWindow).closeWindowRequest(wid);
        a.processEvents();
        FINISH;
    }

    if (command == "id") {
        WId wid = (argc > 2 && QString::fromLocal8Bit(argv[2]) == "active") ?
                    KWindowSystem::activeWindow() : WindowPicker().pick();
        std::cout << CHAR(toString(wid)) << std::endl;
        FINISH;
    }

    bool set = false, toggle = false;
    if ((set = (command == "set")) || (toggle = (command == "toggle")) || (command == "unset")) {
        if (argc < 4)
            printHelp("set");

        REQUIRE_WID;

        command = QString::fromLocal8Bit(argv[3]);
        if (set && command == "desktop") {
            if (argc < 5)
                printHelp("setdesktop");
            command = QString::fromLocal8Bit(argv[4]);
            if (command.toLower() == "all") {
                KWindowSystem::setOnAllDesktops(wid, set);
                FINISH;
            }
            const int desk = virtualDesktop(command);
            if (desk < 1 || desk > KWindowSystem::numberOfDesktops())
                printHelp("falsedesk", command);
            KWindowSystem::setOnDesktop(wid, desk);
            FINISH;
        }

        if (set && command == "geometry") {
            if (argc < 5)
                printHelp("setgeometry");
            int x,y;
            unsigned int w,h;
            int parsed = XParseGeometry(argv[4], &x, &y, &w, &h);
            if (!parsed)
                printHelp("setgeometry");

            const QRect geometry = KWindowInfo(wid, NET::WMGeometry).geometry();
            const QRect area = a.desktop()->availableGeometry(geometry.center());
            enum Gravity { Original = 0, NorthWest, North, NorthEast, West, Center, East, SouthWest, South, SouthEast, Static };
            int flags = 1 << 13; // "from tool"
            int gravity = Center;
            (parsed & WidthValue) ? flags |= 1<<10 : w = geometry.width();
            (parsed & HeightValue) ? flags |= 1<<11 : h = geometry.height();
            if (parsed & XValue) {
                gravity = West;
                flags |= 1<<8;
                if (parsed & XNegative)
                    x = area.right() - (w + x);
            }
            if (parsed & YValue) {
                gravity == West ? NorthWest : North;
                flags |= 1<<9;
                if (parsed & YNegative)
                    y = area.bottom() - (h + y);
            }
            flags |= gravity;
            NETRootInfo(QX11Info::display(), 0).moveResizeWindowRequest(wid, flags, x, y, w, h);
            FINISH;
        }

        if (command == "urgent") {
            KWindowSystem::demandAttention(wid, set);
            FINISH;
        }

        unsigned long stateMask = 0;
        QStringList states = command.simplified().split(',');
        bool error = false;
        foreach (const QString &state, states) {
            if (state.toLower() == "sticky")
                stateMask |= NET::Sticky;
            else if (state.toLower() == "maximized")
                stateMask |= (NET::MaxVert|NET::MaxHoriz);
            else if (state.toLower() == "maximized_vertically")
                stateMask |= NET::MaxVert;
            else if (state.toLower() == "maximized_horizontally")
                stateMask |= NET::MaxHoriz;
            else if (state.toLower() == "shaded")
                stateMask |= NET::Shaded;
            else if (state.toLower() == "skiptaskbar")
                stateMask |= NET::SkipTaskbar;
            else if (state.toLower() == "skippager")
                stateMask |= NET::SkipPager;
            else if (state.toLower() == "hidden")
                stateMask |= NET::Hidden;
            else if (state.toLower() == "fullscreen")
                stateMask |= NET::FullScreen;
            else if (state.toLower() == "keepabove")
                stateMask |= NET::KeepAbove;
            else if (state.toLower() == "keepbelow")
                stateMask |= NET::KeepBelow;
            else if (state.toLower() == "minimized") {
                if (set || (toggle && !KWindowInfo(wid, NET::WMState|NET::XAWMState).isMinimized()))
                    KWindowSystem::minimizeWindow(wid);
                else
                    KWindowSystem::unminimizeWindow(wid);
            } else {
                error = true;
                std::cout << "Unknown state: " << CHAR(state) << std::endl;
            }
        }

        NETWinInfo info(QX11Info::display(), wid, QX11Info::appRootWindow(), NET::WMState);
        unsigned long state = info.state();

        if (set)
            state |= stateMask;
        else if (toggle)
            state ^= stateMask;
        else
            state &= ~stateMask;

        info.setState(state, stateMask);

        if (error)
            printHelp("states");

        FINISH;
    }

    if (command == "desktop") {
        if (argc < 3)
            printHelp("desktop");

        command = QString::fromLocal8Bit(argv[2]);
        INFO("active", currentDesktop)
        INFO("count", numberOfDesktops)
        INFO("showing", showingDesktop)

        if (command == "list") {
            for (int i = 1; i <= KWindowSystem::numberOfDesktops(); ++i)
                std::cout << CHAR(toString(i)) << ": " << CHAR(KWindowSystem::desktopName(i)) << std::endl;
            FINISH;
        }

        if ((set = (command == "show")) || (command == "hide")) {
            const unsigned long properties[2] = {0, NET::WM2ShowingDesktop};
            NETRootInfo(QX11Info::display(), properties, 2).setShowingDesktop(set);
            FINISH;
        }

        if (command == "add") {
            const int desk = (argc > 3) ? virtualDesktop(QString::fromLocal8Bit(argv[3])) : 0;
            // extend number of desktops
            int n = KWindowSystem::numberOfDesktops() + 1;
            const int currentDesktop = KWindowSystem::currentDesktop();
            NETRootInfo(QX11Info::display(), NET::NumberOfDesktops).setNumberOfDesktops(n);
            // shift all windows on desktops above the "inserted"
            if (desk < n) { // KWS counts desktops like humans, starting by 1
                const QList<WId> stack = KWindowSystem::stackingOrder();
                foreach (const WId &wid, stack) {
                    int d = KWindowInfo(wid, NET::WMDesktop).desktop();
                    if (d >= desk)
                        KWindowSystem::setOnDesktop(wid, d+1);
                }
            }
            // rename all desktops >= the new one
            a.processEvents(); // NOTICE the count needs to be updated on the server before!
            for (int i = n; i > desk; --i)
                KWindowSystem::setDesktopName(i, KWindowSystem::desktopName(i-1));
            KWindowSystem::setDesktopName(desk, argc > 4 ? QString::fromLocal8Bit(argv[4]) : QString("Desktop %1").arg(desk));
            if (desk <= currentDesktop)
                KWindowSystem::setCurrentDesktop(currentDesktop + 1);
            FINISH;
        }

        if (argc < 4)
            printHelp("desktop");

        if (command == "name") {
            const int desk = QString::fromLocal8Bit(argv[3]).toInt();
            if (desk < 1 || desk > KWindowSystem::numberOfDesktops())
                printHelp("falsedesk", argv[3]);
            std::cout << CHAR(KWindowSystem::desktopName(desk)) << std::endl;
            FINISH;
        }

        if (command == "activate") {
            const int desk = virtualDesktop(QString::fromLocal8Bit(argv[3]));
            if (desk < 1 || desk > KWindowSystem::numberOfDesktops())
                printHelp("falsedesk", argv[3]);
            KWindowSystem::setCurrentDesktop(desk);
            FINISH;
        }

        if (command == "setCount") {
            const int count = QString::fromLocal8Bit(argv[3]).toInt();
            if (count < 1)
                printHelp("deskcount");
            NETRootInfo(QX11Info::display(), NET::NumberOfDesktops).setNumberOfDesktops(count);
            FINISH;
        }

        if (command == "remove") {
            const int n = KWindowSystem::numberOfDesktops();
            const QString deskId = QString::fromLocal8Bit(argv[3]);
            int desk = virtualDesktop(deskId);
            if (desk < 1 || desk > n)
                printHelp("falsedesk", deskId);

            // shift all windows on desktops above the "removed"
            if (desk < n) {
                const QList<WId> stack = KWindowSystem::stackingOrder();
                foreach (const WId &wid, stack) {
                    int d = KWindowInfo(wid, NET::WMDesktop).desktop();
                    if (d >= desk)
                        KWindowSystem::setOnDesktop(wid, d-1);
                }
            }
            // rename all desktops >= the new one
            for (int i = desk; i < n; ++i)
                KWindowSystem::setDesktopName(i, KWindowSystem::desktopName(i+1));

            // finally, reduce number of desktops
            NETRootInfo(QX11Info::display(), NET::NumberOfDesktops).setNumberOfDesktops(n-1);
            const int cd = KWindowSystem::currentDesktop();
            if (desk < cd)
                KWindowSystem::setCurrentDesktop(cd - 1);
            FINISH;
        }

        if (argc < 5)
            printHelp("desktop");

        if (command == "move") {
            const int d1 = virtualDesktop(QString::fromLocal8Bit(argv[3]));
            if (d1 < 1 || d1 > KWindowSystem::numberOfDesktops())
                printHelp("falsedesk", argv[3]);
            const int n = KWindowSystem::numberOfDesktops();
            const int d2 = qMin(qMax(1, virtualDesktop(QString::fromLocal8Bit(argv[4]))), n);
            if (d1 == d2)
                exit(1); // same desk

            QList<WId> stack = KWindowSystem::stackingOrder();
            foreach (const WId &wid, stack) {
                const int d = KWindowInfo(wid, NET::WMDesktop).desktop();
                if (d == d1)
                    KWindowSystem::setOnDesktop(wid, d2);
                else if (d < d1 && d >= d2)
                    KWindowSystem::setOnDesktop(wid, d+1);
                else if (d > d1 && d <= d2)
                    KWindowSystem::setOnDesktop(wid, d-1);
            }

            const int cd = KWindowSystem::currentDesktop();
            if (cd == d1)
                KWindowSystem::setCurrentDesktop(d2);
            const QString buffer = KWindowSystem::desktopName(d1);
            if (d1 < d2) {
                for (int i = d1; i < d2; ++i) {
                    KWindowSystem::setDesktopName(i, KWindowSystem::desktopName(i+1));
                    if (i+1 == cd)
                        KWindowSystem::setCurrentDesktop(i);
                }
            } else {
                for (int i = d1; i > d2; --i) {
                    KWindowSystem::setDesktopName(i, KWindowSystem::desktopName(i-1));
                    if (i-1 == cd)
                        KWindowSystem::setCurrentDesktop(i);
                }
            }
            KWindowSystem::setDesktopName(d2, buffer);
            FINISH;
        }

        if (command == "swap") {
            const int d1 = virtualDesktop(QString::fromLocal8Bit(argv[3]));
            if (d1 < 1 || d1 > KWindowSystem::numberOfDesktops())
                printHelp("falsedesk", argv[3]);
            const int d2 = virtualDesktop(QString::fromLocal8Bit(argv[4]));
            if (d2 < 1 || d2 > KWindowSystem::numberOfDesktops())
                printHelp("falsedesk", argv[4]);
            if (d1 == d2)
                exit(1); // same desk

            const int cd = KWindowSystem::currentDesktop();
            if (cd == d1)
                KWindowSystem::setCurrentDesktop(d2);
            else if (cd == d2)
                KWindowSystem::setCurrentDesktop(d1);

            QList<WId> stack = KWindowSystem::stackingOrder();
            foreach (const WId &wid, stack) {
                const int d = KWindowInfo(wid, NET::WMDesktop).desktop();
                if (d == d1)
                    KWindowSystem::setOnDesktop(wid, d2);
                else if (d == d2)
                    KWindowSystem::setOnDesktop(wid, d1);
            }

            const QString buffer = KWindowSystem::desktopName(d1);
            KWindowSystem::setDesktopName(d1, KWindowSystem::desktopName(d2));
            KWindowSystem::setDesktopName(d2, buffer);
            FINISH;
        }

        if (command == "rename") {
            const int desk = virtualDesktop(QString::fromLocal8Bit(argv[3]));
            if (desk < 1 || desk > KWindowSystem::numberOfDesktops())
                printHelp("falsedesk", argv[3]);
            KWindowSystem::setDesktopName(desk, QString::fromLocal8Bit(argv[4]));
            FINISH;
        }
    }

    printHelp("unknowncommand");


// UNIMPLEMENTED -----------------------------------------------------
// -- for KWindowSystem
// static QPoint   desktopToViewport (int desktop, bool absolute)

// static QPixmap  icon (WId win, int width=-1, int height=-1, bool scale=false)
// static QPixmap  icon (WId win, int width, int height, bool scale, int flags)
// static bool     mapViewport ()
// static QString  readNameProperty (WId window, unsigned long atom)
// static void     setBlockingCompositing (WId window, bool active)
// static void     setExtendedStrut (WId win, int left_width, int left_start, int left_end, int right_width, int right_start, int right_end, int top_width, int top_start, int top_end, int bottom_width, int bottom_start, int bottom_end)
// static void     setIcons (WId win, const QPixmap &icon, const QPixmap &miniIcon)
// static void     setMainWindow (QWidget *subwindow, WId mainwindow)
// static void     setStrut (WId win, int left, int right, int top, int bottom)
// static void     setType (WId win, NET::WindowType windowType)
// static void     setUserTime (WId win, long time)


// -- for NETRootInfo()
// QSize   desktopLayoutColumnsRows () const
// NET::DesktopLayoutCorner    desktopLayoutCorner () const
// NET::Orientation    desktopLayoutOrientation () const

// void    moveResizeRequest (Window window, int x_root, int y_root, Direction direction)
// void    moveResizeWindowRequest (Window window, int flags, int x, int y, int width, int height)
// void    restackRequest (Window window, RequestSource source, Window above, int detail, Time timestamp)
// int     screenNumber() const
// void    sendPing(Window window, Time timestamp)


}