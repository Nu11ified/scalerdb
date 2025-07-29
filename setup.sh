#!/bin/bash

# ScalerDB Setup Script
echo "Setting up ScalerDB development environment..."

# Check if cmake is installed
if ! command -v cmake &> /dev/null; then
    echo "Error: CMake is not installed. Please install CMake version 3.20 or higher."
    exit 1
fi

# Check CMake version
CMAKE_VERSION=$(cmake --version | head -n1 | cut -d" " -f3)
CMAKE_MAJOR=$(echo $CMAKE_VERSION | cut -d. -f1)
CMAKE_MINOR=$(echo $CMAKE_VERSION | cut -d. -f2)

if [ "$CMAKE_MAJOR" -lt 3 ] || ([ "$CMAKE_MAJOR" -eq 3 ] && [ "$CMAKE_MINOR" -lt 20 ]); then
    echo "Error: CMake version 3.20 or higher is required. Found version $CMAKE_VERSION"
    exit 1
fi

# Check if compiler supports C++23
echo "Checking C++23 compiler support..."

# Create a temporary C++23 test file
cat > temp_cpp23_test.cpp << 'EOF'
#include <expected>
#include <iostream>

int main() {
    std::expected<int, std::string> result = 42;
    std::cout << "C++23 support verified\n";
    return 0;
}
EOF

# Test C++23 compilation
if g++ -std=c++23 temp_cpp23_test.cpp -o temp_cpp23_test 2>/dev/null; then
    echo "✓ C++23 support detected with g++"
    CXX_COMPILER="g++"
elif clang++ -std=c++23 temp_cpp23_test.cpp -o temp_cpp23_test 2>/dev/null; then
    echo "✓ C++23 support detected with clang++"
    CXX_COMPILER="clang++"
else
    echo "⚠  Warning: C++23 support may be limited. Continuing anyway..."
    CXX_COMPILER=""
fi

# Clean up test files
rm -f temp_cpp23_test.cpp temp_cpp23_test

# Create build directory
echo "Creating build directory..."
mkdir -p build

# Set up git if not already initialized
if [ ! -d ".git" ]; then
    echo "Initializing git repository..."
    git init
    
    # Create .gitignore
    cat > .gitignore << 'EOF'
# Build directories
build/
_build/

# IDE files
.vscode/
.idea/
*.swp
*.swo

# OS files
.DS_Store
Thumbs.db

# Dependencies (will be fetched by CMake)
_deps/

# Compiled binaries
*.exe
*.out
scalerdb
test_setup

# CMake cache
CMakeCache.txt
EOF
    
    echo "✓ Git repository initialized with .gitignore"
fi

echo "✓ Setup completed successfully!"
echo ""
echo "Next steps:"
echo "1. Run './make.sh' to build the project"
echo "2. Run './make.sh test' to build and run tests"
echo "3. Run './build/scalerdb' to execute the main program" 