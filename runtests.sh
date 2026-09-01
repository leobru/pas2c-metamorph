#!/bin/bash
# Test runner for p2c compiler test suite
# Runs test programs using runtest.sh and compares outputs

set -e

# Colors for output
RED='\033[1;31m'
GREEN='\033[1;32m'
YELLOW='\033[1;33m'
BLUE='\033[1;34m'
NC='\033[0m' # No Color

TESTS_DIR="tests"
RESULTS_DIR="test_results"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Default (and -work) run tests through the emulator-hosted work compiler
# module work.bin, which is host-built from work.p2c by base.cc.  The
# emulator base-module mode is retired along with base.pas.
RUNNER="runworktest.sh"
case "$1" in
    -work)
        shift
        ;;
esac

# Create results directory
mkdir -p "$RESULTS_DIR"

# Statistics
TOTAL=0
PASSED=0
FAILED=0
SKIPPED=0

# Function to run a single test
run_test() {
    local test_file="$1"
    local test_name=$(basename "$test_file" .p2c)
    local result_file="$RESULTS_DIR/${test_name}.result"
    local expected_file="${test_file%.p2c}.expected"
    
    TOTAL=$((TOTAL + 1))
    
    echo -ne "${BLUE}Running test: ${test_name}${NC} ... "
    
    # Check if test file exists
    if [ ! -f "$test_file" ]; then
        echo -e "${RED}SKIP${NC} (file not found)"
        SKIPPED=$((SKIPPED + 1))
        return
    fi
    
    # Run the test with timeout
    local rc=0
    timeout 10 ./$RUNNER "$test_file" > "$result_file" 2>&1 || rc=$?

    # A timeout is never an "expected failure".  .should_fail asserts that the
    # compiler *rejects* the program, not that it may spin: a hang there used
    # to be reported as a pass, which is how the switch-without-case spin
    # (tests/97) stayed invisible.  124 is timeout(1)'s SIGTERM kill, 137 a
    # SIGKILL that outlived it.  timeout signals the whole process group, so
    # dubna goes down with the runner.
    if [ $rc -eq 124 ] || [ $rc -eq 137 ]; then
        echo -e "${RED}FAIL${NC} (timeout -- infinite loop?)"
        FAILED=$((FAILED + 1))
        return
    fi

    # The work-compiler job may continue to *EXECUTE after reporting source
    # errors.  A .diagnostics sidecar makes a negative test strict: each line
    # is a required fragment in an actual compiler error.
    local diagnostic_file="${test_file%.p2c}.diagnostics"
    if [ -f "$diagnostic_file" ]; then
        if ! grep -Eiq 'Error [0-9]+:|\*\*\*\*\*[0-9]+' "$result_file"; then
            echo -e "${RED}FAIL${NC} (expected compiler error)"
            FAILED=$((FAILED + 1))
            return
        fi
        local expected_diag
        while IFS= read -r expected_diag || [ -n "$expected_diag" ]; do
            [ -z "$expected_diag" ] && continue
            if ! grep -Fqi -- "$expected_diag" "$result_file"; then
                echo -e "${RED}FAIL${NC} (missing diagnostic: $expected_diag)"
                FAILED=$((FAILED + 1))
                return
            fi
        done < "$diagnostic_file"
        echo -e "${GREEN}PASS${NC} (expected failure)"
        PASSED=$((PASSED + 1))
        return
    fi

    if [ $rc -eq 0 ]; then
        # A .ntr sidecar pins only the normalization-mode transitions.  The
        # work runner compiles and executes in one DUBNA job without retaining
        # the object, so the checker performs an extraction-only compilation.
        if [ -f "${test_file%.p2c}.ntr" ]; then
            if ! ./check-ntr.sh work "$test_file"; then
                echo -e "${RED}FAIL${NC} (unexpected NTR sequence)"
                FAILED=$((FAILED + 1))
                return
            fi
        fi
        # Extract output after *EXECUTE line
        if grep -q '\*EXECUTE' "$result_file"; then
            # Get everything after *EXECUTE until the separator line
            sed -n '/\*EXECUTE/,/^----/ p' "$result_file" | tail -n +2 | head -n -1 > "${result_file}.output"
            
            # Check if expected output file exists
            if [ -f "$expected_file" ]; then
                # Compare output
                if diff -q "${result_file}.output" "$expected_file" > /dev/null 2>&1; then
                    echo -e "${GREEN}PASS${NC}"
                    PASSED=$((PASSED + 1))
                else
                    echo -e "${RED}FAIL${NC} (output mismatch)"
                    echo "  Expected:"
                    cat "$expected_file" | head -3
                    echo "  Got:"
                    cat "${result_file}.output" | head -3
                    FAILED=$((FAILED + 1))
                fi
            else
                # No expected output, just check it compiled and ran
                echo -e "${GREEN}PASS${NC} (compiled and executed)"
                PASSED=$((PASSED + 1))
            fi
        else
            # Check if compilation failed (expected for some tests)
            if [ -f "${test_file%.p2c}.should_fail" ]; then
                echo -e "${GREEN}PASS${NC} (expected failure)"
                PASSED=$((PASSED + 1))
            else
                echo -e "${RED}FAIL${NC} (no execution)"
                FAILED=$((FAILED + 1))
            fi
        fi
    else
        # Runner exited nonzero for some reason other than the timeout.
        if [ -f "${test_file%.p2c}.should_fail" ]; then
            echo -e "${GREEN}PASS${NC} (expected failure)"
            PASSED=$((PASSED + 1))
        else
            echo -e "${RED}FAIL${NC} (crash, exit $rc)"
            FAILED=$((FAILED + 1))
        fi
    fi
}

# Main execution
echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}P2C Compiler Test Suite${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

# Check if runner exists
if [ ! -f "$RUNNER" ]; then
    echo -e "${RED}ERROR: $RUNNER not found${NC}"
    exit 1
fi

# Make runner executable
chmod +x "$RUNNER"

# Run all tests if no arguments given, otherwise run specified tests
if [ $# -eq 0 ]; then
    # Find all test files
    test_files=$(find "$TESTS_DIR" -name "*.p2c" | sort)
    
    if [ -z "$test_files" ]; then
        echo -e "${YELLOW}No test files found in $TESTS_DIR${NC}"
        exit 0
    fi
    
    for test_file in $test_files; do
        run_test "$test_file"
    done
else
    # Run specific tests
    for pattern in "$@"; do
        for test_file in "$TESTS_DIR"/${pattern}*.p2c; do
            if [ -f "$test_file" ]; then
                run_test "$test_file"
            fi
        done
    done
fi

# Print summary
echo ""
echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}Test Summary${NC}"
echo -e "${BLUE}========================================${NC}"
echo -e "Total:   $TOTAL"
echo -e "${GREEN}Passed:  $PASSED${NC}"
echo -e "${RED}Failed:  $FAILED${NC}"
echo -e "${YELLOW}Skipped: $SKIPPED${NC}"
echo ""

if [ $FAILED -eq 0 ]; then
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed${NC}"
    exit 1
fi
