# Testing libgossip Python Bindings

This document explains how to run tests for the libgossip Python bindings.

## Prerequisites

Before running the tests, make sure you have the required dependencies installed:

```bash
pip install -r requirements-test.txt
```

This will install:
- pytest
- pytest-cov (for coverage reports)

## Running Tests

### Method 1: Direct pytest execution

From the `bindings/python` directory, run:

```bash
python -m pytest test_libgossip.py -v
```

### Method 2: Using CMake targets

If you have built the project with `BUILD_TESTS=ON` and `BUILD_PYTHON_BINDINGS=ON`, you can use CMake targets:

```bash
# Build the project first
cd build
cmake .. -DBUILD_TESTS=ON -DBUILD_PYTHON_BINDINGS=ON
make

# Run Python tests
make python_tests

# Or build and test in one command
make build_and_test_python
```

### Method 3: From the root directory

From the project root directory:

```bash
make run_python_tests
# or
make all_tests  # Runs both C++ and Python tests
```

## Test Coverage

To generate a coverage report for the Python tests:

```bash
python -m pytest test_libgossip.py --cov=libgossip --cov-report=html
```

This will generate an HTML coverage report in the `htmlcov` directory.

## Writing New Tests

Tests are written using the pytest framework. Add new test functions to `test_libgossip.py` following the existing patterns:

1. Use descriptive function names starting with `test_`
2. Include a docstring explaining what the test does
3. Use assertions to check expected behavior
4. Follow the existing code style

Example:

```python
def test_feature_name():
    """Test description of what this test verifies."""
    # Setup
    obj = MyClass()
    
    # Exercise
    result = obj.method()
    
    # Verify
    assert result == expected_value
```