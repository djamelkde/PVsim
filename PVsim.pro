#-------------------------------------------------
#
# Project created by QtCreator 2015-02-05T15:48:41
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

win32 {
    include(C:/Qt/Qwt/6.1.2/features/qwt.prf)
}

unix {
    include(/usr/local/qwt-6.1.2/features/qwt.prf)
}

TARGET = PVsim
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    Afficheur/afficheur.cpp \
    ControlleurMPPT/controlleurmppt.cpp \
    GPV/climatsimulator.cpp \
    GPV/panneau.cpp \
    GPV/panneaumanager.cpp \
    Algorithme/perturb_and_observe.cpp \
    Algorithme/algorithme_genetique.cpp \
    Algorithme/acomppttracker.cpp \
    Algorithme/icmppttracker.cpp \
    Algorithme/psomppttracker.cpp

HEADERS  += mainwindow.h \
    Afficheur/afficheur.h \
    ControlleurMPPT/controlleurmppt.h \
    GPV/climatsimulator.h \
    GPV/icourant_tension.h \
    GPV/icourbe.h \
    GPV/panneau.h \
    GPV/panneaumanager.h \
    Algorithme/imppt.h \
    ControlleurMPPT/impp.h \
    Algorithme/perturb_and_observe.h \
    Algorithme/algorithme_genetique.h \
    Algorithme/acomppttracker.h \
    Algorithme/icmppttracker.h \
    Algorithme/psomppttracker.h

FORMS    += mainwindow.ui \
    about.ui

RESOURCES += \
    resource.qrc

CONFIG += c++11
CONFIG += static

QMAKE_LFLAGS += /INCREMENTAL:NO
#QMAKE_CXXFLAGS += /Wall

RC_ICONS = img/icon.ico
