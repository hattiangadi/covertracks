#\!/bin/bash

# Script to run tests and show only failures and summary
# Usage: ./run_tests_clean.sh [category]

cd "$(dirname "$0")"

CATEGORY=${1:-""}
TEST_CMD="./Builds/MacOSX/build/Debug/Covertracks.app/Contents/MacOS/Covertracks --run-tests"

if [ -n "$CATEGORY" ]; then
    TEST_CMD="$TEST_CMD --category=\"$CATEGORY\""
fi

echo "Running tests$([ -n "$CATEGORY" ] && echo " in category: $CATEGORY")..."
echo "Filtering output to show only failures and summary..."
echo ""

# Run tests and filter output
eval $TEST_CMD 2>&1 | awk '
BEGIN { 
    in_failure = 0
    failure_count = 0
}

# Capture test failure lines
/^\!\!\! Test [0-9]+ failed:/ {
    in_failure = 1
    failure_count++
    print "❌ FAILURE #" failure_count ": " $0
    next
}

# Capture JUCE assertion failures (they follow the test failure)
/^JUCE Assertion failure/ && in_failure {
    print "   " $0
    next
}

# Capture "FAILED\!\!" summary lines
/^FAILED\!\!/ {
    print "💥 " $0
    in_failure = 0
    next
}

# Capture final test results
/^=== Test Results ===/ {
    print $0
    getline; print $0  # Tests run: X
    getline; print $0  # Failures: X
    getline; print $0  # ❌ or ✅ message
    exit
}

# Reset failure state on test boundaries
/^-+$/ {
    in_failure = 0
}

/^Starting tests in:/ {
    current_test = $0
    in_failure = 0
}

# Suppress all other output
{ }
'

echo ""
echo "✨ Done\! Only failures and summary shown above."
EOF < /dev/null