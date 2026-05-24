QT += core gui widgets sql charts printsupport network

CONFIG += c++17
QT += core gui serialport

# Sources
SOURCES += \
    arduino.cpp \
    connection.cpp \
    fournisseur.cpp \
    gestionclient.cpp \
    gestionemployee.cpp \
    gestionventes.cpp \
    gestionstock.cpp \
    lignevente.cpp \
    opticalstore.cpp \
    main.cpp \
    qrcodegen.cpp

# Headers
HEADERS += \
    arduino.h \
    connection.h \
    fournisseur.h \
    gestionclient.h \
    gestionemployee.h \
    gestionventes.h \
    gestionstock.h \
    lignevente.h \
    opticalstore.h \
    qrcodegen.hpp

# UI Forms
FORMS += \
    opticalstore.ui

# Resources
RESOURCES += \
    resources.qrc

# Ignore Qt Creator user files
DISTFILES += \
    *.pro.user \
    .gitignore

# Deployment rules (optional)
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
