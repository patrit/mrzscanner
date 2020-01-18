#!/bin/bash

set -e 

cd $(dirname $0)
[ -e opencv ] || git clone https://github.com/opencv/opencv.git
cd opencv
git checkout 4.2.0
git checkout -- platforms/js/opencv_js.config.py
pwd
../update_opencv_js_config.py $(find ../../static/ -name "*.js")
#git diff -- platforms/js/opencv_js.config.py
sudo docker run --rm --workdir /code -v "$PWD":/code "trzeci/emscripten:latest" \
  python ./platforms/js/build_js.py build --build_wasm --cmake_option="-DBUILD_LIST=core,imgproc,js" --clean_build_dir
cp build/bin/opencv.js ../../static/
