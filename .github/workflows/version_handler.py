""" """

import sys
import os
import re

from cmake_project_version import extract_version_from_cmakelists
from determine_dev_release import determine_dev_release

REPOSITORY_NAME = os.environ.get("REPOSITORY")
REF = os.environ.get("REF")

# determine the branch that triggered the workflow
match = re.match(r"refs/heads/(.*)", REF)
if not match:
    sys.exit(1)

branch_name = match.group(1)
print("Determined branch name: ", branch_name)

version = extract_version_from_cmakelists()

is_dev_branch = branch_name == "develop"
dev_release = None
if is_dev_branch:
    last_dev_release = determine_dev_release(version, REPOSITORY_NAME)
    dev_release = str(last_dev_release + 1)
    version += "-dev." + dev_release

# Write the version and dev release to GitHub environment variable
with open(os.getenv("GITHUB_ENV"), "a") as env_file:
    env_file.write(f"VERSION={version}\n")
    if is_dev_branch and dev_release:
        env_file.write(f"DEV_RELEASE={dev_release}\n")
