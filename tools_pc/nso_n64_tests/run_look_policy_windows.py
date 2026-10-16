#!/usr/bin/env python3
"""Test the actual Windows engine objects without starting the game or using a ROM.

Requires a completed MinGW Makefiles build with compile_commands.json and SDL2.dll
in the build folder. All generated files stay in <build>/nso-look-policy-test/.
The game executable, objects, data directory and saves are not modified.
"""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("build", type=Path)
    args = parser.parse_args()
    if os.name != "nt":
        parser.error("This runner needs a native Windows MinGW Makefiles build")
    build = args.build.resolve(strict=True)
    out = build / "nso-look-policy-test"
    out.mkdir(exist_ok=True)
    commands = json.loads((build / "compile_commands.json").read_text())
    entry = next(c for c in commands if c["file"].replace("\\", "/").endswith("/port/src/watchsettings.c"))
    prefix = entry["command"].split(" -o ")[0]
    # The compiler path is the first command word in CMake's Windows record.
    if prefix.startswith('"'):
        compiler = prefix.split('"', 2)[1]
    else:
        compiler = prefix.split(" ", 1)[0]
    bin_dir = Path(compiler).parent
    env = os.environ.copy()
    env["PATH"] = str(bin_dir) + os.pathsep + env["PATH"]

    def run(command, **kwargs):
        return subprocess.run(command, cwd=build, env=env, check=True, **kwargs)

    source = Path(__file__).with_name("test_look_policy.c").resolve()
    run(prefix + f' -o "{out / "test.obj"}" -c "{source}"')
    archive = out / "objects.a"
    shutil.copy2(build / "CMakeFiles/ge007.dir/objects.a", archive)
    # Keep the real engine code; replace only its entry point with the test.
    main_obj = out / "main.c.obj"
    shutil.copy2(build / "CMakeFiles/ge007.dir/port/src/main.c.obj", main_obj)
    run([str(bin_dir / "objcopy.exe"), "--redefine-sym", "main=gameMain",
         "--redefine-sym", "SDL_main=gameSdlMain", str(main_obj)])
    run([str(bin_dir / "ar.exe"), "r", str(archive), str(main_obj)])
    link = (build / "CMakeFiles/ge007.dir/link.txt").read_text().splitlines()[-1]
    replacements = {
        "CMakeFiles\\ge007.dir/objects.a": f'"{archive}"',
        "-o ge007.x86_64.exe": f'"{out / "test.obj"}" -o "{out / "test.exe"}"',
        "-Wl,--out-implib,libge007.x86_64.dll.a ": "",
    }
    # Reject an unfamiliar generator/architecture before running a command
    # that could otherwise still point at the original game output.
    if any(link.count(old) != 1 for old in replacements):
        parser.error("Unexpected link command; use an x86-64 MinGW Makefiles build")
    for old, new in replacements.items():
        link = link.replace(old, new)
    run(link)
    shutil.copy2(build / "SDL2.dll", out)
    startup = subprocess.STARTUPINFO()
    startup.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    startup.wShowWindow = 0
    result = subprocess.run([str(out / "test.exe")], cwd=out, env=env,
                            capture_output=True, text=True, timeout=30,
                            startupinfo=startup, creationflags=subprocess.CREATE_NO_WINDOW)
    print(result.stdout, end="")
    if result.stderr:
        print(result.stderr, end="")
    result.check_returncode()


if __name__ == "__main__":
    main()
