#!/bin/bash

echo "Running Tracktion Integration Tests..."
cd /Users/anand/src/covertracks/Builds/MacOSX

# Run and capture just the Tracktion test output with a timeout
timeout 15s "./build/Debug/Covertracks.app/Contents/MacOS/Covertracks" --run-tests 2>&1 | grep -A 20 -B 5 "Tracktion"