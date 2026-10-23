cmake_minimum_required(VERSION 3.15)
project(Covertracks VERSION 1.0.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable compile commands for clang-tidy
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Add JUCE
add_subdirectory(modules/tracktion_engine/modules/juce)

# ==============================================================================
# Main Application
# ==============================================================================

juce_add_gui_app(Covertracks
    BUNDLE_ID                       com.juce.pluginhost
    ICON_BIG                        "${CMAKE_CURRENT_SOURCE_DIR}/JUCEAppIcon.png"
    MICROPHONE_PERMISSION_ENABLED   TRUE
    PLUGINHOST_AU                   TRUE)

juce_generate_juce_header(Covertracks)

# All application source files
target_sources(Covertracks PRIVATE
    # Core startup
    Source/HostStartup.cpp
    
    # Audio system
    Source/Audio/AudioEngine.cpp
    
    # LLM system
    Source/LLM/Core/ClaudeClient.cpp
    Source/LLM/Core/LLMClient.cpp
    Source/LLM/Core/LLMManager.cpp
    Source/LLM/Core/PromptManager.cpp
    Source/LLM/UI/LLMConversationComponent.cpp
    
    # Musician system - Core
    Source/Musician/Core/EnsembleManager.cpp
    Source/Musician/Core/Musician.cpp
    Source/Musician/Core/MusicPatternComponents.cpp
    Source/Musician/Core/MusicPatternSystem.cpp
    Source/Musician/Core/PatternGenerator.cpp
    Source/Musician/Core/PatternParser.cpp
    Source/Musician/Core/Performance.cpp
    Source/Musician/Core/PerformanceManager.cpp
    Source/Musician/Core/Song.cpp
    
    # Musician system - UI
    Source/Musician/UI/EnsembleEditPanel.cpp
    Source/Musician/UI/EnsembleListPanel.cpp
    Source/Musician/UI/EnsembleMainPanel.cpp
    Source/Musician/UI/MusicianEditorComponent.cpp
    Source/Musician/UI/PatternEditorComponent.cpp
    Source/Musician/UI/PerformanceComponent.cpp
    Source/Musician/UI/Performance/PerformanceEditorComponent.cpp
    Source/Musician/UI/Performance/PerformanceLibraryComponent.cpp
    Source/Musician/UI/Performance/PerformanceListComponent.cpp
    Source/Musician/UI/Song/SectionEditorDialog.cpp
    Source/Musician/UI/Song/SongEditorComponent.cpp
    Source/Musician/UI/Song/SongImageImportComponent.cpp
    Source/Musician/UI/Song/SongListComponent.cpp
    Source/Musician/UI/Song/SongManagerComponent.cpp
    
    # Player system - Core
    Source/Player/Core/Instrument.cpp
    Source/Player/Core/Player.cpp
    Source/Player/Core/PlayerManager.cpp

    # Practice system - Core
    Source/Practice/Core/LessonLoader.cpp
    Source/Practice/Core/AccompanimentPlayer.cpp

    # Practice system - UI
    Source/Practice/UI/PracticePianoRoll.cpp
    Source/Practice/UI/PracticeModeComponent.cpp

    # Player system - UI
    Source/Player/UI/PlayerEditorComponent.cpp
    Source/Player/UI/PlayerLibraryComponent.cpp
    Source/Player/UI/PlayerListComponent.cpp
    Source/Player/UI/PlayerModeComponent.cpp
    
    # Plugin system
    Source/Plugins/ARAPlugin.cpp
    Source/Plugins/InternalPlugins.cpp
    Source/Plugins/IOConfigurationWindow.cpp
    Source/Plugins/PluginGraph.cpp
    
    # UI system
    Source/UI/GraphEditorPanel.cpp
    Source/UI/MainHostWindow.cpp
    Source/UI/TestsComponent.cpp
    
    # Tests (included in main app for now)
    Source/Tests/MusicianAdvancedTests.cpp
    Source/Tests/MusicianTests.cpp
    Source/Tests/PatternGeneratorTests.cpp
    Source/Tests/PatternParserTests.cpp
    Source/Tests/PlayerTests.cpp
    Source/Tests/PracticeLessonTests.cpp
    Source/Tests/AccompanimentPlayerTests.cpp
    Source/Tests/PracticeUITests.cpp
    Source/Tests/TestRunner.cpp
)

# Binary data
juce_add_binary_data(CovertracksData SOURCES
    BinaryData/cassette_recorder.wav
    BinaryData/cello.wav
    BinaryData/guitar_amp.wav
    BinaryData/proaudio.path
    BinaryData/reverb_ir.wav
    BinaryData/singing.ogg)

# Compile definitions
target_compile_definitions(Covertracks PRIVATE
    JUCE_ALSA=1
    JUCE_CONTENT_SHARING=1
    JUCE_DIRECTSOUND=1
    JUCE_DISABLE_CAUTIOUS_PARAMETER_ID_CHECKING=1
    JUCE_PLUGINHOST_LADSPA=1
    JUCE_PLUGINHOST_LV2=1
    JUCE_PLUGINHOST_VST3=1
    JUCE_PLUGINHOST_VST=0
    JUCE_PLUGINHOST_ARA=0
    JUCE_USE_CAMERA=0
    JUCE_USE_CDBURNER=0
    JUCE_USE_CDREADER=0
    JUCE_USE_CURL=0
    JUCE_USE_FLAC=0
    JUCE_USE_OGGVORBIS=1
    JUCE_VST3_HOST_CROSS_PLATFORM_UID=1
    JUCE_WASAPI=1
    JUCE_WEB_BROWSER=0
    PIP_JUCE_EXAMPLES_DIRECTORY_STRING="${JUCE_SOURCE_DIR}/examples"
    JUCE_SILENCE_XCODE_15_LINKER_WARNING=1)

# Link libraries
target_link_libraries(Covertracks PRIVATE
    CovertracksData
    juce::juce_audio_utils
    juce::juce_cryptography
    juce::juce_dsp
    juce::juce_opengl
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

juce_add_bundle_resources_directory(Covertracks BinaryData)

# ==============================================================================
# Testing Infrastructure
# ==============================================================================

# Enable testing
enable_testing()

# Option to build tests separately (useful for CI)
option(BUILD_TESTS "Build test executables" ON)

if(BUILD_TESTS)
    # Create test executable
    juce_add_console_app(CovertracksTests
        PRODUCT_NAME "Covertracks Tests")
    
    target_sources(CovertracksTests PRIVATE
        # Core systems needed for tests
        Source/Audio/AudioEngine.cpp
        Source/LLM/Core/ClaudeClient.cpp
        Source/LLM/Core/LLMClient.cpp
        Source/LLM/Core/LLMManager.cpp
        Source/LLM/Core/PromptManager.cpp
        Source/Musician/Core/EnsembleManager.cpp
        Source/Musician/Core/Musician.cpp
        Source/Musician/Core/MusicPatternComponents.cpp
        Source/Musician/Core/MusicPatternSystem.cpp
        Source/Musician/Core/PatternGenerator.cpp
        Source/Musician/Core/PatternParser.cpp
        Source/Musician/Core/Performance.cpp
        Source/Musician/Core/PerformanceManager.cpp
        Source/Musician/Core/Song.cpp
        Source/Player/Core/Instrument.cpp
        Source/Player/Core/Player.cpp
        Source/Player/Core/PlayerManager.cpp
        Source/Plugins/InternalPlugins.cpp
        Source/Plugins/PluginGraph.cpp
        Source/Practice/Core/LessonLoader.cpp
        Source/Practice/Core/AccompanimentPlayer.cpp
        Source/Practice/UI/PracticePianoRoll.cpp
        Source/Practice/UI/PracticeModeComponent.cpp

        # Test files
        Source/Tests/MusicianAdvancedTests.cpp
        Source/Tests/MusicianTests.cpp
        Source/Tests/PatternGeneratorTests.cpp
        Source/Tests/PatternParserTests.cpp
        Source/Tests/PlayerTests.cpp
        Source/Tests/PracticeLessonTests.cpp
        Source/Tests/AccompanimentPlayerTests.cpp
        Source/Tests/PracticeUITests.cpp
        Source/Tests/TestRunner.cpp
        
        # Test main
        Source/Tests/TestMain.cpp  # We'll create this
    )
    
    target_compile_definitions(CovertracksTests PRIVATE
        JUCE_DISABLE_CAUTIOUS_PARAMETER_ID_CHECKING=1
        JUCE_USE_CURL=0
        JUCE_USE_CAMERA=0
        JUCE_WEB_BROWSER=0
        JUCE_USE_FLAC=0
        JUCE_USE_OGGVORBIS=1
    )
    
    target_link_libraries(CovertracksTests PRIVATE
        juce::juce_audio_utils
        juce::juce_cryptography
        juce::juce_dsp
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
    
    # Register test
    add_test(NAME CovertracksTests COMMAND CovertracksTests)
endif()

# ==============================================================================
# Development Tools
# ==============================================================================

# Clang-tidy configuration
if(CMAKE_EXPORT_COMPILE_COMMANDS)
    message(STATUS "Compile commands will be exported to ${CMAKE_BINARY_DIR}/compile_commands.json")
    message(STATUS "Run clang-tidy with: clang-tidy <source-file> -p ${CMAKE_BINARY_DIR}")
endif()

# Custom targets for development
add_custom_target(format
    COMMAND find ${CMAKE_SOURCE_DIR}/Source -name "*.cpp" -o -name "*.h" | xargs clang-format -i
    COMMENT "Formatting source code")

add_custom_target(tidy
    COMMAND find ${CMAKE_SOURCE_DIR}/Source -name "*.cpp" | xargs clang-tidy -p ${CMAKE_BINARY_DIR}
    COMMENT "Running clang-tidy static analysis")

# ==============================================================================
# Installation (optional)
# ==============================================================================

install(TARGETS Covertracks
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin)

# ==============================================================================
# Summary
# ==============================================================================

message(STATUS "")
message(STATUS "Covertracks CMake Configuration:")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Tests: ${BUILD_TESTS}")
message(STATUS "  Compile commands: ${CMAKE_EXPORT_COMPILE_COMMANDS}")
message(STATUS "")
message(STATUS "Available targets:")
message(STATUS "  Covertracks - Main application")
if(BUILD_TESTS)
message(STATUS "  CovertracksTests - Test executable")
endif()
message(STATUS "  format - Format source code")
message(STATUS "  tidy - Run clang-tidy analysis")
message(STATUS "")