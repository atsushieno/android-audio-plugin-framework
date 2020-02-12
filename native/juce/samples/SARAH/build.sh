#!/bin/sh

CURDIR="$( cd `dirname $0` >/dev/null 2>&1 && pwd )"

ANDROID_SDK_OVERRIDE=$HOME/Android/Sdk

echo "Entering $CURDIR ..."
cd $CURDIR

$JUCE_DIR/Projucer --resave SARAH.jucer

# There is no way to generate those files in Projucer.
cp project-override.build.gradle Builds/Android/build.gradle
cp ../../copy-into-generated-project.gradle.properties Builds/Android/gradle.properties

sed -e "s/project (\"SARAH\" C CXX)/project (\"SARAH\" C CXX)\n\nlink_directories (\n  \$\{CMAKE_CURRENT_SOURCE_DIR\}\/..\/..\/..\/..\/..\/..\/build\/native\/androidaudioplugin \n  \$\{CMAKE_CURRENT_SOURCE_DIR\}\/..\/..\/..\/..\/..\/..\/build\/native\/androidaudioplugin-lv2) \n\n/" Builds/CLion/CMakeLists.txt  > Builds/CLion/CMakeLists.txt.patched
mv Builds/CLion/CMakeLists.txt.patched Builds/CLion/CMakeLists.txt

# This does not work on bitrise (Ubuntu 16.04) because libfreetype6-dev is somehow too old and lacks some features that is uncaught. Disabled.
# cd Builds/LinuxMakefile && make && cd ../..

echo "ndk.dir=$ANDROID_SDK_OVERRIDE/ndk/20.0.5594570\nsdk.dir=$ANDROID_SDK_OVERRIDE" > Builds/Android/local.properties

cd Builds/Android && ./gradlew build && cd ../..

