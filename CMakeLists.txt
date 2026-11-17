cmake_minimum_required(VERSION 3.21)
project(desktop_companion VERSION 0.6.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Quick QuickControls2 Gui Widgets Network Test)
find_package(SQLite3 REQUIRED)
qt_standard_project_setup()

add_subdirectory(third_party/stacklog)
add_subdirectory(third_party/singleproc)

qt_add_executable(desktop_companion WIN32
    src/main.cpp
    src/autostart.h src/autostart.cpp
    src/chatengine.h src/chatengine.cpp
    src/chatservice.h src/chatservice.cpp
    src/chatstore.h src/chatstore.cpp
    src/characterpack.h src/characterpack.cpp
    src/characterstore.h src/characterstore.cpp
    src/errorcodes.h src/errorcodes.cpp
    src/hitareameta.h src/hitareameta.cpp
    src/hitmask.h src/hitmask.cpp
    src/intimacyservice.h src/intimacyservice.cpp
    src/petwindowhelper.h src/petwindowhelper.cpp
    src/memorystore.h src/memorystore.cpp
    src/personafile.h src/personafile.cpp
    src/promptbuilder.h src/promptbuilder.cpp
    src/worldstore.h src/worldstore.cpp
    src/configstore.h src/configstore.cpp
    src/llmclient.h src/llmclient.cpp
    src/personastore.h src/personastore.cpp
    src/secretstore.h src/secretstore.cpp
    src/sseparser.h src/sseparser.cpp
    src/bridge.h src/bridge.cpp
)

if(WIN32)
    target_link_libraries(desktop_companion PRIVATE Crypt32)
endif()

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
    SQLite::SQLite3 stacklog::stacklog
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

qt_add_executable(test_chatstore tests/test_chatstore.cpp tests/test_chatstore.h
    src/chatstore.h src/chatstore.cpp)
target_include_directories(test_chatstore PRIVATE src)
target_link_libraries(test_chatstore PRIVATE Qt6::Test SQLite::SQLite3 stacklog::stacklog)
add_test(NAME chatstore COMMAND test_chatstore)

qt_add_executable(test_errorcodes tests/test_errorcodes.cpp tests/test_errorcodes.h
    src/errorcodes.h src/errorcodes.cpp)
target_include_directories(test_errorcodes PRIVATE src)
target_link_libraries(test_errorcodes PRIVATE Qt6::Test Qt6::Core)
add_test(NAME errorcodes COMMAND test_errorcodes)

qt_add_executable(test_characterpack tests/test_characterpack.cpp tests/test_characterpack.h
    src/characterpack.h src/characterpack.cpp)
target_include_directories(test_characterpack PRIVATE src)
target_link_libraries(test_characterpack PRIVATE Qt6::Test Qt6::Core)
add_test(NAME characterpack COMMAND test_characterpack)

qt_add_executable(test_personafile tests/test_personafile.cpp tests/test_personafile.h
    src/personafile.h src/personafile.cpp)
target_include_directories(test_personafile PRIVATE src)
target_link_libraries(test_personafile PRIVATE Qt6::Test Qt6::Core)
add_test(NAME personafile COMMAND test_personafile)

qt_add_executable(test_characterstore tests/test_characterstore.cpp tests/test_characterstore.h
    src/characterstore.h src/characterstore.cpp
    src/personafile.h src/personafile.cpp)
target_include_directories(test_characterstore PRIVATE src)
target_link_libraries(test_characterstore PRIVATE Qt6::Test Qt6::Core)
add_test(NAME characterstore COMMAND test_characterstore)

qt_add_executable(test_worldstore tests/test_worldstore.cpp tests/test_worldstore.h
    src/worldstore.h src/worldstore.cpp
    src/characterstore.h src/characterstore.cpp
    src/personafile.h src/personafile.cpp)
target_include_directories(test_worldstore PRIVATE src)
target_link_libraries(test_worldstore PRIVATE Qt6::Test Qt6::Core)
add_test(NAME worldstore COMMAND test_worldstore)

qt_add_executable(test_hitmask tests/test_hitmask.cpp tests/test_hitmask.h
    src/hitmask.h src/hitmask.cpp)
target_include_directories(test_hitmask PRIVATE src)
target_link_libraries(test_hitmask PRIVATE Qt6::Test Qt6::Gui)
add_test(NAME hitmask COMMAND test_hitmask)

qt_add_executable(test_hitareameta tests/test_hitareameta.cpp tests/test_hitareameta.h
    src/hitareameta.h src/hitareameta.cpp)
target_include_directories(test_hitareameta PRIVATE src)
target_link_libraries(test_hitareameta PRIVATE Qt6::Test Qt6::Core)
add_test(NAME hitareameta COMMAND test_hitareameta)

qt_add_executable(test_memorystore tests/test_memorystore.cpp tests/test_memorystore.h
    src/memorystore.h src/memorystore.cpp)
target_include_directories(test_memorystore PRIVATE src)
target_link_libraries(test_memorystore PRIVATE Qt6::Test Qt6::Core SQLite::SQLite3)
add_test(NAME memorystore COMMAND test_memorystore)

qt_add_executable(test_promptbuilder tests/test_promptbuilder.cpp tests/test_promptbuilder.h
    src/promptbuilder.h src/promptbuilder.cpp)
target_include_directories(test_promptbuilder PRIVATE src)
target_link_libraries(test_promptbuilder PRIVATE Qt6::Test Qt6::Core)
add_test(NAME promptbuilder COMMAND test_promptbuilder)

qt_add_executable(test_intimacy tests/test_intimacy.cpp tests/test_intimacy.h
    src/intimacyservice.h src/intimacyservice.cpp)
target_include_directories(test_intimacy PRIVATE src)
target_link_libraries(test_intimacy PRIVATE Qt6::Test Qt6::Core)
add_test(NAME intimacy COMMAND test_intimacy)

qt_add_executable(test_config tests/test_config.cpp tests/test_config.h
    src/autostart.h src/autostart.cpp
    src/configstore.h src/configstore.cpp
    src/personastore.h src/personastore.cpp
    src/secretstore.h src/secretstore.cpp)
target_include_directories(test_config PRIVATE src)
target_link_libraries(test_config PRIVATE Qt6::Test Qt6::Core)
if(WIN32)
    target_link_libraries(test_config PRIVATE Crypt32)
endif()
add_test(NAME config COMMAND test_config)
