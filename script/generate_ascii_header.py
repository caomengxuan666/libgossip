#!/usr/bin/env python3

import json
import os
import re

# Zero Width space as a separator between header and content
HEADER_START_MARKER = "//\u200B"  # add the character U+200B as a zero-width space to make it invisible//
HEADER_END_MARKER = "//\u200BEND"

def extract_version_from_cmake(cmake_file_path="CMakeLists.txt"):
    """
    Extract version from CMakeLists.txt file
    
    Args:
        cmake_file_path: Path to CMakeLists.txt file
    
    Returns:
        str: Version string or None if not found
    """
    try:
        with open(cmake_file_path, 'r') as f:
            content = f.read()
            # Look for VERSION pattern in project() command
            match = re.search(r'project\s*\([^)]*VERSION\s+([0-9]+\.[0-9]+\.[0-9]+)', content, re.IGNORECASE)
            if match:
                return match.group(1)
    except Exception as e:
        print(f"Error reading version from {cmake_file_path}: {e}")
    return None

def generate_ascii_art_header(project_name="libgossip", repo_owner="caomengxuan666", version="1.0.0", year="2025", copyright_holder="Caomengxuan", ascii_art=None):
    """
    Generate ASCII art style header information with invisible markers
    
    Args:
        project_name: Project name
        repo_owner: GitHub repository owner
        version: Project version
        year: Copyright year
        copyright_holder: Copyright holder
        ascii_art: List of ASCII art lines
    
    Returns:
        str: Formatted ASCII art header with invisible markers
    """
    # Use default ASCII art if none provided
    if ascii_art is None:
        ascii_art = [
            " _ _ _                         _       ",
            "| (_) |__   __ _  ___  ___ ___(_)_ __  ",
            "| | | '_ \\ / _` |/ _ \\/ __/ __| | '_ \\",
            "| | | |_) | (_| | (_) \\__ \\__ \\ | |_) |",
            "|_|_|_.__/ \\__, |\\___/|___/___/_| .__/ ",
            "           |___/                |_|    "
        ]
    
    # Add comment prefix to each line of ASCII art
    ascii_art_lines = ["// " + line for line in ascii_art]
    
    # Create the complete header with invisible markers
    header = f"""{HEADER_START_MARKER}
""" + "\n".join(ascii_art_lines) + f"""
// Project: {project_name}
// Repository: https://github.com/{repo_owner}/{project_name}
// Version: {version}
//
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// SPDX-License-Identifier: MIT
// Copyright (c) {year} {copyright_holder}.
//
// Permission is hereby  granted, free of charge, to any  person obtaining a copy
// of this software and associated  documentation files (the "Software"), to deal
// in the Software  without restriction, including without  limitation the rights
// to  use, copy,  modify, merge,  publish, distribute,  sublicense, and/or  sell
// copies  of  the Software,  and  to  permit persons  to  whom  the Software  is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE  IS PROVIDED "AS  IS", WITHOUT WARRANTY  OF ANY KIND,  EXPRESS OR
// IMPLIED,  INCLUDING BUT  NOT  LIMITED TO  THE  WARRANTIES OF  MERCHANTABILITY,
// FITNESS FOR  A PARTICULAR PURPOSE AND  NONINFRINGEMENT. IN NO EVENT  SHALL THE
// AUTHORS  OR COPYRIGHT  HOLDERS  BE  LIABLE FOR  ANY  CLAIM,  DAMAGES OR  OTHER
// LIABILITY, WHETHER IN AN ACTION OF  CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE  OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
{HEADER_END_MARKER}

"""
    return header

def has_existing_header(content):
    """
    Check if content already has our header using invisible markers
    
    Args:
        content: File content as string
    
    Returns:
        bool: True if header exists, False otherwise
    """
    return HEADER_START_MARKER in content and HEADER_END_MARKER in content

