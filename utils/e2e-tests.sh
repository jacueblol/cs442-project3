#!/usr/bin/env bash

# End to end (e2e) tests
# Run this via make e2e-tests

################################################################################
#                                   Settings                                   #
################################################################################

# Test cases to run (One or more of: "tiny" "small" "large")
CASES=("tiny" "small" "large")

# Thread counts to test (One or more of: "2" "4" "8")
THREAD_COUNTS=("2" "4" "8")

# Mode to test (One or more of: "normal" "debug")
MODES=("normal" "debug")

################################################################################
#                                  Internals                                   #
################################################################################

# Exit on undefined variables and pipe failures
set -uo pipefail

# ANSI color codes
GRAY="\033[0;37m"
RED="\033[0;31m"
GREEN="\033[0;32m"
YELLOW="\033[0;33m"
RESET="\033[0m"

# Logging prefixes
DEBUG="${GRAY}[DEBUG]${RESET}"
SUCCESS="${GREEN}[SUCCESS]${RESET}"
WARNING="${YELLOW}[WARNING]${RESET}"
ERROR="${RED}[ERROR]${RESET}"

# Get the directory of the project root
ROOT_DIR="$(dirname "$(dirname "$(readlink -f "${0}")")")"

# Get the path to the binary
BINARY="${ROOT_DIR}/pzip.debug"

# Run a test
# Parameters:
# - $1: The case
# - $2: The thread count
# - $3: The mode
# Globals:
# - BINARY: The path to the binary
# - ROOT_DIR: The directory of the project root
function run_test {
	# Generate the input and output file names
  local INPUT_NAME="${ROOT_DIR}/tests/input/${1}"

  # Normal mode
  local OUTPUT_BASE_NAME="${ROOT_DIR}/tests/output/${1}/${2}"
  if [ "${3}" == "debug" ]; then
    local OUTPUT_BASE_NAME="${OUTPUT_BASE_NAME}_debug"
  fi
  local OUTPUT_BASE_NAME="${OUTPUT_BASE_NAME}"
	local OUTPUT_EXEPECTED_NAME="${OUTPUT_BASE_NAME}.expected"
  local OUTPUT_ACTUAL_NAME="${OUTPUT_BASE_NAME}.actual"
  local OUTPUT_DIFF_NAME="${OUTPUT_BASE_NAME}.diff"

    # Skip the test if the expected output does not exist
  if [ ! -f "${OUTPUT_EXEPECTED_NAME}" ]; then
    return 0
  fi


  local COMMAND="${BINARY} ${INPUT_NAME} ${OUTPUT_ACTUAL_NAME} ${2}"

  # Debug mode
  if [ "${3}" == "debug" ]; then
    COMMAND="${COMMAND} --debug"
  fi

	# Delete old output files
	rm -f ${OUTPUT_BASE_NAME}.{actual,diff}

	# Run the command
	echo -e "${DEBUG} Running: ${COMMAND}"
  ${COMMAND}
  if [ "${?}" -ne 0 ]; then
		echo -e "${ERROR} Failed to run the command ${COMMAND}!"
		return 1
	fi

  # Compare the output
  if [ "${3}" == "debug" ]; then
    local DIFF=$(diff --label "Expected output" --label "Actual output" --unified "${OUTPUT_ACTUAL_NAME}" "${OUTPUT_EXEPECTED_NAME}")
  else
    local DIFF=$(diff --label "Expected output" --label "Actual output" --unified <(xxd "${OUTPUT_ACTUAL_NAME}") <(xxd "${OUTPUT_EXEPECTED_NAME}"))
  fi

  if [ "${DIFF}" != "" ]; then
    echo "${DIFF}" > "${OUTPUT_DIFF_NAME}"
    echo -e "${ERROR} Test failed! (Case: ${1}, threads: ${2}, mode: ${3}, diff: ${OUTPUT_DIFF_NAME})"
    return 1
  fi

  echo -e "${SUCCESS} Test passed! (Case: ${1}, threads: ${2}, mode: ${3})"

	return 0
}

# Check if invoked by Make
if [ "${MAKELEVEL:-0}" -ne 1 ]; then
  echo -e "${ERROR} This script should be run via make e2e-tests!"
  exit 1
fi

# Run end to end tests
ALL_TESTS_PASSED=true
for CASE in "${CASES[@]}"; do
  for THREAD_COUNT in "${THREAD_COUNTS[@]}"; do
    for MODE in "${MODES[@]}"; do
      run_test "${CASE}" "${THREAD_COUNT}" "${MODE}"

      if [ "${?}" -ne 0 ]; then
          ALL_TESTS_PASSED=false
      fi
    done
  done
done

# Print the result
if [ "${ALL_TESTS_PASSED}" == false ]; then
	echo -e "${ERROR} Some tests failed!"
	exit 1
else
	echo -e "${SUCCESS} All configured end to end tests passed!"
	exit 0
fi
