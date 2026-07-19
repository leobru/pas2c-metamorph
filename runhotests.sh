#!/bin/bash
# Test runner for p2c compiler test suite through native base + tmpbin.bin.
# Runs test programs using runhotest.sh and compares outputs.

set -e

RED='\033[1;31m'
GREEN='\033[1;32m'
YELLOW='\033[1;33m'
BLUE='\033[1;34m'
NC='\033[0m'

TESTS_DIR="tests"
RESULTS_DIR="test_results_hot"
RUNNER="runhotest.sh"

mkdir -p "$RESULTS_DIR"

TOTAL=0
PASSED=0
FAILED=0
SKIPPED=0

run_test() {
    local test_file="$1"
    local test_name=$(basename "$test_file" .p2c)
    local result_file="$RESULTS_DIR/${test_name}.result"
    local expected_file="${test_file%.p2c}.expected"

    TOTAL=$((TOTAL + 1))

    echo -ne "${BLUE}Running test: ${test_name}${NC} ... "

    if [ ! -f "$test_file" ]; then
        echo -e "${RED}SKIP${NC} (file not found)"
        SKIPPED=$((SKIPPED + 1))
        return
    fi

    if timeout 10 ./$RUNNER "$test_file" > "$result_file" 2>&1; then
        if grep -q '\*EXECUTE' "$result_file"; then
            sed -n '/\*EXECUTE/,/^----/ p' "$result_file" | tail -n +2 | head -n -1 > "${result_file}.output"

            if [ -f "$expected_file" ]; then
                if diff -q "${result_file}.output" "$expected_file" > /dev/null 2>&1; then
                    echo -e "${GREEN}PASS${NC}"
                    PASSED=$((PASSED + 1))
                else
                    echo -e "${RED}FAIL${NC} (output mismatch)"
                    echo "  Expected:"
                    head -3 "$expected_file"
                    echo "  Got:"
                    head -3 "${result_file}.output"
                    FAILED=$((FAILED + 1))
                fi
            else
                echo -e "${GREEN}PASS${NC} (compiled and executed)"
                PASSED=$((PASSED + 1))
            fi
        else
            if [ -f "${test_file%.p2c}.should_fail" ]; then
                echo -e "${GREEN}PASS${NC} (expected failure)"
                PASSED=$((PASSED + 1))
            else
                echo -e "${RED}FAIL${NC} (no execution)"
                FAILED=$((FAILED + 1))
            fi
        fi
    else
        if [ -f "${test_file%.p2c}.should_fail" ]; then
            echo -e "${GREEN}PASS${NC} (expected failure)"
            PASSED=$((PASSED + 1))
        else
            echo -e "${RED}FAIL${NC} (timeout or crash)"
            FAILED=$((FAILED + 1))
        fi
    fi
}

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}P2C Hot Test Suite${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

if [ ! -f "$RUNNER" ]; then
    echo -e "${RED}ERROR: $RUNNER not found${NC}"
    exit 1
fi

chmod +x "$RUNNER"

if [ $# -eq 0 ]; then
    test_files=$(find "$TESTS_DIR" -name "*.p2c" | sort)

    if [ -z "$test_files" ]; then
        echo -e "${YELLOW}No test files found in $TESTS_DIR${NC}"
        exit 0
    fi

    for test_file in $test_files; do
        run_test "$test_file"
    done
else
    for pattern in "$@"; do
        matched=0
        if [ -f "$pattern" ]; then
            run_test "$pattern"
            matched=1
        else
            for test_file in "$TESTS_DIR"/${pattern}*.p2c; do
                if [ -f "$test_file" ]; then
                    run_test "$test_file"
                    matched=1
                fi
            done
        fi
        if [ $matched -eq 0 ]; then
            echo -e "${RED}ERROR: no tests matched ${pattern}${NC}"
            FAILED=$((FAILED + 1))
        fi
    done
fi

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
