ICON = HPVCreator.icns
RC_ICONS = HPVCreator.ico
CONFIG	     -= console debug
HEADERS	     += \
    stb_dxt.h \
    stb_image.h \
    Timer.h \
    YCoCg.h \
    YCoCgDXT.h \
    HPVCreator.hpp \
    HPVHeader.hpp \
    Log.hpp \
    lz4.h \
    lz4hc.h \
    ThreadSafeContainers.hpp \
    alt_key.hpp \
    aqp.hpp \
    kuhn_munkres.hpp
SOURCES	     += \
    HPVCreator.cpp \
    YCoCg.cpp \
    YCoCgDXT.cpp \
    Log.cpp \
    lz4.c \
    lz4hc.c \
    alt_key.cpp \
    aqp.cpp \
    kuhn_munkres.cpp
HEADERS      += mainwindow.hpp
SOURCES	     += mainwindow.cpp
SOURCES	     += main.cpp
DEFINES	     += USE_CUSTOM_DIR_MODEL
QT += widgets


