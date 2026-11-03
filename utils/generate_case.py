#!/usr/bin/env python3

"""
Generate test case for project 3
"""

from argparse import ArgumentParser
from random import choice, randint
from string import ascii_lowercase

ALPHABET = ascii_lowercase
"""
The alphabet to use for generating the input
"""

MAX_OCCURENCE = 255
"""
The maximum number of times a character can occur in a row
"""

def main():
    """
    Main function
    """

    # Parse arguments
    parser = ArgumentParser(description="Generate test case for project 3")
    parser.add_argument("input", type=str, help="Path to save the input file to")
    parser.add_argument(
        "size", type=int, help="Size of the (uncompressed) input in bytes"
    )

    args = parser.parse_args()

    # Generate the case input
    case_input = ""

    while len(case_input) < args.size:
        # Generate a random character different from the last one
        char = choice(ALPHABET)
        if len(case_input) > 0 and char == case_input[-1]:
            continue

        # Generate a random number of occurrences
        occurence = randint(1, min(MAX_OCCURENCE, args.size - len(case_input)))

        # Append the character to the input
        case_input += char * occurence

    with open(args.input, "w", encoding="utf-8") as file:
        file.write(case_input)

    # Log
    print(
        f"Generated input at {args.input} for {args.size} bytes"
    )


if __name__ == "__main__":
    main()
