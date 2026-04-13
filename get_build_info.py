Import("env")
import subprocess
import datetime

def get_git_sha():
    try:
        return subprocess.check_output(
            ["git", "rev-parse", "--short=8", "HEAD"],
            stderr=subprocess.DEVNULL
        ).decode("utf-8").strip()
    except Exception:
        return "unknown"

def get_git_dirty():
    try:
        result = subprocess.check_output(
            ["git", "status", "--porcelain"],
            stderr=subprocess.DEVNULL
        ).decode("utf-8").strip()
        return "-dirty" if result else ""
    except Exception:
        return ""

sha = get_git_sha() + get_git_dirty()
build_time = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")

env.Append(CPPDEFINES=[
    ("BUILD_GIT_SHA", env.StringifyMacro(sha)),
    ("BUILD_TIMESTAMP", env.StringifyMacro(build_time)),
])
