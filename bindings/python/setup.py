import os
from pybind11.setup_helpers import Pybind11Extension, build_ext
from setuptools import setup, find_packages

# Determine the source directory
src_dir = os.path.dirname(os.path.abspath(__file__))
libgossip_root = os.path.join(src_dir, "../..")

# Define the extension module
ext_modules = [
    Pybind11Extension(
        "libgossip.libgossip_py",
        [
            "gossip_py.cpp",
            "../../src/core/gossip_core.cpp",
            "../../src/core/gossip_c.cpp"
        ],
        include_dirs=[
            "../../include",
            "../../src",
        ],
        cxx_std=17,
        define_macros=[("LIBGOSSIP_BUILD", None)],
    ),
]

# Only keep build-related configurations, metadata is provided by pyproject.toml
setup(
    ext_modules=ext_modules,
    cmdclass={"build_ext": build_ext},
    packages=find_packages(),
    zip_safe=False,
    python_requires=">=3.7",
    # Note: Do not write name, version, description, etc. here!
)