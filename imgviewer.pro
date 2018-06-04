TEMPLATE = app
TARGET = archive_test
INCLUDEPATH += .
LIBS += -lzstd
QT += core
CONFIG += c++14 console

HEADERS += lib/FxSF/Archive.hpp lib/FxSF/ArchiveConstructor.hpp lib/FxSF/zstd++.hpp
SOURCES += lib/FxSF/Archive.cpp lib/FxSF/ArchiveConstructor.cpp lib/FxSF/zstd++.cpp


# Input
HEADERS += cli/core_routines.hpp cli/qt_file_utils.hpp
SOURCES += cli/core_routines.cpp cli/qt_file_utils.cpp cli/main.cpp

# Generate both debug and release on Linux (disabled)
CONFIG += debug_and_release

# Position of binaries and build files
Release:DESTDIR = release
Release:UI_DIR = release/.ui
Release:OBJECTS_DIR = release/.obj
Release:MOC_DIR = release/.moc
Release:RCC_DIR = release/.qrc

Debug:DESTDIR = debug
Debug:UI_DIR = debug/.ui
Debug:OBJECTS_DIR = debug/.obj
Debug:MOC_DIR = debug/.moc
Debug:RCC_DIR = debug/.qrc
