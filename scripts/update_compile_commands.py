#!/usr/bin/env python3
"""Merge firmware and native host compile commands for one VS Code folder."""

import json
from pathlib import Path


def main():
    repo = Path(__file__).resolve().parent.parent
    sources = [
        repo / "firmware/ecat_tsn_gateway/build/compile_commands.json",
        repo / "host/hpm_gateway_ros2_ws/build/hpm_gateway_control/compile_commands.json",
        repo / "host/hpm_gateway_ros2_ws/build/hpm_gateway_gamepad/compile_commands.json",
    ]
    entries = {}
    for source in sources:
        if not source.is_file():
            continue
        for entry in json.loads(source.read_text()):
            directory = Path(entry["directory"])
            if not directory.is_absolute():
                directory = source.parent / directory
            directory = directory.resolve()
            file = Path(entry["file"])
            if not file.is_absolute():
                file = directory / file
            entry["directory"] = str(directory)
            entry["file"] = str(file.resolve())
            entries[entry["file"]] = entry

    output = repo / ".vscode/compile_commands.json"
    if not entries:
        output.unlink(missing_ok=True)
        raise SystemExit("No compile databases found; build firmware or host first.")
    content = json.dumps([entries[key] for key in sorted(entries)], indent=2) + "\n"
    if not output.exists() or output.read_text() != content:
        temporary = output.with_suffix(".json.tmp")
        temporary.write_text(content)
        temporary.replace(output)
    print(f"VS Code: {len(entries)} compile commands in {output}")


if __name__ == "__main__":
    main()
