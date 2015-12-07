#!/usr/bin/env bash
#
# Copyright (c) 2015 nyuszika7h <nyuszika7h@openmailbox.org>
#
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
# CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

PATH="$PATH:/c/Program Files/7-Zip"

optimize_flags='-Ofast -s'

if [[ "$1" == '--debug' ]]; then
    debug='y'
    optimize_flags='-O0 -g'
    shift
fi

arch="$1"
date="$(date +'%Y%m%d')"
commit="$(git rev-parse --short HEAD)"

if [[ -z "$1" ]]; then
    echo 'Error: No arch specified.' >&2
    exit 1
fi

echo 'Starting build'

build_root="build/${arch}"
build_name="letvezi_win_${arch}_$(date +%Y%m%d)_$(git rev-parse --short HEAD)${debug:+_debug}"
build_dir="${build_root}/${build_name}"

printf 'Build directory: %s\n' "$build_dir"

if [[ -e "$build_dir" ]]; then
    printf 'Error: Build directory already exists.\n' "$build_dir" >&2
    exit 1
fi

mkdir -p "$build_dir"

# NB: The lack of quotes around $optimize_flags is intentional.
g++ -std=c++14 src/*.cpp -Wall -Wextra $optimize_flags -Isrc/ -Ilibs/ -I"../${arch}/" -I"../${arch}/boost_1_59_0/" -L"../${arch}/" -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer -lpthread -o "${build_dir}/letvezi.exe"

cp -r "../${arch}"/*.dll "$build_dir"
cp -r ../licenses "$build_dir"
cp -r art "$build_dir"
cp LICENSE "${build_dir}/LICENSE.txt"

if [[ -z "$debug" ]]; then
    # Don't create a zip in debug mode
    cd "$build_root"
    7z a "${build_name}.zip" "$build_name"
fi
