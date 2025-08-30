#!/usr/bin/env python3
"""
Generate filtered code coverage report for libgossip project.

This script generates a code coverage report that excludes system libraries,
third-party libraries, and test frameworks to focus only on the actual 
project code coverage.
"""

import os
import sys
import subprocess
import argparse
import shutil
from pathlib import Path

def check_tools():
    """Check if required tools are available."""
    tools = ['lcov', 'genhtml', 'ctest']
    missing_tools = []
    
    for tool in tools:
        if not shutil.which(tool):
            missing_tools.append(tool)
    
    if missing_tools:
        print(f"Error: Missing required tools: {', '.join(missing_tools)}")
        print("Please install them using your package manager, e.g.:")
        print("  sudo apt-get install lcov")
        sys.exit(1)

def run_command(cmd, cwd=None, ignore_failure=False):
    """Run a shell command and return the result."""
    print(f"Running command: {cmd}")
    try:
        result = subprocess.run(
            cmd, 
            shell=True, 
            cwd=cwd,
            check=True, 
            capture_output=True, 
            text=True
        )
        if result.stdout:
            print(f"stdout: {result.stdout}")
        return result.stdout.strip()
    except subprocess.CalledProcessError as e:
        print(f"Error running command: {cmd}")
        if e.stdout:
            print(f"stdout: {e.stdout}")
        if e.stderr:
            print(f"stderr: {e.stderr}")
        if not ignore_failure:
            sys.exit(1)
        return None

def run_tests(build_dir):
    """Run tests to generate coverage data."""
    build_path = Path(build_dir).resolve()
    print("Running tests to generate coverage data...")
    try:
        result = subprocess.run(
            ["ctest", "-V"],
            cwd=build_path,
            check=True,
            capture_output=True,
            text=True
        )
        print("Tests completed successfully.")
        print(result.stdout)
        return True
    except subprocess.CalledProcessError as e:
        print("Error running tests:")
        print(e.stdout)
        print(e.stderr)
        return False

def generate_filtered_coverage(build_dir, output_dir, run_tests_first):
    """Generate filtered coverage report."""
    build_path = Path(build_dir).resolve()
    output_path = Path(output_dir).resolve()
    
    # Create output directory
    output_path.mkdir(parents=True, exist_ok=True)
    
    print(f"Generating coverage report in {build_path}")
    print(f"Output will be saved to {output_path}")
    
    # Step 0: Run tests if requested
    if run_tests_first:
        print("0. Running tests...")
        if not run_tests(build_dir):
            print("Tests failed. Continuing with coverage generation anyway...")
    
    # Step 1: Reset counters to zero
    print("1. Resetting coverage counters...")
    run_command(
        f"lcov --directory . --zerocounters",
        cwd=build_path,
        ignore_failure=True
    )
    
    # Step 2: Capture initial coverage data (baseline)
    print("2. Capturing initial coverage baseline...")
    run_command(
        f"lcov --capture --initial --directory . --output-file coverage_base.info "
        f"--ignore-errors mismatch,version,gcov,empty",
        cwd=build_path,
        ignore_failure=True
    )
    
    # Step 3: Run tests again to generate coverage data
    if not run_tests_first:
        print("3. Running tests...")
        run_tests(build_dir)
    
    # Step 4: Capture coverage data after tests
    print("4. Capturing coverage data after tests...")
    run_command(
        f"lcov --capture --directory . --output-file coverage_test.info "
        f"--ignore-errors mismatch,version,gcov,empty",
        cwd=build_path,
        ignore_failure=True
    )
    
    # Step 5: Combine baseline and test coverage data
    print("5. Combining baseline and test coverage data...")
    run_command(
        f"lcov --add-tracefile coverage_base.info --add-tracefile coverage_test.info "
        f"--output-file coverage.info --ignore-errors mismatch,version,gcov,empty",
        cwd=build_path,
        ignore_failure=True
    )
    
    # Check if coverage.info was created
    coverage_info_path = build_path / "coverage.info"
    if not coverage_info_path.exists():
        print("Error: coverage.info was not generated.")
        return
    
    # Step 6: Remove system libraries and third-party code
    print("6. Filtering out system and third-party code...")
    filters = [
        "/usr/include/*",
        "/usr/lib/*",
        "*/googletest/*",
        "*/gtest/*",
        "*/third_party/*",
        "*/build/*",
        "*/tests/*"
    ]
    
    filter_cmd = "lcov "
    for filter_pattern in filters:
        filter_cmd += f"--remove coverage.info '{filter_pattern}' "
    
    filter_cmd += "--output-file coverage_filtered.info --ignore-errors unused,mismatch,version,gcov,empty"
    run_command(filter_cmd, cwd=build_path, ignore_failure=True)
    
    # Step 7: Generate HTML report
    print("7. Generating HTML report...")
    html_result = run_command(
        f"genhtml coverage_filtered.info --output-directory {output_path} --ignore-errors empty",
        cwd=build_path,
        ignore_failure=True
    )
    
    if html_result is None:
        # Try with minimal options
        print("Trying alternative genhtml command...")
        run_command(
            f"genhtml coverage_filtered.info --output-directory {output_path}",
            cwd=build_path,
            ignore_failure=True
        )
    
    # Step 8: Show summary
    print("\n8. Coverage Summary:")
    try:
        # Try to extract summary information
        summary = run_command(
            "lcov --summary coverage_filtered.info",
            cwd=build_path,
            ignore_failure=True
        )
        if summary:
            print(summary)
        else:
            print("Could not generate summary, but report was created successfully.")
    except:
        print("Could not generate summary, but report was created successfully.")
    
    print(f"\nCoverage report generation completed!")
    report_index = output_path / "index.html"
    if report_index.exists():
        print(f"Open {report_index} in your browser to view the report.")
    else:
        print("Warning: index.html was not generated. Check the output above for errors.")

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        '--build-dir', 
        default='build',
        help='Build directory (default: build)'
    )
    parser.add_argument(
        '--output-dir',
        default='build/coverage_report_filtered',
        help='Output directory for HTML report (default: build/coverage_report_filtered)'
    )
    parser.add_argument(
        '--run-tests',
        action='store_true',
        help='Run tests before generating coverage report'
    )
    
    args = parser.parse_args()
    
    # Check if required tools are available
    check_tools()
    
    # Generate filtered coverage report
    generate_filtered_coverage(args.build_dir, args.output_dir, args.run_tests)

if __name__ == '__main__':
    main()