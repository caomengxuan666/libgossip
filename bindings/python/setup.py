import sys
import shutil
from pathlib import Path
from pybind11.setup_helpers import Pybind11Extension, build_ext
from setuptools import setup, find_packages
from setuptools.command.sdist import sdist as _sdist

try:
    import tomllib
except ModuleNotFoundError:
    import tomli as tomllib

HERE = Path(__file__).resolve().parent
PROJECT_ROOT = HERE.parents[1]
NATIVE_BUNDLE_DIR = HERE / "native"

PYPROJECT_PATH = HERE / "pyproject.toml"
PACKAGE_VERSION = tomllib.loads(PYPROJECT_PATH.read_text(encoding="utf-8"))["project"]["version"]


def ensure_generated_version_header():
    major, minor, patch = PACKAGE_VERSION.split(".")
    generated_include = HERE / "build" / "generated" / "include" / "core"
    generated_include.mkdir(parents=True, exist_ok=True)
    version_header = generated_include / "version.hpp"
    version_header.write_text(
        "\n".join(
            [
                "#pragma once",
                "",
                f"#define LIBGOSSIP_VERSION_MAJOR {major}",
                f"#define LIBGOSSIP_VERSION_MINOR {minor}",
                f"#define LIBGOSSIP_VERSION_PATCH {patch}",
                f'#define LIBGOSSIP_VERSION "{PACKAGE_VERSION}"',
                "",
                '#define LIBGOSSIP_NAME "libgossip"',
                "",
            ]
        ),
        encoding="utf-8",
    )
    return generated_include.parent


GENERATED_INCLUDE_DIR = ensure_generated_version_header()


def has_native_bundle():
    return (
        (NATIVE_BUNDLE_DIR / "include" / "core" / "gossip_core.hpp").exists()
        and (NATIVE_BUNDLE_DIR / "src" / "core" / "gossip_core.cpp").exists()
        and (NATIVE_BUNDLE_DIR / "third_party" / "asio" / "asio" / "include").exists()
        and (NATIVE_BUNDLE_DIR / "third_party" / "json" / "single_include").exists()
        and (NATIVE_BUNDLE_DIR / "third_party" / "magic_enum" / "magic_enum.hpp").exists()
    )


def native_path(*parts):
    if has_native_bundle():
        return str(Path("native").joinpath(*parts))
    return str(Path("..").joinpath("..", *parts))


class sdist(_sdist):
    def make_release_tree(self, base_dir, files):
        filtered_files = [
            path for path in files
            if not path.endswith((".pyd", ".so", ".dylib"))
        ]
        files[:] = filtered_files
        self.filelist.files[:] = filtered_files
        super().make_release_tree(base_dir, files)

        release_native = Path(base_dir) / "native"
        if (PROJECT_ROOT / "src").exists():
            source_roots = [
                (PROJECT_ROOT / "include", release_native / "include"),
                (PROJECT_ROOT / "src", release_native / "src"),
                (
                    PROJECT_ROOT / "third_party" / "asio" / "asio" / "include",
                    release_native / "third_party" / "asio" / "asio" / "include",
                ),
                (
                    PROJECT_ROOT / "third_party" / "json" / "single_include",
                    release_native / "third_party" / "json" / "single_include",
                ),
                (
                    PROJECT_ROOT / "third_party" / "magic_enum",
                    release_native / "third_party" / "magic_enum",
                ),
            ]
        elif has_native_bundle():
            source_roots = [(NATIVE_BUNDLE_DIR, release_native)]
        else:
            raise FileNotFoundError(
                "Cannot build libgossip sdist: native sources are missing. "
                "Build from the repository root checkout or from an existing sdist."
            )

        for source, destination in source_roots:
            if not source.exists():
                raise FileNotFoundError(f"Required native source path is missing: {source}")
            shutil.copytree(source, destination, dirs_exist_ok=True)

        release_native_files = [
            str(path.relative_to(base_dir))
            for path in release_native.rglob("*")
            if path.is_file()
        ]
        files.extend(release_native_files)
        self.filelist.files.extend(release_native_files)

DEFINE_MACROS = [("LIBGOSSIP_BUILD", None)]
if sys.platform == "win32":
    DEFINE_MACROS.extend([
        ("_WIN32_WINNT", "0x0601"),
        ("WIN32_LEAN_AND_MEAN", None),
    ])

# Define the extension module with relative paths
ext_modules = [
    Pybind11Extension(
        "libgossip.libgossip_py",
        [
            "gossip_py.cpp",
            native_path("src", "core", "gossip_core.cpp"),
            native_path("src", "core", "gossip_c.cpp"),
            native_path("src", "core", "node_id_utils.cpp"),
            native_path("src", "net", "udp_transport.cpp"),
            native_path("src", "net", "tcp_transport.cpp"),
            native_path("src", "net", "transport_factory.cpp"),
            native_path("src", "net", "serializer_factory.cpp"),
            native_path("src", "net", "json_serializer.cpp"),
        ],
        include_dirs=[
            native_path("include"),
            native_path("src"),
            native_path("third_party", "asio", "asio", "include"),
            native_path("third_party", "json", "single_include"),
            native_path("third_party", "magic_enum"),
            native_path("third_party"),
            str(GENERATED_INCLUDE_DIR),
        ],
        cxx_std=17,
        define_macros=DEFINE_MACROS,
        # Only add stdc++ library on non-Windows platforms
        libraries=["stdc++"] if sys.platform != "win32" else [],
        # Add extra flags for manylinux compatibility
        extra_compile_args=(
            ["-fvisibility=hidden"]
            if sys.platform != "win32"
            else ["/utf-8", "/wd4251", "/wd4996"]
        ),
    ),
]

# Only keep build-related configurations, metadata is provided by pyproject.toml
setup(
    ext_modules=ext_modules,
    cmdclass={"build_ext": build_ext, "sdist": sdist},
    packages=find_packages(),
    zip_safe=False,
    python_requires=">=3.8",
    # Note: Do not write name, version, description, etc. here!
)
