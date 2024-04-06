import sys
import re
from github import Github


def parse_semantic_version(version):
    # Parse the semantic version and extract major and minor versions
    match = re.match(r"v?(\d+)\.(\d+)\.(\d+)", version)
    if match:
        print(match.groups())
        major, minor, patch = match.groups()
        return int(major), int(minor), int(patch)
    else:
        return None


def find_highest_dev_release(repo, major, minor):
    # Initialize a GitHub instance
    g = Github()

    # Get the repository object
    repository = g.get_repo(repo)

    # Fetch all releases
    releases = repository.get_releases()

    # Filter pre-releases with matching major and minor versions in their titles
    pre_releases = [
        release
        for release in releases
        if release.prerelease and release.title.startswith(f"v{major}.{minor}")
    ]

    # Extract DEV_RELEASE numbers from titles
    dev_releases = [int(release.title.split(".")[-1]) for release in pre_releases]

    # Find the highest DEV_RELEASE number
    highest_dev_release = max(dev_releases, default=0)

    return highest_dev_release


def determine_dev_release(version, repository):
    if version == "0":
        result = 0
    else:
        # Parse the semantic version to extract major and minor versions
        major, minor, _ = parse_semantic_version(version)
        if major is not None and minor is not None:
            result = find_highest_dev_release(repository, major, minor)
        else:
            print("Invalid semantic version format")
            sys.exit(1)

    print("Determined dev release version as ", result)

    return result
