#!/bin/bash
# Test suite through the work compiler under dispak.
set -e

RED='\033[1;31m'
GREEN='\033[1;32m'
YELLOW='\033[1;33m'
BLUE='\033[1;34m'
NC='\033[0m'

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$ROOT"

TESTS_DIR="tests"
RESULTS_DIR="test_results_dispak"
RUNNER="dispak/runworktest.sh"

case "$1" in
    -work) shift ;;
esac

mkdir -p "$RESULTS_DIR"

TOTAL=0
PASSED=0
FAILED=0
SKIPPED=0

run_test() {
    local test_file=$1
    local test_name
    test_name=$(basename "$test_file" .p2c)
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
    ( ulimit -t 10; exec ./"$RUNNER" "$test_file" ) > "$result_file" 2>&1 || rc=$?

    if [ "$rc" -eq 137 ]; then
        echo -e "${RED}FAIL${NC} (CPU cap -- infinite loop?)"
        FAILED=$((FAILED + 1))
        return
    fi

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

    if [ "$rc" -eq 0 ]; then
        if [ -f "${test_file%.p2c}.should_fail" ]; then
            echo -e "${RED}FAIL${NC} (expected to fail)"
            FAILED=$((FAILED + 1))
            return
        fi
        if [ -f "$expected_file" ]; then
            # After *EXECUTE, keep program output only; drop Monitor-80 epilogue.
            if awk 'BEGIN{p=0} /\*EXECUTE/{p=1; next} /КОНЕЦ ЗАДАЧИ/{exit} p' "$result_file" | diff -u "$expected_file" - >/dev/null; then
                echo -e "${GREEN}PASS${NC}"
                PASSED=$((PASSED + 1))
            else
                echo -e "${RED}FAIL${NC} (output mismatch)"
                FAILED=$((FAILED + 1))
            fi
        else
            echo -e "${YELLOW}PASS${NC} (no .expected)"
            PASSED=$((PASSED + 1))
        fi
    else
        if [ -f "${test_file%.p2c}.should_fail" ]; then
            echo -e "${GREEN}PASS${NC} (expected failure)"
            PASSED=$((PASSED + 1))
        else
            echo -e "${RED}FAIL${NC}"
            FAILED=$((FAILED + 1))
        fi
    fi
}

if [ $# -gt 0 ]; then
    for t in "$@"; do
        run_test "$t"
    done
else
    for t in "$TESTS_DIR"/*.p2c; do
        run_test "$t"
    done
fi

echo
echo "Total: $TOTAL  Passed: $PASSED  Failed: $FAILED  Skipped: $SKIPPED"
[ "$FAILED" -eq 0 ]
