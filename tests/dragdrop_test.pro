TEMPLATE = app
TARGET = tst_dragdrop

QT += qml quick testlib widgets
CONFIG += testcase console

DEFINES += HAVE_POSIX_OPENPT HAVE_SYS_TIME_H
macx:DEFINES += HAVE_UTMPX _UTMPX_COMPAT HAVE_PTSNAME HAVE_UNLOCKPT HAVE_GRANTPT

INCLUDEPATH += $$PWD/../lib
INCLUDEPATH += $$PWD/../src

include($$PWD/../lib.pri)

SOURCES += $$PWD/tst_dragdrop.cpp
