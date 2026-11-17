# stacklog qmake 集成文件
INCLUDEPATH += $$PWD/include
HEADERS += $$PWD/include/stacklog/stacklog.hpp $$PWD/include/stacklog/version.hpp
SOURCES += $$PWD/src/stacklog.cpp
CONFIG += c++11
unix:LIBS += -lpthread
