#!/usr/bin/env python3
from __future__ import annotations

import os
import random
import shutil
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
BUILD_DIR = ROOT / "build" / "codeforces_tests"
ALPHABET = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#%:;^&*()-."
SOLUTIONS = ["A1m", "A1q", "A1r", "A1rq"]
SEED = 20260524


def compiler() -> str:
    env_cxx = os.environ.get("CXX")
    if env_cxx:
        return env_cxx
    for candidate in ("clang++", "g++"):
        found = shutil.which(candidate)
        if found:
            return found
    raise RuntimeError("C++ compiler not found")


def compile_solution(name: str, cxx: str) -> Path:
    BUILD_DIR.mkdir(parents=True, exist_ok=True)
    source = ROOT / "codeforces" / f"{name}.cpp"
    binary = BUILD_DIR / name
    command = [
        cxx,
        "-std=c++17",
        "-O2",
        "-Wall",
        "-Wextra",
        "-pedantic",
        str(source),
        "-o",
        str(binary),
    ]
    subprocess.run(command, check=True)
    return binary


def make_input(values: list[str]) -> str:
    if not values:
        return "0\n"
    return f"{len(values)}\n" + "\n".join(values) + "\n"


def run_case(binary: Path, values: list[str]) -> None:
    expected = sorted(values)
    completed = subprocess.run(
        [str(binary)],
        input=make_input(values),
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=True,
    )
    actual = completed.stdout.splitlines()
    if actual != expected:
        raise AssertionError(
            f"{binary.name} failed\ninput={values!r}\nexpected={expected!r}\nactual={actual!r}\n"
            f"stderr={completed.stderr}"
        )


def random_string(rng: random.Random, min_len: int = 1, max_len: int = 40) -> str:
    length = rng.randint(min_len, max_len)
    return "".join(rng.choice(ALPHABET) for _ in range(length))


def test_cases() -> list[list[str]]:
    rng = random.Random(SEED)
    cases: list[list[str]] = [
        [],
        ["ac", "ab", "aa", "ae", "ad"],
        ["aaaaaaaaaa"],
        ["samevalue0", "samevalue0", "samevalue0"],
        ["prefix0000", "prefix000", "prefix00000", "prefix001"],
        list(ALPHABET),
        sorted(random_string(rng, 10, 20) for _ in range(80)),
    ]
    cases.append(list(reversed(cases[-1])))

    for size in [1, 2, 3, 7, 31, 74, 75, 100, 250]:
        values = [random_string(rng) for _ in range(size)]
        cases.append(values)
        cases.append(sorted(values))
        cases.append(list(reversed(sorted(values))))

    for _ in range(80):
        size = rng.randint(0, 200)
        values = [random_string(rng) for _ in range(size)]
        if values and rng.random() < 0.25:
            prefix = random_string(rng, 5, 15)
            values = [prefix + random_string(rng, 1, 20) for _ in range(size)]
        cases.append(values)

    return cases


def ensure_no_using_namespace_std() -> None:
    offenders = []
    forbidden = "using namespace " + "std"
    for name in SOLUTIONS:
        source = (ROOT / "codeforces" / f"{name}.cpp").read_text(encoding="utf-8")
        if forbidden in source:
            offenders.append(name)
    if offenders:
        raise AssertionError("forbidden namespace directive found in: " + ", ".join(offenders))


def main() -> None:
    ensure_no_using_namespace_std()
    cxx = compiler()
    binaries = {name: compile_solution(name, cxx) for name in SOLUTIONS}
    cases = test_cases()

    total = 0
    for name, binary in binaries.items():
        for values in cases:
            run_case(binary, values)
            total += 1
        print(f"{name}: {len(cases)} tests passed")

    print(f"all tests passed: {total} cases")


if __name__ == "__main__":
    main()
