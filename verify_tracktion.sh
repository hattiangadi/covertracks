#!/bin/bash

echo "Verifying Tracktion Engine integration..."
echo "========================================="

cd /Users/anand/src/covertracks/Builds/MacOSX

# Start the app in background and capture output
timeout 30s ./build/Debug/Covertracks.app/Contents/MacOS/Covertracks > tracktion_test_output.log 2>&1 &
app_pid=$!

# Wait a bit for the app to start
sleep 2

# Check if the app started successfully
if ps -p $app_pid > /dev/null; then
    echo "✅ Application started successfully"
else
    echo "❌ Application failed to start"
    exit 1
fi

# Wait a bit more for initialization
sleep 3

# Kill the app
kill $app_pid 2>/dev/null

# Check the log for Tracktion-related output
echo ""
echo "Checking for Tracktion Engine evidence in output..."
echo "=================================================="

if grep -i "tracktion" tracktion_test_output.log > /dev/null; then
    echo "✅ Found Tracktion-related output"
    echo ""
    echo "Tracktion-related lines:"
    grep -i "tracktion" tracktion_test_output.log | head -10
else
    echo "❌ No Tracktion-related output found"
fi

echo ""
echo "Checking for test execution patterns..."
echo "======================================"

if grep -i "test" tracktion_test_output.log > /dev/null; then
    echo "✅ Found test-related output"
    test_lines=$(grep -i "test" tracktion_test_output.log | wc -l)
    echo "Number of test-related lines: $test_lines"
else
    echo "❌ No test-related output found"
fi

echo ""
echo "Checking for successful initialization..."
echo "======================================="

if grep -i "initialized\|created\|success" tracktion_test_output.log > /dev/null; then
    echo "✅ Found initialization success indicators"
    echo ""
    echo "Success indicators:"
    grep -i "initialized\|created\|success" tracktion_test_output.log | head -5
else
    echo "❌ No success indicators found"
fi

echo ""
echo "First 20 lines of output:"
echo "========================"
head -20 tracktion_test_output.log

echo ""
echo "Last 10 lines of output:"
echo "======================="
tail -10 tracktion_test_output.log

echo ""
echo "Verification complete. See tracktion_test_output.log for full output."