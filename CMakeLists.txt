cmake_minimum_required(VERSION 3.21)
project(desktop_companion VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Quick QuickControls2 Gui Widgets Network Test)
qt_standard_project_setup()

add_subdirectory(third_party/stacklog)
add_subdirectory(third_party/singleproc)

qt_add_executable(desktop_companion WIN32
    src/main.cpp
    src/chatengine.h src/chatengine.cpp
    src/chatservice.h src/chatservice.cpp
    src/configstore.h src/configstore.cpp
    src/llmclient.h src/llmclient.cpp
    src/personastore.h src/personastore.cpp
    src/sseparser.h src/sseparser.cpp
    src/bridge.h src/bridge.cpp
)

target_compile_definitions(desktop_companion PRIVATE
    DESKTOP_COMPANION_VERSION="${PROJECT_VERSION}"
)

qt_add_resources(desktop_companion "qml_sources"
    PREFIX "/qml"
    BASE "qml"
    FILES
        qml/Pet.qml
        qml/Chat.qml
        qml/Bubble.qml
        qml/Manager.qml
)

qt_add_resources(desktop_companion "app_resources"
    PREFIX "/"
    BASE "resources"
    FILES resources/pet.png
)

target_link_libraries(desktop_companion PRIVATE
    Qt6::Quick Qt6::QuickControls2 Qt6::Gui Qt6::Widgets Qt6::Network
    stacklog::stacklog
    singleproc::singleproc
)

install(TARGETS desktop_companion RUNTIME DESTINATION .)

# 单元测试：与产品分开构建，不进入安装与打包。
enable_testing()
qt_add_executable(test_chatengine tests/test_chatengine.cpp tests/test_chatengine.h
    src/chatengine.h src/chatengine.cpp)
target_include_directories(test_chatengine PRIVATE src)
target_link_libraries(test_chatengine PRIVATE Qt6::Test Qt6::Core)
add_test(NAME chatengine COMMAND test_chatengine)

qt_add_executable(test_sse tests/test_sse.cpp tests/test_sse.h src/sseparser.h src/sseparser.cpp)
target_include_directories(test_sse PRIVATE src)
target_link_libraries(test_sse PRIVATE Qt6::Test Qt6::Core)
add_test(NAME sse COMMAND test_sse)

qt_add_executable(test_config tests/test_config.cpp tests/test_config.h
    src/configstore.h src/configstore.cpp
    src/personastore.h src/personastore.cpp)
target_include_directories(test_config PRIVATE src)
target_link_libraries(test_config PRIVATE Qt6::Test Qt6::Core)
add_test(NAME config COMMAND test_config)
