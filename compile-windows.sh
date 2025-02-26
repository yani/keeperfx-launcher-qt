#!/bin/bash

# Check if the Docker image exists
if [[ "$(docker images -q kfx-launcher-win64-compiler 2> /dev/null)" == "" ]]; then
  echo "Docker image kfx-launcher-win64-compiler does not exist. Building the image..."
  docker build -t kfx-launcher-win64-compiler -f Dockerfile.win64 .
else
  echo "Docker image 'kfx-launcher-win64-compiler' already exists. Skipping build."
fi

# rm -r "$(pwd)/release"

# Run the Docker container and compile the project
docker run --rm -v "$(pwd)":/project kfx-launcher-win64-compiler bash -c "
    cd /project &&
    x86_64-w64-mingw32.shared-cmake -Bbuild/mingw-win64 -H. -DWINDOWS=TRUE -Wno-dev &&
    x86_64-w64-mingw32.shared-cmake --build build/mingw-win64 --config Release --verbose &&
    mkdir -p /project/release/win64/ &&
    cp /project/build/mingw-win64/*.exe /project/release/win64/ &&
    cp /project/build/mingw-win64/*.dll /project/release/win64/ &&
    wget https://raw.githubusercontent.com/saidinesh5/mxedeployqt/refs/heads/master/mxedeployqt -O /opt/mxedeployqt &&
    chmod +x /opt/mxedeployqt &&
    /opt/mxedeployqt \
        --mxetarget x86_64-w64-mingw32.shared \
        --mxepath=/opt/mxe \
        --skiplibs=\"qsqlmysql.dll;qsqlodbc.dll;qsqlpsql.dll;qsqltds.dll\" \
        --additionallibs=\"libwinpthread-1.dll;icudt74.dll;icuin74.dll;icuuc74.dll\" \
        /project/release/win64/
    "

    # sed -i 's|^Prefix = .*|Prefix = /launcher-data|' /project/release/win64/qt.conf &&
    # mkdir -p /project/release/win64/launcher-data &&
    # mv /project/release/win64/*.dll /project/release/win64/launcher-data/




# TODO: download mxedeployqt in docker container


    # mkdir -p /project/release/win64 &&
    # cp /project/build/mingw-win64/keeperfx-launcher-qt.exe /project/release/win64/ &&
    

# --qtplugins=\"tls;iconengines;imageformats;platforminputcontexts;platforms;styles\" \

    # cp /project/build/mingw-win64/*.dll /project/release/win64/"




    # /opt/mxe/usr/x86_64-w64-mingw32.shared/bin/windeployqt --release /project/build/mingw-win64/keeperfx-launcher-qt.exe &&

# /opt/mxe/tools/copydlldeps.sh

# wget https://raw.githubusercontent.com/saidinesh5/mxedeployqt/refs/heads/master/mxedeployqt
# chmod +x mxedeployqt

# /opt/mxedeployqt --mxepath /opt/mxe --mxetarget x86_64-w64-mingw32.shared .
# maybe this works: /opt/mxedeployqt --mxepath /opt/mxe --mxetarget x86_64-w64-mingw32.shared /project/build/mingw-win64/keeperfx-launcher-qt.exe