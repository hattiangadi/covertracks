#!/bin/bash

# Test script to run just the Tracktion tests
cd /Users/anand/src/covertracks/Builds/MacOSX

# Build first
echo "Building project..."
xcodebuild -project Covertracks.xcodeproj -scheme "Covertracks - App" -configuration Debug build

# Run tests with timeout and capture output
echo "Running tests..."
timeout 10s "./build/Debug/Covertracks.app/Contents/MacOS/Covertracks" --run-tests 2>&1 | grep -E "(Tracktion|PASS|FAIL|TEST|Error)" | head -50