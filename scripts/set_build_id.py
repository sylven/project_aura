Import("env")

import subprocess


def _run_git(args):
    try:
        return subprocess.check_output(
            ["git"] + args,
            cwd=env["PROJECT_DIR"],
            stderr=subprocess.DEVNULL,
            text=True,
        ).strip()
    except Exception:
        return ""


def resolve_build_id():
    short_sha = _run_git(["rev-parse", "--short=7", "HEAD"])
    if not short_sha:
        return "nogit"

    dirty = subprocess.call(
        ["git", "diff", "--quiet", "--ignore-submodules", "HEAD", "--"],
        cwd=env["PROJECT_DIR"],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    if dirty != 0:
        return f"{short_sha}-dirty"
    return short_sha


env.Append(CPPDEFINES=[("APP_BUILD_ID", env.StringifyMacro(resolve_build_id()))])
