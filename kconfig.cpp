/********************************************************************
 KConfig - a CLI config tool
 This file is part of the KDE project.

Copyright (C) 2013 Thomas Lübking <thomas.luebking@gmail.com>

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
#include <KConfig>
#include <KConfigGroup>
#include <QFile>
#include <QtDebug>
// #include <QSettings>

#ifdef Q_OS_WIN32 // windows of course needs an extra invitation.
    #include <io.h>
    #define IS_A_TTY(_I_) _isatty(_I_)
#else
    #include <unistd.h>
    #define IS_A_TTY(_I_) isatty(_I_)
#endif

#define CHAR(_S_) _S_.toLocal8Bit().data()
static const char *gs_separator = "\n  ---";

void usage() {
    std::cout << "kconfig <component>[/<group>[/<subgroup>[...]]] read|write|delete|list|replace [<key>] [<value>]\n"
                "* read|get <key>\n"
                "  prints <key> in this (sub)group\n"
                "* write|set <key> <value>\n"
                "  sets <key> to <value>, informs if the key did not exist before\n"
                "* delete <key>\n"
                "  removes the <key> from the (sub)group and informs whether there was such\n"
                "* deletegroup <key>\n"
                "  removes the group <key> from the (sub)group and informs whether there was such\n"
                "* list [<key>]\n"
                "  lists all keys in the (sub)group matching the regular expression <key> (if provided)\n"
                "* replace <key> <value>\n"
                "  replaces regular expression <key> with <value>, eg. \n"
                "           kconfig MyApp/Group replace Item(.*)=Old(.*) Item\\1=New\\2\n";
}

enum Mode { Invalid = 0, Read, Write, List, Replace, Delete, DeleteGroup };

QString path(KConfigGroup group)
{
    QString ret;
    while (group.exists()) {
        ret.prepend( "/" + group.name());
        group = group.parent();
    }
    ret.prepend(group.config()->name());
    return ret;
}

void process(Mode mode, KConfigGroup &grp, QString key, QString value)
{
    switch (mode) {
    case Read:
        if (IS_A_TTY(1))
            std::cout << CHAR(key) << ": " << CHAR(grp.readEntry(key, "does not exist")) << " (" << CHAR(path(grp)) << ")" << std::endl;
        else
            std::cout << CHAR(grp.readEntry(key, ""));
        break;
    case Write: {
        if (grp.isImmutable()) {
            std::cout << "The component/group " << CHAR(path(grp)) << " cannot be modified" << std::endl;
            exit(1);
        }
        bool added = !grp.hasKey(key);
        QString oldv;
        if (!added) oldv = grp.readEntry(key);
        grp.writeEntry(key, QString(value));
        grp.sync();
        if (added)
            std::cout << "New " << CHAR(key) << ": " << CHAR(grp.readEntry(key)) << std::endl;
        else
            std::cout << CHAR(key) << ": " << CHAR(oldv) << " -> " << CHAR(grp.readEntry(key)) << std::endl;
        break;
    }
    case Delete: {
        if (grp.isImmutable()) {
            std::cout << "The component/group " << CHAR(path(grp)) << " cannot be modified" << std::endl;
            exit(1);
        }
        if (grp.hasKey(key)) {
            std::cout << "Removed " << CHAR(key) << ": " << CHAR(grp.readEntry(key)) << std::endl;
            grp.deleteEntry(key);
            grp.sync();
        } else if (grp.hasGroup(key)) {
            std::cout << "There's a group, but no key: " << CHAR(key) << "\nPlease explicitly use deletegroup" << std::endl;
            exit(1);
        } else {
            std::cout << "There's no key " << CHAR(key) << " in " << CHAR(path(grp)) << std::endl;
            exit(1);
        }
        break;
    }
    case DeleteGroup: {
        if (grp.hasGroup(key)) {
            grp = grp.group(key);
            if (grp.isImmutable()) {
                std::cout << "The component/group " << CHAR(path(grp)) << " cannot be modified" << std::endl;
                exit(1);
            }
            QMap<QString, QString> map = grp.entryMap();
            std::cout << "Removed " << CHAR(key) << gs_separator << std::endl;
            for (QMap<QString, QString>::const_iterator it = map.constBegin(), end = map.constEnd(); it != end; ++it) {
                std::cout << CHAR(it.key()) << ": " << CHAR(it.value()) << std::endl;
            }
            grp.deleteGroup();
            grp.sync();
        } else {
            std::cout << "There's no group " << CHAR(key) << " in " << CHAR(path(grp)) << std::endl;
            exit(1);
        }
        break;
    }
    case List: {
        if (!grp.exists()) { // could be parent group
            QStringList groups = grp.parent().exists() ? grp.parent().groupList() : grp.config()->groupList();
            if (groups.isEmpty()) {
                std::cout << "The component/group " << CHAR(path(grp)) << " does not exist" << std::endl;
                exit(1);
            }
            std::cout << "Groups in " << CHAR(path(grp)) << gs_separator << std::endl;
            foreach (const QString &s, groups)
                if (key.isEmpty() || s.contains(key, Qt::CaseInsensitive))
                    std::cout << CHAR(s) << std::endl;
            exit(0);
        }

        QMap<QString, QString> map = grp.entryMap();
        if (map.isEmpty()) {
            std::cout << "The group " << CHAR(path(grp)) << " is empty" << std::endl;
            break;
        }

        bool matchFound = false;
        for (QMap<QString, QString>::const_iterator it = map.constBegin(), end = map.constEnd(); it != end; ++it) {
            if (key.isEmpty() || it.key().contains(key, Qt::CaseInsensitive)) {
                if (!matchFound)
                    std::cout << std::endl << CHAR(path(grp)) << gs_separator << std::endl;
                matchFound = true;
                std::cout << CHAR(it.key()) << ": " << CHAR(it.value()) << std::endl;
            }
        }

        if (!matchFound)
            std::cout << "No present key matches \"" << CHAR(key) << "\" in " << CHAR(path(grp));
        std::cout << std::endl;
        break;
    }
    case Replace: {
        if (grp.isImmutable()) {
            std::cout << "The component/group " << CHAR(path(grp)) << " cannot be modified" << std::endl;
            exit(1);
        }
        QStringList match = key.split("=");
        if (match.count() != 2) {
            std::cout << "The match sequence must be of the form <key regexp>=<value regexp>" << std::endl;
            exit(1);
        }
        QRegExp keyMatch(match.at(0), Qt::CaseInsensitive);
        QRegExp valueMatch(match.at(1), Qt::CaseInsensitive);
        QStringList replace = value.split("=");
        if (replace.count() != 2) {
            std::cout << "The replace sequence must be of the form <key string>=<value string>" << std::endl;
            exit(1);
        }
        QMap<QString, QString> map = grp.entryMap();
        QStringList keys;
        for (QMap<QString, QString>::const_iterator it = map.constBegin(), end = map.constEnd(); it != end; ++it) {
            if (keyMatch.exactMatch(it.key()) && valueMatch.exactMatch(it.value())) {
                keys << it.key();
            }
        }
        foreach (const QString &key, keys) {
            QString newKey = key;
            newKey.replace(keyMatch, replace.at(0));
            QString newValue = grp.readEntry(key);
            const QString oldValue = newValue;
            newValue.replace(valueMatch, replace.at(1));
            if (key != newKey)
                grp.deleteEntry(key);
            grp.writeEntry(newKey, newValue);
            std::cout << CHAR(key) << ": " << CHAR(oldValue) << " -> " << CHAR(newKey) << ": " << CHAR(grp.readEntry(newKey)) << std::endl;
            grp.sync();
        }
        break;
    }
    Invalid:
    default:
        break;
    }
}
#if 0
void process(Mode mode, QSettings &set, QString key, QString value)
{
    switch (mode) {
    case Read:
        if (IS_A_TTY(1))
            std::cout << CHAR(key) << ": " << CHAR(set.value(key, "does not exist").toString()) << " (" << CHAR(path(grp)) << ")" << std::endl;
        else
            std::cout << CHAR(set.value(key, "").toString());
        break;
    case Write: {
        bool added = !set.hasKey(key);
        QString oldv;
        if (!added) oldv = set.value(key).toString();
        set.setValue(key, QString(value));
        grp.sync();
        if (added)
            std::cout << "New " << CHAR(key) << ": " << CHAR(grp.value(key).toString()) << std::endl;
        else
            std::cout << CHAR(key) << ": " << CHAR(oldv) << " -> " << CHAR(grp.value(key).toString()) << std::endl;
        break;
    }
    case Delete: {
        if (grp.hasKey(key)) {
            std::cout << "Removed " << CHAR(key) << ": " << CHAR(grp.value(key).toString()) << std::endl;
            grp.deleteEntry(key);
            grp.sync();
        } else if (grp.hasGroup(key)) {
            std::cout << "There's a group, but no key: " << CHAR(key) << "\nPlease explicitly use deletegroup" << std::endl;
            exit(1);
        } else {
            std::cout << "There's no key " << CHAR(key) << " in " << CHAR(path(grp)) << std::endl;
            exit(1);
        }
        break;
    }
    case DeleteGroup: {
        if (grp.hasGroup(key)) {
            grp = grp.group(key);
            if (grp.isImmutable()) {
                std::cout << "The component/group " << CHAR(path(grp)) << "/" << CHAR(key) << " cannot be modified" << std::endl;
                exit(1);
            }
            QMap<QString, QString> map = grp.entryMap();
            std::cout << "Removed " << CHAR(key) << gs_separator << std::endl;
            for (QMap<QString, QString>::const_iterator it = map.constBegin(), end = map.constEnd(); it != end; ++it) {
                std::cout << CHAR(it.key()) << ": " << CHAR(it.value()) << std::endl;
            }
            grp.deleteGroup();
            grp.sync();
        } else {
            std::cout << "There's no group " << CHAR(key) << " in " << CHAR(path(grp)) << std::endl;
            exit(1);
        }
        break;
    }
    case List: {
        if (!grp.exists()) { // could be parent group
            QStringList groups = grp.parent().exists() ? grp.parent().groupList() : grp.config()->groupList();
            if (groups.isEmpty()) {
                std::cout << "The component/group " << CHAR(path(grp)) << " does not exist" << std::endl;
                exit(1);
            }
            std::cout << "Groups in " << CHAR(path(grp)) << gs_separator << std::endl;
            foreach (const QString &s, groups)
                if (key.isEmpty() || s.contains(key, Qt::CaseInsensitive))
                    std::cout << CHAR(s) << std::endl;
            exit(0);
        }

        QMap<QString, QString> map = grp.entryMap();
        if (map.isEmpty()) {
            std::cout << "The group is empty" << std::endl;
            exit(1);
        }

        bool matchFound = false;
        for (QMap<QString, QString>::const_iterator it = map.constBegin(), end = map.constEnd(); it != end; ++it) {
            if (key.isEmpty() || it.key().contains(key, Qt::CaseInsensitive)) {
                matchFound = true;
                std::cout << CHAR(it.key()) << ": " << CHAR(it.value()) << std::endl;
            }
        }

        if (!matchFound)
            std::cout << "No present key matches \"" << CHAR(key) << "\" in " << CHAR(path(grp)) << std::endl;
        break;
    }
    case Replace: {
        QStringList match = key.split("=");
        if (match.count() != 2) {
            std::cout << "The match sequence must be of the form <key regexp>=<value regexp>" << std::endl;
            exit(1);
        }
        QRegExp keyMatch(match.at(0), Qt::CaseInsensitive);
        QRegExp valueMatch(match.at(1), Qt::CaseInsensitive);
        QStringList replace = value.split("=");
        if (replace.count() != 2) {
            std::cout << "The replace sequence must be of the form <key string>=<value string>" << std::endl;
            exit(1);
        }
        QMap<QString, QString> map = grp.entryMap();
        QStringList keys;
        for (QMap<QString, QString>::const_iterator it = map.constBegin(), end = map.constEnd(); it != end; ++it) {
            if (keyMatch.exactMatch(it.key()) && valueMatch.exactMatch(it.value())) {
                keys << it.key();
            }
        }
        foreach (const QString &key, keys) {
            QString newKey = key;
            newKey.replace(keyMatch, replace.at(0));
            QString newValue = grp.value(key);
            const QString oldValue = newValue;
            newValue.replace(valueMatch, replace.at(1));
            if (key != newKey)
                grp.deleteEntry(key);
            grp.writeEntry(newKey, newValue);
            std::cout << CHAR(key) << ": " << CHAR(oldValue) << " -> " << CHAR(newKey) << ": " << CHAR(grp.value(newKey).toString()) << std::endl;
            grp.sync();
        }
        break;
    }
    Invalid:
    default:
        break;
    }
}
#endif

Mode checkMode(QString modekey, int argc) {
    Mode mode(Invalid);
    if (modekey == "read" || modekey == "get") {
        if (argc < 4) {
            std::cout << "You must say what <key> to read" << std::endl;
            exit(1);
        }
        mode = Read;
    }
    if (modekey == "write" || modekey == "set") {
        if (argc < 5) {
            std::cout << "You must say what <key> to write to what <value>" << std::endl;
            exit(1);
        }
        mode = Write;
    }
    if (modekey == "delete") {
        if (argc < 4) {
            std::cout << "You must say what key to delete" << std::endl;
            exit(1);
        }
        mode = Delete;
    }
    if (modekey == "deletegroup") {
        if (argc < 4) {
            std::cout << "You must say what group to delete" << std::endl;
            exit(1);
        }
        mode = DeleteGroup;
    }
    if (modekey == "list") {
        mode = List;
    }
    if (modekey == "replace") {
        if (argc < 5) {
            std::cout << "You must say <what regexp> to replace by <what string>" << std::endl;
            exit(1);
        }
        mode = Replace;
    }
    return mode;
}

void processGroup(KConfigGroup grp, QStringList component, int next, Mode mode, QString key, QString value) {
    if (next == component.count()) {
        process(mode, grp, key, value);
        return;
    }

    const QStringList groups = grp.exists() ? grp.groupList() : grp.config()->groupList();
    const QRegExp groupMatch(component.at(next), Qt::CaseInsensitive);
    int hits = 0;
    foreach (const QString &g, groups) {
        if (groupMatch.exactMatch(g)) {
            ++hits;
            processGroup(grp.group(g), component, next + 1, mode, key, value);
        }
    }
    if (!hits) {
        std::cout << "No existing group matches " << CHAR(component.at(next)) << std::endl;
    }
}

// void processGroup(QSettings &set, QStringList component, int next, Mode mode, QString key, QString value) {
//     if (next == component.count()) {
//         process(mode, set, key, value);
//         return;
//     }
//
//     const QStringList groups = set.childGroups();
//     const QRegExp groupMatch(component.at(next), Qt::CaseInsensitive);
//     foreach (const QString &g, groups) {
//         if (groupMatch.exactMatch(g)) {
//             set.beginGroup(g);
//             processGroup(set, component, next + 1, mode, key, value);
//             set.endGroup();
//         }
//     }
// }

int main (int argc, char **argv)
{
    if (argc < 3) {
        usage();
        exit(1);
    }
    Mode mode = checkMode(QString::fromLocal8Bit(argv[2]), argc);
    if (mode == Invalid) {
        std::cout << "Unknown command: " << argv[2] << std::endl;
        exit(1);
    }

    QString file = QString::fromLocal8Bit(argv[1]);
    QStringList component = file.split('/', QString::SkipEmptyParts);
    int firstGroupIndex = 1;
    if (file.startsWith('/')) {
        file = '/' + component.at(firstGroupIndex - 1);
        while (firstGroupIndex < component.count() && QFile::exists(file + '/' + component.at(firstGroupIndex)))
            file += '/' + component.at(firstGroupIndex++);
    } else {
        file = component.at(0);
    }
    QString key, value;
    if (argc > 2)
        key = QString::fromLocal8Bit(argv[3]);
    if (argc > 3)
        value = QString::fromLocal8Bit(argv[4]);
    KConfig cfg(file);
    KConfigGroup grp = cfg.group(QString());
    processGroup(grp, component, firstGroupIndex, mode, key, value);
    exit(0);
}
