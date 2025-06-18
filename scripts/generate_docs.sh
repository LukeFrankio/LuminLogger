#!/bin/bash

# Exit on error
set -e

# Check if Doxygen is installed
if ! command -v doxygen &> /dev/null; then
    echo "Doxygen is not installed. Please install it first."
    exit 1
fi

# Create docs/doxygen directory if it doesn't exist
mkdir -p docs/doxygen

# Generate Doxyfile if it doesn't exist
if [ ! -f docs/Doxyfile ]; then
    echo "Generating Doxyfile..."
    doxygen -g docs/Doxyfile
    
    # Update Doxyfile with project settings
    sed -i 's/PROJECT_NAME           = "My Project"/PROJECT_NAME           = "LuminLogger"/' docs/Doxyfile
    sed -i 's/PROJECT_BRIEF          =/PROJECT_BRIEF          = "A modern C++ logging library"/' docs/Doxyfile
    sed -i 's/OUTPUT_DIRECTORY       =/OUTPUT_DIRECTORY       = docs\/doxygen/' docs/Doxyfile
    sed -i 's/EXTRACT_ALL            = NO/EXTRACT_ALL            = YES/' docs/Doxyfile
    sed -i 's/EXTRACT_PRIVATE        = NO/EXTRACT_PRIVATE        = YES/' docs/Doxyfile
    sed -i 's/EXTRACT_STATIC         = NO/EXTRACT_STATIC         = YES/' docs/Doxyfile
    sed -i 's/INPUT                  =/INPUT                  = include src/' docs/Doxyfile
    sed -i 's/RECURSIVE              = NO/RECURSIVE              = YES/' docs/Doxyfile
    sed -i 's/GENERATE_HTML          = NO/GENERATE_HTML          = YES/' docs/Doxyfile
    sed -i 's/GENERATE_LATEX         = YES/GENERATE_LATEX         = NO/' docs/Doxyfile
    sed -i 's/USE_MDFILE_AS_MAINPAGE =/USE_MDFILE_AS_MAINPAGE = README.md/' docs/Doxyfile
fi

# Run Doxygen
echo "Generating documentation..."
doxygen docs/Doxyfile

echo "Documentation generated successfully in docs/doxygen/html/" 