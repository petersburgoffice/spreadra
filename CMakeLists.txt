cmake_minimum_required(VERSION 3.15)
project(Reverb VERSION 0.1.0)

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

# Автоматическое увеличение версии при каждой сборке
execute_process(
    COMMAND ${CMAKE_SOURCE_DIR}/increment_version.sh
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE VERSION_OUTPUT
    RESULT_VARIABLE VERSION_RESULT
)

if(VERSION_RESULT EQUAL 0)
    message(STATUS "Version auto-incremented: ${VERSION_OUTPUT}")
endif()

# Читаем версию из файла
file(STRINGS ${CMAKE_SOURCE_DIR}/version.txt VERSION_STRING)
string(REPLACE "." ";" VERSION_LIST ${VERSION_STRING})
list(GET VERSION_LIST 0 VERSION_MAJOR)
list(GET VERSION_LIST 1 VERSION_MINOR)
list(GET VERSION_LIST 2 VERSION_BUILD)

message(STATUS "Version configured: ${VERSION_STRING}")

# Собираем список всех исходников из src/ (без utils - используем Common)
file(GLOB_RECURSE SHIMMER_SOURCES
    src/core/*.cpp src/core/*.h
    src/dsp/*.cpp src/dsp/*.h
    src/gui/*.cpp src/gui/*.h
)

juce_add_plugin(Reverb
    COMPANY_NAME "SonicMakers"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    PLUGIN_MANUFACTURER_CODE Snmk
    PLUGIN_CODE Revb
    FORMATS AU Standalone
    PRODUCT_NAME "Reverb"
)

target_sources(Reverb PRIVATE ${SHIMMER_SOURCES})

target_link_libraries(Reverb PRIVATE
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
    add_custom_command(TARGET Reverb_AU POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${AU_INSTALL_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory 
            "$<TARGET_BUNDLE_DIR:Reverb_AU>" 
            "${AU_INSTALL_DIR}/Reverb.component"
        COMMENT "Installing AU plugin to ${AU_INSTALL_DIR}"
    )
    
    message(STATUS "AU plugin will be automatically installed to: ${AU_INSTALL_DIR}")
endif() 