import os
import sys
import platform
from pybind11.setup_helpers import Pybind11Extension, build_ext
from setuptools import setup, find_packages

__version__ = "0.1.0"

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

setup(
    name="libgossip",
    version=__version__,
    author="caomengxuan666",
    author_email="caomengxuan666@gmail.com",
    url="https://github.com/caomengxuan666/libgossip",
    description="Python bindings for libgossip - a C++ Gossip protocol implementation",
    long_description="",
    ext_modules=ext_modules,
    extras_require={"test": "pytest"},
    cmdclass={"build_ext": build_ext},
    zip_safe=False,
    python_requires=">=3.7",
    packages=find_packages(),
)