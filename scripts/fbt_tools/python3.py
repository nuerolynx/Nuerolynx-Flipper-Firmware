import os


def generate(env):
    py_name = "python3"
    if env["PLATFORM"] == "win32":
        # On Windows, Python 3 executable is usually just "python"
        if toolchain_root := os.environ.get("FBT_TOOLCHAIN_ROOT"):
            # SCons expands PYTHON3 into command strings. Quote the bundled
            # interpreter path because the repository may live below a user
            # directory containing spaces.
            py_name = f'"{os.path.join(toolchain_root, "python", "python.exe")}"'
        else:
            py_name = "python"

    env.SetDefault(
        PYTHON3=py_name,
    )


def exists(env):
    return True
