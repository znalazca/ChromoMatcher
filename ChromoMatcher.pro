# Copyright (C) 2018 Dzmitry Hatouka (Gotowka). htotatut@gmail.com

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program. See COPYING.txt
# If not, see <https://www.gnu.org/licenses/>.

# Please note that this program has CLA (Contributor Licence Agreement).
# You accept it by making any contribution to this program.
# See file CONTRIBUTE.txt for more details.
# List of 3-rd party components and their licenses is in 3RDPARTIES.txt

#-------------------------------------------------
#
# Project created by QtCreator 2018-03-26T21:03:21
#
#-------------------------------------------------

QT       += core gui sql charts

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ChromoMatcher
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
        main.cpp \
        mainwindow.cpp \
    addpersondialog.cpp \
    addpersonthread.cpp \
    about.cpp \
    import.cpp \
    controlgroups.cpp \
    datum.cpp \
    admixture.cpp \
    addsegment.cpp

HEADERS += \
        mainwindow.h \
    addpersondialog.h \
    addpersonthread.h \
    about.h \
    import.h \
    controlgroups.h \
    datum.h \
    admixture.h \
    addsegment.h

FORMS += \
        mainwindow.ui \
    addpersondialog.ui \
    about.ui \
    import.ui \
    controlgroups.ui \
    admixture.ui \
    addsegment.ui

# Do not forget to compile translation and put .qm file in destination folder - otherwise translation will not work.
TRANSLATIONS += ChromoMatcher_ru_RU.ts
DISTFILES += ChromoMatcher_ru_RU.ts

win32:RC_ICONS += cm.ico

# You need zlib and quazip libraries to build this project.
# Here you may find a detailed guide how to do it:
# http://www.antonioborondo.com/2014/10/22/zipping-and-unzipping-files-with-qt/
INCLUDEPATH += "$$PWD/lib/zlib-1.2.11"
LIBS += -L"$$PWD/lib/zlib-1.2.11" -lz
INCLUDEPATH += "$$PWD/lib/quazip-0.7.3/quazip"
LIBS += -L"$$PWD/lib/quazip-0.7.3/quazip" -lquazip
