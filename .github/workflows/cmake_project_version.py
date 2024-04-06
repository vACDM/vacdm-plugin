""" Finds the project version defined in CMakeLists.txt"""

import sys
import re


def extract_version_from_cmakelists():
    # Define the pattern to search for in the file
    pattern = r'PROJECT\(.*?VERSION\s+"(\d+\.\d+\.\d+)"'

    # Read the contents of CMakeLists.txt
    with open("CMakeLists.txt", "r") as file:
        contents = file.read()

    # Search for the pattern
    match = re.search(pattern, contents)

    # If a match is found, extract the version
    if match:
        print("Found version number in CMakeLists.txt: ", match.group(1))
        return match.group(1)

    # exit if the version could not be found
    print("Could not find version in CMakeLists.txt")
    sys.exit(1)
