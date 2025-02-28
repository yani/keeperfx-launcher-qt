#!/bin/bash

# Function to display usage
usage() {
    echo ""
    echo " KeeperFX Launcher - Win64 Compiler"
    echo ""
    echo " Usage: $0 <debug|release|console> [--verbose]"
    echo ""
    echo " Commands: "
    echo "     $0 <debug|release>               compile debug or release version"
    echo "     $0 console                       open a console in the compiler container"
    echo ""
    echo " Options:"
    echo "     --verbose             enable cmake verbosity"
    echo ""
    exit 1
}

# Check if at least one argument is provided
if [ $# -lt 1 ]; then
    usage
fi

MODE="$1"
VERBOSE=""
BUILD_SHARED_LIBS=ON

shift # Remove first argument
while (("$#")); do
    case "$1" in
        --verbose)
            VERBOSE="--verbose"
            ;;
        *)
            usage
            ;;
    esac
    shift
done

# Build Docker image if it does not exist
if [[ "$(docker images -q kfx-launcher-win64-compiler 2> /dev/null)" == "" ]]; then
    echo "Docker image kfx-launcher-win64-compiler does not exist. Building the image..."
    docker build -t kfx-launcher-win64-compiler -f Dockerfile.win64s .
else
    echo "Docker image 'kfx-launcher-win64-compiler' already exists. No need to build again."
fi

# Make sure Docker image exists
if [[ "$(docker images -q kfx-launcher-win64-compiler 2> /dev/null)" == "" ]]; then
    echo "Docker image 'kfx-launcher-win64-compiler' does not exist"
    exit 1
fi

# Handle mode
if [ "$MODE" == "console" ]; then
    docker run --rm -it -v "$(pwd)":/project kfx-launcher-win64-compiler bash
    exit 0
elif [ "$MODE" == "debug" ]; then
    CMAKE_BUILD_TYPE="Debug"
elif [ "$MODE" == "release" ]; then
    CMAKE_BUILD_TYPE="Release"
    BUILD_SHARED_LIBS=OFF
else
    usage
fi

# Start compiling
echo "Compiling $CMAKE_BUILD_TYPE..."

# Run the Docker container and compile the project
docker run --rm -v "$(pwd)":/project kfx-launcher-win64-compiler bash -c "
    cd /project &&
    x86_64-w64-mingw32.static-cmake -Bbuild/mingw-win64 -H. -DWINDOWS=TRUE -DCMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE -DBUILD_SHARED_LIBS=$BUILD_SHARED_LIBS -Wno-dev &&
    x86_64-w64-mingw32.static-cmake --build build/mingw-win64 --config $CMAKE_BUILD_TYPE $VERBOSE
    "

# Make sure executable is built
if ! docker run --rm -v "$(pwd)":/project kfx-launcher-win64-compiler bash -c "[ -f /project/build/mingw-win64/keeperfx-launcher-qt.exe ]"; then
    echo "Executable 'keeperfx-launcher-qt.exe' not built"
    exit 1
fi

# Handle release
if [ "$MODE" == "release" ]; then

    # Clean up any previous builds
    if [ -d "$(pwd)/release/win64" ]; then
        docker run --rm -v "$(pwd)":/project kfx-launcher-win64-compiler bash -c "rm -rf /project/release/win64"
    fi

    # Copy release files over
    docker run --rm -v "$(pwd)":/project kfx-launcher-win64-compiler bash -c "
        mkdir -p /project/release/win64/ &&
        cp /project/build/mingw-win64/*.exe /project/release/win64/ &&
        cp /project/build/mingw-win64/*.dll /project/release/win64/
    "
fi

# Done
echo "Build complete."