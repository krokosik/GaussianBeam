######################################################################
# Automatically generated by qmake (2.01a) dim. mai 20 14:36:38 2007
######################################################################

# Comment or uncomment this line depending on whether or not you want plot support
#CONFIG += plot


TEMPLATE = app
TARGET =
DEPENDPATH += .
CONFIG += qt
QT += xml
QMAKE_CXXFLAGS += -pedantic -Wno-sign-compare -Wno-long-long


plot {
	LIBS += -lqwt-qt4
	DEFINES += GBPLOT
	HEADERS += gui/GaussianBeamPlot.h
	SOURCES += gui/GaussianBeamPlot.cpp
}

CODECFORTR     = UTF-8
CODECFORSRC    = UTF-8

# Input
# src
HEADERS += src/GaussianBeam.h src/Optics.h src/OpticsBench.h src/Statistics.h src/GaussianFit.h \
           src/Function.h src/OpticsFunction.h src/Cavity.h src/Utils.h
SOURCES += src/GaussianBeam.cpp src/Optics.cpp src/OpticsBench.cpp src/GaussianFit.cpp \
           src/Function.cpp src/OpticsFunction.cpp src/Cavity.cpp src/Utils.cpp
# gui
HEADERS += gui/GaussianBeamWidget.h gui/OpticsView.h gui/OpticsWidgets.h gui/GaussianBeamDelegate.h \
           gui/GaussianBeamModel.h gui/GaussianBeamWindow.h gui/Unit.h gui/Names.h
SOURCES += gui/GaussianBeamWidget.cpp gui/OpticsView.cpp gui/OpticsWidgets.cpp gui/GaussianBeamDelegate.cpp \
           gui/GaussianBeamModel.cpp gui/GaussianBeamWindow.cpp gui/Unit.cpp gui/Names.cpp \
           gui/GaussianBeamSave.cpp gui/GaussianBeamLoad.cpp gui/main.cpp
FORMS    = gui/GaussianBeamWidget.ui gui/GaussianBeamWindow.ui gui/OpticsViewProperties.ui
RESOURCES = gui/GaussianBeamRessource.qrc

TRANSLATIONS = po/GaussianBeam_fr.ts

# Generate Universal Binary for Mac OS X
macx {
	CONFIG += x86 ppc
}
