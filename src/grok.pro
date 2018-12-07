# Automatically generated by qmake, then modified manually by tjm

TEMPLATE = app
TARGET = grok

# Replacement for old GBIN/GLIB
!defined(PREFIX, var) {
	PREFIX = $$[QT_INSTALL_PREFIX]
}

QT = core gui widgets printsupport

# This probably can't be done portably
# Both qmake and the shell strip quotes from this
#DEFINES += LIB=\\\"$${PREFIX}/share\\\"
DEFINES += "LIB=\"\\\"$${PREFIX}/share/grok\\\"\"" PATH=LIB
# Needed for private interface to retrieve standard button names
INCLUDEPATH += $$[QT_INSTALL_HEADERS]/QtGui/$$QT_VERSION \
	       $$[QT_INSTALL_HEADERS]/QtGui/$$QT_VERSION/QtGui \
               $$[QT_INSTALL_HEADERS]/QtCore/$$QT_VERSION \
	       $$[QT_INSTALL_HEADERS]/QtCore/$$QT_VERSION/QtCore

CONFIG += yacc
# default yacc on gentoo defines yyparse with C linkage, but I use C++
# http://dinosaur.compilertools.net/#yacc  version 1.9.1
# byacc works correctly, but isn't reentrant (even with -P and YYPURE=1)
# https://invisible-island.net/byacc/byacc.html version 20180609
#QMAKE_YACC = byacc
# bison works correctly, and is reentrant version 3.2.1 (and probably eariler)
QMAKE_YACC = bison -y

# compile C as if it were C++ so I don't have to rename all the sources
# it should be possible to override extensions to do the same, but no
#QMAKE_EXT_CXX += .c
#QMAKE_EXT_C -= .c

QMAKE_CC = $$QMAKE_CXX
QMAKE_CFLAGS = $$QMAKE_CXXFLAGS

# The following define makes your compiler warn you if you use any
# feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# Input
SOURCES += canvdraw.c cardwin.c chart.c chartdrw.c convert.c dbase.c dbfile.c \
           editwin.c eval.c evalfunc.c formfile.c formop.c formwin.c help.c \
	   main.c mainwin.c popup.c prefwin.c query.c \
	   querywin.c sectwin.c sumwin.c template.c templmk.c templwin.c \
	   util.c HtmlCssUtils.cpp
YACCSOURCES += parser.y
HEADERS = bm_icon.h bm_left.h bm_right.h config.h form.h grok.h proto.h \
          resource.h version.h layout-qss.h chart-widget.h canv-widget.h \
	  ../misc/Grok.xpm HtmlCssUtils.hpp saveprint.h

# Install target; should probably depend on unix but right now, everything does
target.path = $${PREFIX}/bin
help.files = ../misc/grok.hlp
help.path = $${PREFIX}/share/grok
man.files = ../man/grok.1
man.path = $${PREFIX}/share/man/man1
doc.path = $${PREFIX}/share/doc/grok
doc.files = ../README.md ../TODO.md ../HISTORY
demo.files = ../demo/*
demo.path = $${PREFIX}/share/grok/grokdir
icon.path = $${PREFIX}/share/icons
icon.files = ../misc/Grok.xpm
desktop.path = $${PREFIX}/share/applications
desktop.files = grok.desktop
INSTALLS += target help man doc icon desktop demo
