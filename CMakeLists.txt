cmake_minimum_required(VERSION 3.21)
project(GenryTB VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 Multimedia Concurrent ShaderTools)
qt_standard_project_setup(REQUIRES 6.8)

if(MSVC)
    add_compile_options(/utf-8 /W3 /permissive- /Zc:__cplusplus)
    add_compile_options($<$<CONFIG:Release>:/Zi>)
    add_link_options($<$<CONFIG:Release>:/DEBUG> $<$<CONFIG:Release>:/OPT:REF> $<$<CONFIG:Release>:/OPT:ICF>)
endif()

# ---- core: the game's archive and catalog, the story language, preview, compiler, builder (no UI) ----
add_library(tb_core STATIC
    src/core/Pickle.cpp
    src/core/Rpa.cpp
    src/core/Catalog.cpp
    src/core/Story.cpp
    src/core/Motion.cpp
    src/core/Renderer.cpp
    src/core/Compiler.cpp
    src/core/Builder.cpp
)
target_include_directories(tb_core PUBLIC src/core)
target_link_libraries(tb_core PUBLIC Qt6::Core Qt6::Gui Qt6::Concurrent)
target_compile_definitions(tb_core PRIVATE TB_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

qt_add_executable(tb_cli tools/tb_cli.cpp)
set_target_properties(tb_cli PROPERTIES WIN32_EXECUTABLE OFF)
target_link_libraries(tb_cli PRIVATE tb_core)

qt_add_executable(tb_selftest tools/selftest.cpp)
set_target_properties(tb_selftest PROPERTIES WIN32_EXECUTABLE OFF)
target_link_libraries(tb_selftest PRIVATE tb_core)
target_compile_definitions(tb_selftest PRIVATE TB_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

# ---- the app: C++ engine + QML UI ----
# TB_DEV: Genry's own developer build (BUILD_V1.cmd) - its things not in the public release (18+ mods)
option(TB_DEV "the developer's build" OFF)
qt_add_executable(GenryTB WIN32 src/app/main.cpp src/app.rc)
if(TB_DEV)
    target_compile_definitions(GenryTB PRIVATE TB_DEV=1)
endif()
set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
qt_add_qml_module(GenryTB
    URI GenryTB
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/Theme.qml
        qml/MenuButton.qml
        qml/MenuField.qml
        qml/Polaroid.qml
        qml/Launcher.qml
        qml/Editor.qml
        qml/Chip.qml
        qml/TButton.qml
        qml/TField.qml
        qml/ScenePicker.qml
        qml/BlockCard.qml
        qml/BlocksView.qml
        qml/BackgroundPicker.qml
        qml/CharacterPicker.qml
        qml/AudioPicker.qml
        qml/SpeakerPicker.qml
        qml/Inspector.qml
        qml/ScenesPanel.qml
        qml/PreviewPane.qml
        qml/CodeView.qml
    SOURCES
        src/app/Engine.h src/app/Engine.cpp
        src/app/Blocks.h src/app/Blocks.cpp
        src/app/Images.h src/app/Images.cpp
        src/app/I18n.h src/app/I18n.cpp
        src/app/Highlighter.h src/app/Highlighter.cpp
    RESOURCE_PREFIX /qt/qml
)
# the preview's tape looks (the same maths as the hub's Ren'Py shaders)
qt_add_shaders(GenryTB "tb_shaders"
    PREFIX "/shaders"
    FILES shaders/vhs.frag shaders/noise.frag
)
target_include_directories(GenryTB PRIVATE src/app)
target_link_libraries(GenryTB PRIVATE tb_core Qt6::Quick Qt6::QuickControls2 Qt6::Qml Qt6::Multimedia)

# ---- GenryTB_Setup.exe: the one-file installer (plain Win32, the zipped program appended by tools/make_release.py)
if(WIN32)
    add_executable(GenryTB_Setup WIN32 tools/tb_setup.cpp src/app.rc)
    target_compile_options(GenryTB_Setup PRIVATE /utf-8)
    target_compile_definitions(GenryTB_Setup PRIVATE UNICODE _UNICODE)
    target_link_libraries(GenryTB_Setup PRIVATE ole32 shell32 uuid advapi32)
    # the static C runtime: the one file people download starts on a clean Windows (no VC++ Redistributable)
    set_property(TARGET GenryTB_Setup PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")
endif()