def remove_existing_header(content):
    """
    Remove existing header from file content if it exists using invisible markers
    
    Args:
        content: File content as string
    
    Returns:
        str: Content with header removed
    """
    if has_existing_header(content):
        # Use regex to find and remove content between markers (including markers)
        pattern = re.escape(HEADER_START_MARKER) + r'.*?' + re.escape(HEADER_END_MARKER)
        content = re.sub(pattern, '', content, flags=re.DOTALL)
    
    return content.lstrip()

def insert_header_to_file(file_path, header):
    """
    Insert header information at the beginning of the file,
    replacing existing header if it exists using invisible markers
    
    Args:
        file_path: Target file path
        header: Header information to insert
    """
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()
        
        # Remove existing header if it exists
        content_without_header = remove_existing_header(content)
        
        # Write file with new header
        with open(file_path, 'w', encoding='utf-8') as f:
            f.write(header + content_without_header)
        
        print(f"✓ Header successfully {'updated' if has_existing_header(content) else 'inserted'} in {file_path}")
        
    except Exception as e:
        print(f"✗ Error processing {file_path}: {e}")

def load_config(config_path):
    """
    Load configuration from JSON file
    
    Args:
        config_path: Path to the configuration file
    
    Returns:
        dict: Configuration data
    """
    try:
        with open(config_path, 'r', encoding='utf-8') as f:
            return json.load(f)
    except Exception as e:
        print(f"Error loading config from {config_path}: {e}")
        return None

def process_files_from_config(config_path):
    """
    Process files specified in the configuration file
    
    Args:
        config_path: Path to the configuration file
    """
    config = load_config(config_path)
    if not config:
        print("Failed to load configuration")
        return
    
    project_info = config.get("project_info", {})
    ascii_art = config.get("ascii_art", None)
    files_to_process = config.get("files_to_process", [])
    
    # Get version from CMakeLists.txt if not specified in config
    version = project_info.get("version")
    if not version:
        version = extract_version_from_cmake()
        if not version:
            version = "1.0.0"  # fallback version
    
    # Generate header with project info
    header = generate_ascii_art_header(
        project_name=project_info.get("name", "libgossip"),
        repo_owner=project_info.get("repo_owner", "caomengxuan666"),
        version=version,
        year=project_info.get("year", "2025"),
        copyright_holder=project_info.get("copyright_holder", "Caomengxuan"),
        ascii_art=ascii_art
    )
    
    # Process each file
    for file_path in files_to_process:
        # Check if file exists
        if os.path.exists(file_path):
            insert_header_to_file(file_path, header)
        else:
            print(f"✗ File not found: {file_path}")

def create_sample_config():
    """Create a sample configuration file"""
    sample_config = {
        "project_info": {
            "name": "libgossip",
            "repo_owner": "caomengxuan666",
            "version": "",
            "year": "2025",
            "copyright_holder": "Caomengxuan"
        },
        "ascii_art": [
            " _ _ _                         _       ",
            "| (_) |__   __ _  ___  ___ ___(_)_ __  ",
            "| | | '_ \\ / _` |/ _ \\/ __/ __| | '_ \\ ",
            "| | | |_) | (_| | (_) \\__ \\__ \\ | |_) |",
            "|_|_|_.__/ \\__, |\\___/|___/___/_| .__/ ",
            "           |___/                |_|    "
        ],
        "files_to_process": [
            "src/main.cpp",
            "include/libgossip.h",
            "src/utils.cpp"
        ]
    }
    
    with open('header_config.json', 'w', encoding='utf-8') as f:
        json.dump(sample_config, f, indent=2, ensure_ascii=False)
    
    print("✓ Sample configuration file created: header_config.json")

if __name__ == "__main__":
    # Check if config file exists
    config_file = "header_config.json"
    if not os.path.exists(config_file):
        print("Config file not found. Creating sample config...")
        create_sample_config()
        print("Please edit header_config.json and run the script again.")
    else:
        print(f"Processing files from config: {config_file}")
        process_files_from_config(config_file)