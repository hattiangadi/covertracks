#!/usr/bin/env python3

import re
import sys

def convert_beat_patterns(content):
    # Convert {"beat": x, "primitive": "y"} to [x, "y"]
    content = re.sub(
        r'\{"beat":\s*([0-9.]+),\s*"primitive":\s*"([^"]+)"\}',
        r'[\1, "\2"]',
        content
    )
    
    # Convert {"beat": x, "chord": "y"} to [x, {"chord": "y"}]
    content = re.sub(
        r'\{"beat":\s*([0-9.]+),\s*"chord":\s*"([^"]+)"\}',
        r'[\1, {"chord": "\2"}]',
        content
    )
    
    return content

def main():
    file_path = '/Users/anand/src/covertracks/Source/Tests/PatternParserTests.cpp'
    
    with open(file_path, 'r') as f:
        content = f.read()
    
    # Convert patterns
    content = convert_beat_patterns(content)
    
    with open(file_path, 'w') as f:
        f.write(content)
    
    print("Converted verbose patterns to compact format")

if __name__ == "__main__":
    main()