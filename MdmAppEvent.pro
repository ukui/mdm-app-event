QT       += core gui KWindowSystem dbus

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0
PKGCONFIG+=glib-2.0
LIBS+=-lglib-2.0
CONFIG += no_keywords link_pkgconfig

CONFIG += link_pkgconfig no_keywords c++11 lrelease hide_symbols
PKGCONFIG += glib-2.0 gio-2.0 gio-unix-2.0 poppler-qt5 gsettings-qt udisks2 libnotify libcanberra

SOURCES += \
    main.cpp \
    mdm_app_event.cpp

HEADERS += \
    mdm_app_event.h

TARGET = MdmAppEvent

target.source += $$TARGET
target.path = /usr/sbin
#service.files += etc/com.mdm.app.event.service
#service.path = /lib/systemd/system/
config.files += etc/com.mdm.app.event.conf
config.path = /etc/dbus-1/system.d/

INSTALLS += target service config
# Default rules for deployment.
#qnx: target.path = /tmp/$${TARGET}/bin
#else: unix:!android: target.path = /opt/$${TARGET}/bin
#!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    etc/com.mdm.app.event.conf \
    etc/com.mdm.app.event.desktop \
    etc/com.mdm.app.event.service
