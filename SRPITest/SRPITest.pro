QT -= gui
QT += multimedia

CONFIG += c++11 console
CONFIG -= app_bundle

TARGET  = SRPITest
VERSION = 1.0.0.0

DEFINES += APP_NAME=\\\"$${TARGET}\\\" \
           APP_VERSION=\\\"$${VERSION}\\\"
           APP_DESIGNER=\\\"Russian Biometric Society\\\"

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        main.cpp

HEADERS += \
    srpihelper.h

INCLUDEPATH += $${PWD}/..

include($${PWD}/Vendor.pri)
include($${PWD}/openmp.pri)

# Following param controls who will be responsible to read audio files
CONFIG += customwav # comment this line if you want to use Qt's decoder else custom wav decoder will be used

customwav {
    DEFINES += USE_CUSTOM_WAV_DECODER
    SOURCES += qwavdecoder.cpp
    HEADERS += qwavdecoder.h
}

