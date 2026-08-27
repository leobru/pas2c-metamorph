#!/bin/bash
# Test-suite runner: compiles every tests/*.p2c with the host-native ./base
# (via runhotest.sh) and compares outputs against the .expected files.

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

    local rc=0
    timeout 10 ./$RUNNER "$test_file" > "$result_file" 2>&1 || rc=$?

    # A timeout is never an "expected failure".  .should_fail asserts that the
    # compiler *rejects* the program, not that it may spin: a hang there used
    # to be reported as a pass, which is how the switch-without-case spin
    # (tests/97) stayed invisible.  124 is timeout(1)'s SIGTERM kill, 137 a
    # SIGKILL that outlived it.  timeout signals the whole process group, so
    # ./base or dubna go down with the runner.
    if [ $rc -eq 124 ] || [ $rc -eq 137 ]; then
        echo -e "${RED}FAIL${NC} (timeout -- infinite loop?)"
        FAILED=$((FAILED + 1))
        return
    fi

    # The per-test runner deliberately returns success after a compiler
    # diagnostic so it can print the listing, and its compatibility trailer
    # contains an *EXECUTE line.  A .diagnostics sidecar makes a negative test
    # strict: each line is a required fragment in an actual compiler error.
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
            echo -e "${RED}FAIL${NC} (crash, exit $rc)"
            FAILED=$((FAILED + 1))
        fi
    fi
}

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}base / tests Suite${NC}"
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
