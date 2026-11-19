cmake_minimum_required(VERSION 3.21)
project(desktop_companion VERSION 1.1.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Quick QuickControls2 Gui Widgets Network Test LinguistTools)
find_package(SQLite3 REQUIRED)
qt_standard_project_setup()

add_subdirectory(third_party/stacklog EXCLUDE_FROM_ALL)
add_subdirectory(third_party/singleproc EXCLUDE_FROM_ALL)

qt_add_translations(desktop_companion
    TS_FILES translations/desktop_companion_en.ts
    RESOURCE_PREFIX "/i18n"
)

# ---------------------------------------------------------------------------
# 模块化目录（docs/架构重构-MVVM.md §1）
#
#   src/common/    通用设施（无业务、无 UI；当前尚无源文件）
#   src/domain/    纯值类型（无 IO、无 GUI）
#   src/data/      持久化仓库
#   src/services/  领域服务
#   src/platform/  平台/基础设施
#
# include 根固定为 src/，头文件一律带模块前缀引用（如 "data/configstore.h"）。
# Lead 后续接入 src/app/**、src/viewmodels/** 与重写后的 main.cpp 时，
# 把对应文件追加到 DESKTOP_COMPANION_CORE_SOURCES（或新增变量）即可。
# ---------------------------------------------------------------------------
set(DESKTOP_COMPANION_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(DESKTOP_COMPANION_INCLUDE_DIRS ${DESKTOP_COMPANION_SRC_DIR})

set(DESKTOP_COMPANION_DOMAIN_SOURCES
    ${DESKTOP_COMPANION_SRC_DIR}/domain/appconfig.h
    ${DESKTOP_COMPANION_SRC_DIR}/domain/character.h
    ${DESKTOP_COMPANION_SRC_DIR}/domain/chatmessage.h
    ${DESKTOP_COMPANION_SRC_DIR}/domain/intimacy.h
    ${DESKTOP_COMPANION_SRC_DIR}/domain/memoryentry.h
)

set(DESKTOP_COMPANION_DATA_SOURCES
    ${DESKTOP_COMPANION_SRC_DIR}/data/characterstore.h ${DESKTOP_COMPANION_SRC_DIR}/data/characterstore.cpp
    ${DESKTOP_COMPANION_SRC_DIR}/data/chatstore.h ${DESKTOP_COMPANION_SRC_DIR}/data/chatstore.cpp
    ${DESKTOP_COMPANION_SRC_DIR}/data/configstore.h ${DESKTOP_COMPANION_SRC_DIR}/data/configstore.cpp
    ${DESKTOP_COMPANION_SRC_DIR}/data/intimacyrepository.h ${DESKTOP_COMPANION_SRC_DIR}/data/intimacyrepository.cpp
    ${DESKTOP_COMPANION_SRC_DIR}/data/memorystore.h ${DESKTOP_COMPANION_SRC_DIR}/data/memorystore.cpp
    ${DESKTOP_COMPANION_SRC_DIR}/data/personastore.h ${DESKTOP_COMPANION_SRC_DIR}/data/personastore.cpp
    ${DESKTOP_COMPANION_SRC_DIR}/data/worldstore.h ${DESKTOP_COMPANION_SRC_DIR}/data/worldstore.cpp
)

set(DESKTOP_COMPANION_SERVICES_SOURCES
    ${DESKTOP_COMPANION_SRC_DIR}/services/autostart.h ${DESKTOP_COMPANION_SRC_DIR}/services/autostart.cpp
    ${DESKTOP_COMPANION_SRC_DIR}/services/characterpack.h ${DESKTOP_COMPANION_SRC_DIR}/services/characterpack.cpp
    ${DESKTOP_COMPANION_SRC_DIR}/services/chatengine.h ${DESKTOP_COMPANION_SRC_DIR}/services/chatengine.cpp
    ${DESKTOP_COMPANION_SRC_DIR}/services/chatservice.h ${DESKTOP_COMPANION_SRC_DIR}/services/chatservice.cpp
    ${DESKTOP_COMPANION_SRC_DIR}/services/errorcodes.h ${DESKTOP_COMPANION_SRC_DIR}/services/errorcodes.cpp
    ${DESKTOP_COMPANION_SRC_DIR}/services/llmclient.h ${DESKTOP_COMPANION_SRC_DIR}/services/llmclient.cpp
    ${DESKTOP_COMPANION_SRC_DIR}/services/personafile.h ${DESKTOP_COMPANION_SRC_DIR}/services/personafile.cpp
    ${DESKTOP_COMPANION_SRC_DIR}/services/promptbuilder.h ${DESKTOP_COMPANION_SRC_DIR}/services/promptbuilder.cpp
    ${DESKTOP_COMPANION_SRC_DIR}/services/secretstore.h ${DESKTOP_COMPANION_SRC_DIR}/services/secretstore.cpp
    ${DESKTOP_COMPANION_SRC_DIR}/services/sseparser.h ${DESKTOP_COMPANION_SRC_DIR}/services/sseparser.cpp
)

set(DESKTOP_COMPANION_PLATFORM_SOURCES
    ${DESKTOP_COMPANION_SRC_DIR}/platform/foregroundwatcher.h ${DESKTOP_COMPANION_SRC_DIR}/platform/foregroundwatcher.cpp
    ${DESKTOP_COMPANION_SRC_DIR}/platform/globalhotkey.h ${DESKTOP_COMPANION_SRC_DIR}/platform/globalhotkey.cpp
    ${DESKTOP_COMPANION_SRC_DIR}/platform/hitareameta.h ${DESKTOP_COMPANION_SRC_DIR}/platform/hitareameta.cpp
    ${DESKTOP_COMPANION_SRC_DIR}/platform/hitmask.h ${DESKTOP_COMPANION_SRC_DIR}/platform/hitmask.cpp
    ${DESKTOP_COMPANION_SRC_DIR}/platform/petwindowhelper.h ${DESKTOP_COMPANION_SRC_DIR}/platform/petwindowhelper.cpp
)

# 核心模型层（domain/data/services/platform）；View 层见 DESKTOP_COMPANION_APP_SOURCES。
set(DESKTOP_COMPANION_CORE_SOURCES
    ${DESKTOP_COMPANION_DOMAIN_SOURCES}
    ${DESKTOP_COMPANION_DATA_SOURCES}
    ${DESKTOP_COMPANION_SERVICES_SOURCES}
    ${DESKTOP_COMPANION_PLATFORM_SOURCES}
)


# View 层（C++ 装配：窗口注册表、应用外壳）
set(DESKTOP_COMPANION_APP_SOURCES
    ${DESKTOP_COMPANION_SRC_DIR}/app/application.h ${DESKTOP_COMPANION_SRC_DIR}/app/application.cpp
    ${DESKTOP_COMPANION_SRC_DIR}/app/windowregistry.h ${DESKTOP_COMPANION_SRC_DIR}/app/windowregistry.cpp
)

# ViewModel 层（经典 MVVM：Q_PROPERTY + Q_INVOKABLE）
file(GLOB DESKTOP_COMPANION_VIEWMODEL_SOURCES CONFIGURE_DEPENDS
    ${DESKTOP_COMPANION_SRC_DIR}/viewmodels/*.h
    ${DESKTOP_COMPANION_SRC_DIR}/viewmodels/*.cpp
)

qt_add_executable(desktop_companion WIN32
    ${DESKTOP_COMPANION_SRC_DIR}/main.cpp
    ${DESKTOP_COMPANION_CORE_SOURCES}
    ${DESKTOP_COMPANION_APP_SOURCES}
    ${DESKTOP_COMPANION_VIEWMODEL_SOURCES}
)
target_include_directories(desktop_companion PRIVATE ${DESKTOP_COMPANION_INCLUDE_DIRS})

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
        qml/components/Avatar.qml
        qml/components/Card.qml
        qml/components/CharacterCard.qml
        qml/components/EmptyState.qml
        qml/components/FormRow.qml
        qml/components/IconButton.qml
        qml/components/IconData.js
        qml/components/Icon.qml
        qml/components/InputDialog.qml
        qml/components/PageHeader.qml
        qml/components/PageScroll.qml
        qml/components/PrimaryButton.qml
        qml/components/SecondaryButton.qml
        qml/components/SegmentedControl.qml
        qml/components/SelectBox.qml
        qml/components/SidebarItem.qml
        qml/components/SliderField.qml
        qml/components/StatusBar.qml
        qml/components/Tag.qml
        qml/components/TextFieldCard.qml
        qml/components/Toast.qml
        qml/components/ToggleSwitch.qml
        qml/MainWindow.qml
        qml/pages/AboutPage.qml
        qml/pages/CharactersPage.qml
        qml/pages/ChatPage.qml
        qml/pages/DataPage.qml
        qml/pages/MemoriesPage.qml
        qml/pages/ModelsPage.qml
        qml/pages/PersonasPage.qml
        qml/pages/RecordsPage.qml
        qml/pages/SettingsPage.qml
        qml/pages/WorldsPage.qml
        qml/pet/BallWindow.qml
        qml/pet/BubbleWindow.qml
        qml/pet/PetWindow.qml
        qml/theme/qmldir
        qml/theme/ThemeDark.qml
        qml/theme/ThemeLight.qml
        qml/theme/Theme.qml
)

qt_add_resources(desktop_companion "app_resources"
    PREFIX "/"
    BASE "resources"
    FILES resources/pet.png
        resources/icons/activity.svg
        resources/icons/alert-triangle.svg
        resources/icons/book-open.svg
        resources/icons/check.svg
        resources/icons/chevron-down.svg
        resources/icons/chevron-right.svg
        resources/icons/cpu.svg
        resources/icons/external-link.svg
        resources/icons/film.svg
        resources/icons/folder-open.svg
        resources/icons/github.svg
        resources/icons/image.svg
        resources/icons/images.svg
        resources/icons/info.svg
        resources/icons/keyboard.svg
        resources/icons/maximize.svg
        resources/icons/minimize.svg
        resources/icons/minus.svg
        resources/icons/monitor.svg
        resources/icons/moon.svg
        resources/icons/panel-left-close.svg
        resources/icons/panel-left-open.svg
        resources/icons/pause.svg
        resources/icons/play.svg
        resources/icons/plus.svg
        resources/icons/refresh-cw.svg
        resources/icons/rotate-ccw.svg
        resources/icons/settings.svg
        resources/icons/sparkles.svg
        resources/icons/square.svg
        resources/icons/sun.svg
        resources/icons/x.svg
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
    ${DESKTOP_COMPANION_SRC_DIR}/services/chatengine.h ${DESKTOP_COMPANION_SRC_DIR}/services/chatengine.cpp)
target_include_directories(test_chatengine PRIVATE ${DESKTOP_COMPANION_INCLUDE_DIRS})
target_link_libraries(test_chatengine PRIVATE Qt6::Test Qt6::Core)
add_test(NAME chatengine COMMAND test_chatengine)

qt_add_executable(test_sse tests/test_sse.cpp tests/test_sse.h
    ${DESKTOP_COMPANION_SRC_DIR}/services/sseparser.h ${DESKTOP_COMPANION_SRC_DIR}/services/sseparser.cpp)
target_include_directories(test_sse PRIVATE ${DESKTOP_COMPANION_INCLUDE_DIRS})
target_link_libraries(test_sse PRIVATE Qt6::Test Qt6::Core)
add_test(NAME sse COMMAND test_sse)

qt_add_executable(test_chatstore tests/test_chatstore.cpp tests/test_chatstore.h
    ${DESKTOP_COMPANION_SRC_DIR}/data/chatstore.h ${DESKTOP_COMPANION_SRC_DIR}/data/chatstore.cpp)
target_include_directories(test_chatstore PRIVATE ${DESKTOP_COMPANION_INCLUDE_DIRS})
target_link_libraries(test_chatstore PRIVATE Qt6::Test SQLite::SQLite3 stacklog::stacklog)
add_test(NAME chatstore COMMAND test_chatstore)

qt_add_executable(test_errorcodes tests/test_errorcodes.cpp tests/test_errorcodes.h
    ${DESKTOP_COMPANION_SRC_DIR}/services/errorcodes.h ${DESKTOP_COMPANION_SRC_DIR}/services/errorcodes.cpp)
target_include_directories(test_errorcodes PRIVATE ${DESKTOP_COMPANION_INCLUDE_DIRS})
target_link_libraries(test_errorcodes PRIVATE Qt6::Test Qt6::Core)
add_test(NAME errorcodes COMMAND test_errorcodes)

qt_add_executable(test_characterpack tests/test_characterpack.cpp tests/test_characterpack.h
    ${DESKTOP_COMPANION_SRC_DIR}/services/characterpack.h ${DESKTOP_COMPANION_SRC_DIR}/services/characterpack.cpp)
target_include_directories(test_characterpack PRIVATE ${DESKTOP_COMPANION_INCLUDE_DIRS})
target_link_libraries(test_characterpack PRIVATE Qt6::Test Qt6::Core)
add_test(NAME characterpack COMMAND test_characterpack)

qt_add_executable(test_personafile tests/test_personafile.cpp tests/test_personafile.h
    ${DESKTOP_COMPANION_SRC_DIR}/services/personafile.h ${DESKTOP_COMPANION_SRC_DIR}/services/personafile.cpp)
target_include_directories(test_personafile PRIVATE ${DESKTOP_COMPANION_INCLUDE_DIRS})
target_link_libraries(test_personafile PRIVATE Qt6::Test Qt6::Core)
add_test(NAME personafile COMMAND test_personafile)

qt_add_executable(test_characterstore tests/test_characterstore.cpp tests/test_characterstore.h
    ${DESKTOP_COMPANION_SRC_DIR}/data/characterstore.h ${DESKTOP_COMPANION_SRC_DIR}/data/characterstore.cpp
    ${DESKTOP_COMPANION_SRC_DIR}/services/personafile.h ${DESKTOP_COMPANION_SRC_DIR}/services/personafile.cpp)
target_include_directories(test_characterstore PRIVATE ${DESKTOP_COMPANION_INCLUDE_DIRS})
target_link_libraries(test_characterstore PRIVATE Qt6::Test Qt6::Core)
add_test(NAME characterstore COMMAND test_characterstore)

qt_add_executable(test_worldstore tests/test_worldstore.cpp tests/test_worldstore.h
    ${DESKTOP_COMPANION_SRC_DIR}/data/worldstore.h ${DESKTOP_COMPANION_SRC_DIR}/data/worldstore.cpp
    ${DESKTOP_COMPANION_SRC_DIR}/data/characterstore.h ${DESKTOP_COMPANION_SRC_DIR}/data/characterstore.cpp
    ${DESKTOP_COMPANION_SRC_DIR}/services/personafile.h ${DESKTOP_COMPANION_SRC_DIR}/services/personafile.cpp)
target_include_directories(test_worldstore PRIVATE ${DESKTOP_COMPANION_INCLUDE_DIRS})
target_link_libraries(test_worldstore PRIVATE Qt6::Test Qt6::Core)
add_test(NAME worldstore COMMAND test_worldstore)

qt_add_executable(test_hitmask tests/test_hitmask.cpp tests/test_hitmask.h
    ${DESKTOP_COMPANION_SRC_DIR}/platform/hitmask.h ${DESKTOP_COMPANION_SRC_DIR}/platform/hitmask.cpp)
target_include_directories(test_hitmask PRIVATE ${DESKTOP_COMPANION_INCLUDE_DIRS})
target_link_libraries(test_hitmask PRIVATE Qt6::Test Qt6::Gui)
add_test(NAME hitmask COMMAND test_hitmask)

qt_add_executable(test_hitareameta tests/test_hitareameta.cpp tests/test_hitareameta.h
    ${DESKTOP_COMPANION_SRC_DIR}/platform/hitareameta.h ${DESKTOP_COMPANION_SRC_DIR}/platform/hitareameta.cpp)
target_include_directories(test_hitareameta PRIVATE ${DESKTOP_COMPANION_INCLUDE_DIRS})
target_link_libraries(test_hitareameta PRIVATE Qt6::Test Qt6::Core)
add_test(NAME hitareameta COMMAND test_hitareameta)

qt_add_executable(test_memorystore tests/test_memorystore.cpp tests/test_memorystore.h
    ${DESKTOP_COMPANION_SRC_DIR}/data/memorystore.h ${DESKTOP_COMPANION_SRC_DIR}/data/memorystore.cpp)
target_include_directories(test_memorystore PRIVATE ${DESKTOP_COMPANION_INCLUDE_DIRS})
target_link_libraries(test_memorystore PRIVATE Qt6::Test Qt6::Core SQLite::SQLite3)
add_test(NAME memorystore COMMAND test_memorystore)

qt_add_executable(test_promptbuilder tests/test_promptbuilder.cpp tests/test_promptbuilder.h
    ${DESKTOP_COMPANION_SRC_DIR}/services/promptbuilder.h ${DESKTOP_COMPANION_SRC_DIR}/services/promptbuilder.cpp)
target_include_directories(test_promptbuilder PRIVATE ${DESKTOP_COMPANION_INCLUDE_DIRS})
target_link_libraries(test_promptbuilder PRIVATE Qt6::Test Qt6::Core)
add_test(NAME promptbuilder COMMAND test_promptbuilder)

qt_add_executable(test_intimacy tests/test_intimacy.cpp tests/test_intimacy.h
    ${DESKTOP_COMPANION_SRC_DIR}/data/intimacyrepository.h ${DESKTOP_COMPANION_SRC_DIR}/data/intimacyrepository.cpp)
target_include_directories(test_intimacy PRIVATE ${DESKTOP_COMPANION_INCLUDE_DIRS})
target_link_libraries(test_intimacy PRIVATE Qt6::Test Qt6::Core)
add_test(NAME intimacy COMMAND test_intimacy)

qt_add_executable(test_config tests/test_config.cpp tests/test_config.h
    ${DESKTOP_COMPANION_SRC_DIR}/services/autostart.h ${DESKTOP_COMPANION_SRC_DIR}/services/autostart.cpp
    ${DESKTOP_COMPANION_SRC_DIR}/data/configstore.h ${DESKTOP_COMPANION_SRC_DIR}/data/configstore.cpp
    ${DESKTOP_COMPANION_SRC_DIR}/data/personastore.h ${DESKTOP_COMPANION_SRC_DIR}/data/personastore.cpp
    ${DESKTOP_COMPANION_SRC_DIR}/services/secretstore.h ${DESKTOP_COMPANION_SRC_DIR}/services/secretstore.cpp)
target_include_directories(test_config PRIVATE ${DESKTOP_COMPANION_INCLUDE_DIRS})
target_link_libraries(test_config PRIVATE Qt6::Test Qt6::Core)
if(WIN32)
    target_link_libraries(test_config PRIVATE Crypt32)
endif()
add_test(NAME config COMMAND test_config)

# ---- ViewModel 单元测试（MVVM 层） ----
qt_add_executable(test_viewmodels
    tests/test_viewmodels.cpp tests/test_viewmodels.h
    ${DESKTOP_COMPANION_VIEWMODEL_SOURCES}
    ${DESKTOP_COMPANION_DATA_SOURCES}
    ${DESKTOP_COMPANION_SERVICES_SOURCES}
    ${DESKTOP_COMPANION_SRC_DIR}/platform/hitareameta.h
    ${DESKTOP_COMPANION_SRC_DIR}/platform/hitareameta.cpp
)
target_include_directories(test_viewmodels PRIVATE ${DESKTOP_COMPANION_INCLUDE_DIRS})
target_compile_definitions(test_viewmodels PRIVATE
    DESKTOP_COMPANION_VERSION="${PROJECT_VERSION}")
target_link_libraries(test_viewmodels PRIVATE
    Qt6::Test Qt6::Core Qt6::Network Qt6::Widgets
    SQLite::SQLite3 stacklog::stacklog)
if(WIN32)
    target_link_libraries(test_viewmodels PRIVATE Crypt32)
endif()
add_test(NAME viewmodels COMMAND test_viewmodels)

# 架构分层静态校验（可选：需要 Python3）。
find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_Interpreter_FOUND)
    add_test(NAME architecture_check
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/architecture_check.py)
endif()
