#!/usr/bin/env python3
"""Fetch the official CPython Android 3.14.7 packages and stage them for Gradle/CMake."""
from pathlib import Path
import shutil
import tarfile
import tempfile
import urllib.request

VERSION = "3.14.8"
ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "android" / "app" / "build" / "cpython"
ASSETS = ROOT / "android" / "app" / "src" / "main" / "assets" / "python"
JNILIBS = ROOT / "android" / "app" / "src" / "main" / "jniLibs"

PACKAGES = {
    "arm64-v8a": "aarch64-linux-android",
    "x86_64": "x86_64-linux-android",
}


def download(url: str, target: Path) -> None:
    print(f"Downloading {url}")
    with urllib.request.urlopen(url, timeout=120) as src, target.open("wb") as dst:
        shutil.copyfileobj(src, dst)


def find_prefix(root: Path) -> Path:
    candidates = [p for p in root.rglob("prefix") if p.is_dir()]
    if candidates:
        return candidates[0]
    # Some packages unpack directly as a prefix-like tree.
    if (root / "lib" / f"libpython3.14.so").exists() or (root / "lib" / f"libpython3.14.so.1.0").exists():
        return root
    raise RuntimeError(f"Could not find CPython prefix in {root}")


def copy_tree(src: Path, dst: Path) -> None:
    if dst.exists(): shutil.rmtree(dst)
    shutil.copytree(src, dst, symlinks=True)


def prepare(abi: str, host: str) -> None:
    url = f"https://www.python.org/ftp/python/{VERSION}/python-{VERSION}-{host}.tar.gz"
    with tempfile.TemporaryDirectory(prefix="dreamengine-python-") as td:
        archive = Path(td) / "python.tar.gz"
        extracted = Path(td) / "extracted"
        extracted.mkdir()
        download(url, archive)
        with tarfile.open(archive, "r:gz") as tf:
            tf.extractall(extracted)
        prefix = find_prefix(extracted)

        out = OUT / abi
        if out.exists(): shutil.rmtree(out)
        shutil.copytree(prefix, out, symlinks=True)

        # Package every shared dependency supplied by CPython into the matching ABI.
        jni = JNILIBS / abi
        jni.mkdir(parents=True, exist_ok=True)
        for so in (prefix / "lib").glob("*.so*"):
            if so.is_file(): shutil.copy2(so, jni / so.name)

        # Python's standard library and its architecture-specific lib-dynload are assets.
        asset = ASSETS / abi
        if asset.exists(): shutil.rmtree(asset)
        libpy = prefix / "lib" / "python3.14"
        if not libpy.is_dir(): raise RuntimeError(f"Missing {libpy}")
        shutil.copytree(libpy, asset / "lib" / "python3.14", symlinks=True)

        # Keep a tiny marker used by the Java first-run extractor.
        (asset / "pyvenv.cfg").write_text(f"home = {out}\nversion = {VERSION}\n", encoding="utf-8")
        print(f"Prepared CPython {VERSION} for {abi}")


for abi, host in PACKAGES.items():
    prepare(abi, host)
