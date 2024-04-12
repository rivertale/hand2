#!/bin/sh
mkdir -p ../publish
cd ../publish

# usage
version="$1"
if [ -z "${version}" ]
then
    echo "usage: publish.sh version"
else
    abort_archive=0
    rm -r ./hand2 2>/dev/null
    rm -f "./hand2-v${version}.zip" 2>/dev/null
    mkdir -p hand2
    cp ../build/hand.exe hand2/
    if [ $? -ne 0 ]
    then
        abort_archive=1
        echo "Can't find ../build/hand.exe"
    fi
    cp ../build/libcurl-x64.dll hand2/
    if [ $? -ne 0 ]
    then
        abort_archive=1
        echo "Can't find ../build/libcurl-x64.dll"
    fi
    cp ../build/libgit2-x64.dll hand2/
    if [ $? -ne 0 ]
    then
        abort_archive=1
        echo "Can't find ../build/libgit2-x64.dll"
    fi
    cp ../build/hand hand2/
    if [ $? -ne 0 ]
    then
        abort_archive=1
        echo "Can't find ../build/hand"
    fi
    
    which zip 1>/dev/null 2>/dev/null
    if [ $? -eq 0 ]
    then
        zip -r "hand2-v${version}.zip" hand2
    else
        tar -cf "hand2-v${version}.zip" hand2
    fi
    
    if [ $? -eq 0 ]; then echo "Output file: hand2-v${version}.zip"; fi
    rm -r ./hand2 2>/dev/null
fi
