######################################################################
# Automatically generated by qmake (2.01a) dim. mai 20 14:36:38 2007
######################################################################

# Comment or uncomment this line depending on whether or not you want plot support
#CONFIG += plot


TEMPLATE = app
TARGET =
DEPENDPATH += .
INCLUDEPATH += . ./src
CONFIG += qt
QT += xml

plot {
	LIBS += -lqwt-qt4
	DEFINES += GBPLOT
	HEADERS += GaussianBeamPlot.h
	SOURCES += GaussianBeamPlot.cpp
}

CODECFORTR     = UTF-8
CODECFORSRC    = UTF-8

# Input
# src
HEADERS += src/GaussianBeam.h src/Optics.h src/OpticsBench.h src/Statistics.h
SOURCES += src/GaussianBeam.cpp src/Optics.cpp src/OpticsBench.cpp
# gui
HEADERS += gui/GaussianBeamWidget.h gui/OpticsView.h gui/GaussianBeamDelegate.h \
           gui/GaussianBeamModel.h gui/Unit.h
SOURCES += gui/GaussianBeamWidget.cpp gui/OpticsView.cpp gui/GaussianBeamDelegate.cpp \
           gui/GaussianBeamModel.cpp gui/Unit.cpp gui/GaussianBeamWidgetSave.cpp gui/main.cpp
FORMS   = gui/GaussianBeamForm.ui

TRANSLATIONS = po/GaussianBeam_fr.ts
