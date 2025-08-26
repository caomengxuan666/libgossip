"""
libgossip top-level package
"""

# This file is intentionally left blank
# It makes the bindings/python directory a Python package
# This allows running examples from this directory

import sys
import os

# Add the libgossip subdirectory to the path so the examples can import the module
_current_dir = os.path.dirname(__file__)
_libgossip_dir = os.path.join(_current_dir, 'libgossip')
if _libgossip_dir not in sys.path:
    sys.path.insert(0, _libgossip_dir)