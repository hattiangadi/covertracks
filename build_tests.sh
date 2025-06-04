#!/bin/bash

# Simple script to build and run pattern parser tests
# This assumes JUCE is available and basic includes are set up

echo "Building PatternParser tests..."

# Create a simple build directory for tests
mkdir -p test_build
cd test_build

# Note: This is a simplified build - you may need to adjust paths for your JUCE setup
g++ -std=c++17 -I../Source -I../modules/juce/modules -I../modules/tracktion_engine/modules \
    -DJUCE_STANDALONE_APPLICATION=1 \
    ../Source/Tests/TestRunner.cpp \
    ../Source/Musician/Core/PatternParser.cpp \
    ../Source/Musician/Core/MusicPatternSystem.cpp \
    ../Source/Musician/Core/MusicPatternComponents.cpp \
    -o pattern_tests \
    2>&1

if [ $? -eq 0 ]; then
    echo "Build successful! Running tests..."
    ./pattern_tests
else
    echo "Build failed. You may need to adjust include paths or use Xcode to build the tests."
    echo "Try adding the tests to your Xcode project instead."
fi