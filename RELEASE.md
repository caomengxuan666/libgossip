# Release Process

This checklist keeps libgossip releases reproducible across the C++ library,
Python bindings, and CI publishing workflows.

## Preconditions

- Work from `develop`.
- Keep `CMakeLists.txt`, public version macros, and
  `bindings/python/pyproject.toml` on the same version.
- Ensure submodules are initialized with `git submodule update --init --recursive`.
- Ensure GitHub Actions secrets exist for PyPI publishing:
  `PYPI_API_TOKEN` and `TEST_PYPI_API_TOKEN`.

## Local Verification

Run these checks before tagging:

```bash
pip install build wheel twine pybind11==3.0.0
cmake --build build/msvc-debug --parallel
ctest --test-dir build/msvc-debug --output-on-failure
cd bindings/python
python -m build --wheel --sdist --no-isolation
```

Verify that the source distribution can rebuild a wheel outside the checkout:

```bash
python - <<'PY'
import pathlib
import subprocess
import sys
import tarfile
import tempfile

sdist = next(pathlib.Path("dist").glob("*.tar.gz"))
with tempfile.TemporaryDirectory() as tmp:
    with tarfile.open(sdist, "r:gz") as tf:
        tf.extractall(tmp)
    root = next(pathlib.Path(tmp).iterdir())
    subprocess.check_call([sys.executable, "-m", "build", "--wheel", "--no-isolation"], cwd=root)
PY
```

## Tagging

Use a `vMAJOR.MINOR.PATCH` tag matching the project version:

```bash
git tag -a v1.4.0 -m "libgossip 1.4.0"
git push origin v1.4.0
```

The Python publishing workflow builds wheels, builds the source distribution,
checks metadata with Twine, attaches release artifacts to the GitHub release,
and publishes on version tags. The local checklist above verifies the sdist
rebuild path before tagging.

## Post-release Checks

- Confirm the GitHub Actions release jobs passed.
- Confirm the GitHub release contains the wheel and sdist artifacts.
- Confirm the PyPI release page shows the expected version and artifacts.
- Install from PyPI in a clean environment:

```bash
python -m pip install --upgrade libgossip
python -c "import libgossip; print(libgossip.__file__)"
```

- Update release notes if the GitHub release was created manually.

## Adoption Work

Repository changes can make adoption easier, but they cannot prove external
standard status. After each release, publish examples, announce the version in
the target developer communities, and track real adoption signals such as
downloads, issues from external users, dependent projects, forks, and third-party
mentions.
