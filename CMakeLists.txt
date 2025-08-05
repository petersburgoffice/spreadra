cmake_minimum_required(VERSION 3.15)
project(Spreadra VERSION 0.9.1)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add JUCE submodule
add_subdirectory(../JUCE ${CMAKE_BINARY_DIR}/JUCE)

# Add Common library
add_subdirectory(../Common ${CMAKE_BINARY_DIR}/Common)

# FFTW3 support
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFTW3 REQUIRED IMPORTED_TARGET fftw3)

# Читаем версию из файла (версия управляется через Makefile)
file(STRINGS ${CMAKE_SOURCE_DIR}/version.txt VERSION_STRING)
string(REPLACE "." ";" VERSION_LIST ${VERSION_STRING})
list(GET VERSION_LIST 0 VERSION_MAJOR)
list(GET VERSION_LIST 1 VERSION_MINOR)
list(GET VERSION_LIST 2 VERSION_BUILD)

message(STATUS "Version configured: ${VERSION_STRING}")

# Генерируем Version.h из шаблона
configure_file(
    ${CMAKE_SOURCE_DIR}/src/core/Version.h.in
    ${CMAKE_SOURCE_DIR}/src/core/Version.h
    @ONLY
)

# Собираем список всех исходников из src/ (без utils - используем Common)
file(GLOB_RECURSE SHIMMER_SOURCES
    src/core/*.cpp src/core/*.h
    src/dsp/*.cpp src/dsp/*.h
    src/gui/*.cpp src/gui/*.h
)

juce_add_plugin(Spreadra
    COMPANY_NAME "SonicMakers"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    PLUGIN_MANUFACTURER_CODE Snmk
    PLUGIN_CODE Spdr
    FORMATS AU
    PRODUCT_NAME "Spreadra"
)

# Force modern parameter indexing
target_compile_definitions(Spreadra PRIVATE
    JUCE_FORCE_USE_LEGACY_PARAM_IDS=0
)

target_compile_definitions(Spreadra_AU PRIVATE  
    JUCE_FORCE_USE_LEGACY_PARAM_IDS=0
)

target_sources(Spreadra PRIVATE ${SHIMMER_SOURCES})

# Add binary resources
juce_add_binary_data(SpreadraData SOURCES
    Resources/UI.png
)

target_link_libraries(Spreadra PRIVATE
    SpreadraData
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_audio_plugin_client
    juce::juce_audio_formats
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
    PkgConfig::FFTW3
    Common
    # juce::juce_dsp # если используете
)

# Автоматический деплой AU версии после сборки
if(APPLE)
    # Путь к папке Audio Components
    set(AU_INSTALL_DIR "$ENV{HOME}/Library/Audio/Plug-Ins/Components")
    
    # Добавляем команду копирования AU после сборки
    add_custom_command(TARGET Spreadra_AU POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${AU_INSTALL_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory 
            "$<TARGET_BUNDLE_DIR:Spreadra_AU>" 
            "${AU_INSTALL_DIR}/Spreadra.component"
        COMMENT "Installing AU plugin to ${AU_INSTALL_DIR}"
    )
    
    message(STATUS "AU plugin will be automatically installed to: ${AU_INSTALL_DIR}")
endif() 