# Changelog

## Unreleased

- Fixed UDP/TCP async send buffer lifetime issues on Windows.
- Cleaned MSVC build noise and aligned coverage defaults with actual tool availability.
- Enabled CI on `develop` and included example builds in the CMake workflow.
- Updated stale version markers and documentation references.
- Aligned Python package metadata version with the core library version.
- Made the Python source distribution self-contained and added a Python < 3.11
  `tomli` fallback for packaging.
- Added a CI check that rebuilds a wheel from the published Python sdist.
- Added GitHub Release artifact publishing alongside PyPI uploads.
- Added citation metadata for GitHub and downstream indexers.

## 1.4.0

- Pluggable serialization layer.
- High-level `gossip_manager` API.
- Core C and Python bindings.
- Improved example coverage and developer tooling.

## 1.3.0.0

- Added C/Python APIs for metadata updates.
- Added transport abstractions and deprecation path for legacy transports.
