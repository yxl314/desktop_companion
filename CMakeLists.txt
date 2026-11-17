cmake_minimum_required(VERSION 3.21)
project(desktop_companion VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Quick QuickControls2 Gui Widgets)
qt_standard_project_setup()

qt_add_executable(desktop_companion WIN32
    src/main.cpp
    src/chatengine.h src/chatengine.cpp
    src/bridge.h src/bridge.cpp
)

qt_add_resources(desktop_companion "qml_sources"
    PREFIX "/qml"
    BASE "qml"
    FILES
        qml/Pet.qml
        qml/Chat.qml
        qml/Bubble.qml
)

qt_add_resources(desktop_companion "app_resources"
    PREFIX "/"
    BASE "resources"
    FILES resources/pet.png
)

target_link_libraries(desktop_companion PRIVATE
    Qt6::Quick Qt6::QuickControls2 Qt6::Gui Qt6::Widgets
)
