#!/bin/bash

# Check if the Docker image exists
if [[ "$(docker images -q kfx-launcher-win64-compiler 2> /dev/null)" == "" ]]; then
  echo "Docker image kfx-launcher-win64-compiler does not exist. Building the image..."
  docker build -t kfx-launcher-win64-compiler -f Dockerfile.win64 .
else
  echo "Docker image 'kfx-launcher-win64-compiler' already exists. Skipping build."
fi

# Run the Docker container and compile the project
docker run --rm -v "$(pwd)":/project kfx-launcher-win64-compiler bash -c "
    cd /project &&
    x86_64-w64-mingw32.shared-cmake -Bbuild-windows -H. -DWINDOWS=TRUE -Wno-dev &&
    x86_64-w64-mingw32.shared-cmake --build build-windows --config Release --verbose &&
    /opt/mxe/usr/x86_64-w64-mingw32.shared/bin/windeployqt --release /project/build-windows/keeperfx-launcher-qt.exe &&
    mkdir -p /project/release/win64 &&
    cp /project/build-windows/keeperfx-launcher-qt.exe /project/release/win64/ &&
    cp /project/build-windows/*.dll /project/release/win64/"






# wget https://raw.githubusercontent.com/saidinesh5/mxedeployqt/refs/heads/master/mxedeployqt
# chmod +x mxedeployqt


# /opt/mxedeployqt --mxepath /opt/mxe --mxetarget x86_64-w64-mingw32.shared .