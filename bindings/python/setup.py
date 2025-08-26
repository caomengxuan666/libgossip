from setuptools import setup, find_packages, Extension
from pybind11.setup_helpers import Pybind11Extension, build_ext
from pybind11 import get_cmake_dir
import pybind11
import os
from pathlib import Path

# Get the directory of this file
here = os.path.abspath(os.path.dirname(__file__))

# Read README.md for long description
with open(os.path.join(here, "README.md"), "r", encoding="utf-8") as fh:
    long_description = fh.read()

# Try to find the compiled extension, if not available, build from source
libgossip_py_path = Path("libgossip/libgossip_py")
compiled_module_exists = any(libgossip_py_path.with_suffix(suffix).exists() 
                            for suffix in [".pyd", ".so", ".dylib"])

ext_modules = []
if not compiled_module_exists:
    # Define the extension module
    ext_modules = [
        Pybind11Extension(
            "libgossip.libgossip_py",
            ["gossip_py.cpp"],
            include_dirs=[
                "../include",
                "../../include",
                pybind11.get_cmake_dir(),
            ],
            cxx_std=17,
            define_macros=[("LIBGOSSIP_BUILD", None)],
        ),
    ]

setup(
    name="libgossip",
    version="0.1.0",
    description="Python bindings for libgossip - a C++ Gossip protocol implementation",
    long_description=long_description,
    long_description_content_type="text/markdown",
    author="caomengxuan666",
    author_email="caomengxuan666@gmail.com",
    url="https://github.com/caomengxuan666/libgossip",
    packages=find_packages(where=".", include=["libgossip", "libgossip.*"]),
    package_data={
        'libgossip': ['libgossip_py.*'],  # Include compiled extensions
    },
    install_requires=[
        "pybind11>=2.6.0",
    ],
    classifiers=[
        "Development Status :: 3 - Alpha",
        "Intended Audience :: Developers",
        "License :: OSI Approved :: MIT License",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.7",
        "Programming Language :: Python :: 3.8",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
        "Programming Language :: Python :: 3.11",
        "Programming Language :: Python :: 3.12",
        "Programming Language :: Python :: 3.13",
        "Programming Language :: Python :: 3.14",
    ],
    python_requires=">=3.7",
    zip_safe=False,
    ext_modules=ext_modules,
    cmdclass={"build_ext": build_ext} if ext_modules else {},
)