Import("env")

import subprocess

MAJOR_VERSION = "0"

try:
    git_commit = subprocess.check_output(
        ["git", "rev-parse", "--short", "HEAD"]
    ).decode().strip()

    git_count = subprocess.check_output(
        ["git", "rev-list", "--count", "HEAD"]
    ).decode().strip()

    version = (
        f"{MAJOR_VERSION}."
        f"{git_count}-"
        f"{git_commit}"
    )

except:
    version = "unknown"

env.Append(
    CPPDEFINES=[
        ("GIT_VERSION", '\\"%s\\"' % version)
    ]
)