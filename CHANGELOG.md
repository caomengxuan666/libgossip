# Changelog

## Unreleased

## 1.4.1

- Fixed Windows shared-library builds by moving the high-level `gossip_manager`
  implementation into the network library target.
- Kept `LIBGOSSIP_BUILD` private to library builds so installed CMake targets
  use import declarations instead of export declarations.

## 1.4.0

- Fixed UDP/TCP async send buffer lifetime issues on Windows.
- Cleaned MSVC build noise and aligned coverage defaults with actual tool availability.
- Enabled CI on `develop` and included example builds in the CMake workflow.
- Updated stale version markers and documentation references.
- Aligned Python package metadata version with the core library version.
- Made the Python source distribution self-contained and added a Python < 3.11
  `tomli` fallback for packaging.
- Added CI packaging checks for wheel and source distribution artifacts.
- Added GitHub Release artifact publishing alongside PyPI uploads.
- Added citation metadata for GitHub and downstream indexers.

- Pluggable serialization layer.
- High-level `gossip_manager` API.
- Core C and Python bindings.
- Improved example coverage and developer tooling.

## 1.3.0.0

- Added C/Python APIs for metadata updates.
- Added transport abstractions and deprecation path for legacy transports.
