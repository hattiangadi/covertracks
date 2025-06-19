#!/bin/bash

# Test script for AudioEngine feature completion validation
# Tests the new MIDI callback and player graph integration features

echo "=== AudioEngine Feature Completion Test ==="
echo ""

echo "Building project..."
cd Builds/MacOSX
if xcodebuild -project Covertracks.xcodeproj -configuration Debug build > /dev/null 2>&1; then
    echo "✅ Build successful"
else
    echo "❌ Build failed"
    exit 1
fi

echo ""
echo "Launching app to run AudioEngine tests..."
echo "Look for:"
echo "- MIDI Routing test results"
echo "- Player Graph Integration test results"
echo "- Performance Mode test results"
echo ""

open "build/Debug/Covertracks - App.app"

echo "Test app launched. Check the test results in the application."
echo ""
echo "Expected new functionality:"
echo "✅ Player MIDI callback system"
echo "✅ AudioEngine player graph integration"
echo "✅ Enhanced MIDI routing to players"
echo "✅ Thread-safe player coordination"
echo ""