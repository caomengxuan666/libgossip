import os
import sys
from pybind11.setup_helpers import Pybind11Extension, build_ext
from setuptools import setup, find_packages

# Define the extension module with relative paths
ext_modules = [
    Pybind11Extension(
        "libgossip.libgossip_py",
        [
            "gossip_py.cpp",
            "../../src/core/gossip_core.cpp",
            "../../src/core/gossip_c.cpp",
            "../../src/net/udp_transport.cpp",
            "../../src/net/tcp_transport.cpp",
            "../../src/net/transport_factory.cpp",
            "../../src/net/json_serializer.cpp"
        ],
        include_dirs=[
            "../../include",
            "../../src",
            "../../third_party/asio/asio/include",
            "../../third_party/json/single_include/nlohmann",
            "../../third_party/magic_enum"
        ],
        cxx_std=17,
        define_macros=[("LIBGOSSIP_BUILD", None)],
        # Only add stdc++ library on non-Windows platforms
        libraries=["stdc++"] if sys.platform != "win32" else [],
    ),
]

# Only keep build-related configurations, metadata is provided by pyproject.toml
setup(
    ext_modules=ext_modules,
    cmdclass={"build_ext": build_ext},
    packages=find_packages(),
    zip_safe=False,
    python_requires=">=3.8",
    # Note: Do not write name, version, description, etc. here!
)