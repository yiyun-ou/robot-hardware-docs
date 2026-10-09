#!/usr/bin/env python3
"""Download and restore large files listed in manifest.json.

For a private repository, authenticate with GH_TOKEN/GITHUB_TOKEN or `gh auth login`.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import shutil
import subprocess
import sys
import tempfile
import urllib.request
from pathlib import Path


def token() -> str | None:
    value = os.environ.get("GH_TOKEN") or os.environ.get("GITHUB_TOKEN")
    if value:
        return value
    try:
        result = subprocess.run(
            ["gh", "auth", "token"], capture_output=True, text=True, check=True
        )
        return result.stdout.strip() or None
    except (OSError, subprocess.CalledProcessError):
        return None


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(8 * 1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def download(url: str, target: Path, auth_token: str | None) -> None:
    headers = {"Accept": "application/octet-stream", "User-Agent": "robot-hardware-docs-restore"}
    if auth_token:
        headers["Authorization"] = f"Bearer {auth_token}"
        headers["X-GitHub-Api-Version"] = "2022-11-28"
    request = urllib.request.Request(url, headers=headers)
    with urllib.request.urlopen(request) as response, target.open("wb") as output:
        shutil.copyfileobj(response, output, length=8 * 1024 * 1024)


def restore(entry: dict, output_root: Path, auth_token: str | None, overwrite: bool) -> None:
    target = output_root / Path(entry["path"])
    target.parent.mkdir(parents=True, exist_ok=True)
    if target.exists() and not overwrite:
        if target.stat().st_size == entry["size"] and sha256(target) == entry["sha256"]:
            print(f"verified existing: {entry['path']}")
            return
        raise RuntimeError(f"Target exists and differs: {target}; use --overwrite")
    assets = entry.get("chunks") or [entry.get("release_asset")]
    assets = [item for item in assets if item]
    if not assets:
        raise RuntimeError(f"No release assets recorded for {entry['path']}")
    with tempfile.TemporaryDirectory(prefix="robot-hardware-restore-") as temp_dir:
        temp = Path(temp_dir)
        part_paths = []
        for index, asset in enumerate(assets):
            part = temp / f"part-{index:06d}"
            url = asset.get("api_url") or asset.get("url")
            if not url:
                raise RuntimeError(f"No download URL for {entry['path']} part {index}")
            print(f"downloading {entry['path']} part {index + 1}/{len(assets)}")
            download(url, part, auth_token)
            expected_part_size = asset.get("size")
            if expected_part_size is not None and part.stat().st_size != expected_part_size:
                raise RuntimeError(f"Part size mismatch for {entry['path']} part {index}")
            expected_part_hash = asset.get("sha256")
            if expected_part_hash and sha256(part) != expected_part_hash:
                raise RuntimeError(f"Part SHA-256 mismatch for {entry['path']} part {index}")
            part_paths.append(part)
        staging = target.with_name(target.name + ".restoring")
        with staging.open("wb") as output:
            for part in part_paths:
                with part.open("rb") as source:
                    shutil.copyfileobj(source, output, length=8 * 1024 * 1024)
        if staging.stat().st_size != entry["size"] or sha256(staging) != entry["sha256"]:
            staging.unlink(missing_ok=True)
            raise RuntimeError(f"Restored file verification failed: {entry['path']}")
        os.replace(staging, target)
    print(f"restored and verified: {entry['path']}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--manifest", type=Path, default=Path(__file__).resolve().parents[1] / "manifest.json")
    parser.add_argument("--output-dir", type=Path, default=Path("restored"))
    parser.add_argument("--path", action="append", dest="paths", help="restore only this relative path; repeatable")
    parser.add_argument("--overwrite", action="store_true")
    args = parser.parse_args()
    data = json.loads(args.manifest.read_text(encoding="utf-8"))
    entries = [item for item in data["files"] if item["storage"] in {"release", "release_chunks"}]
    if args.paths:
        wanted = set(args.paths)
        entries = [item for item in entries if item["path"] in wanted]
        missing = wanted - {item["path"] for item in entries}
        if missing:
            raise SystemExit("Not found among release files: " + ", ".join(sorted(missing)))
    auth_token = token()
    for entry in entries:
        restore(entry, args.output_dir, auth_token, args.overwrite)
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print(f"ERROR: {error}", file=sys.stderr)
        raise SystemExit(1)
