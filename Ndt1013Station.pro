QT += core gui qml quick quickcontrols2
TEMPLATE = app
TARGET = Ndt1013Station
CONFIG += c++17
CONFIG -= app_bundle

!win32:error("The iRay SDK integration requires Windows x64.")
!contains(QT_ARCH, x86_64):error("Select a 64-bit Qt MSVC kit.")
!msvc:error("Select a Qt MSVC kit for this SDK project.")

# All vendor files travel with the project.
IRAY_SDK_ROOT = $$PWD/third_party/iray
!exists($$IRAY_SDK_ROOT/include/IRayFpdSys.h):error("Project-local SDK headers are missing.")
!exists($$IRAY_SDK_ROOT/bin/x64/FpdSys.dll):error("Project-local SDK runtime is missing.")
INCLUDEPATH += $$quote($$IRAY_SDK_ROOT/include)
DEFINES += QT_NO_CAST_FROM_ASCII
QMAKE_CXXFLAGS += /utf-8 /W4

SOURCES += $$PWD/src/main.cpp \
           $$PWD/src/Station.cpp
HEADERS += $$PWD/src/Station.h
HEADERS += $$files($$IRAY_SDK_ROOT/include/*.h)
RESOURCES += $$PWD/resources.qrc
OTHER_FILES += $$PWD/qml/Main.qml $$PWD/README.md
OTHER_FILES += $$files($$IRAY_SDK_ROOT/bin/x64/*)
OTHER_FILES += $$files($$IRAY_SDK_ROOT/work_dir/NDT1013LA/*, true)
OTHER_FILES += $$IRAY_SDK_ROOT/README.md $$IRAY_SDK_ROOT/manifest.json
QML_IMPORT_PATH = $$[QT_INSTALL_QML]

# Use the same application-relative SDK layout as the CMake build.
DESTDIR = $$OUT_PWD/bin
OBJECTS_DIR = $$OUT_PWD/obj
MOC_DIR = $$OUT_PWD/moc
RCC_DIR = $$OUT_PWD/rcc
sdkSource = $$shell_path($$relative_path($$IRAY_SDK_ROOT/bin/x64, $$OUT_PWD))
sdkDestination = bin\sdk
workSource = $$shell_path($$relative_path($$IRAY_SDK_ROOT/work_dir/NDT1013LA, $$OUT_PWD))
workDestination = bin\work_dir\NDT1013LA
QMAKE_POST_LINK += $$QMAKE_COPY_DIR $$shell_quote($$sdkSource) $$shell_quote($$sdkDestination)
QMAKE_POST_LINK += $$escape_expand(\n\t)
QMAKE_POST_LINK += $$QMAKE_COPY_DIR $$shell_quote($$workSource) $$shell_quote($$workDestination)
