#!/bin/bash

# Copyright (c) 2020 - 2021 by Apex.AI Inc. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-License-Identifier: Apache-2.0

# Necessary tools:
# mkdocs-awesome-pages-plugin, v2.6.0
# mkdocs-autolinks-plugin, v0.4.0 (with svg patch)
# mkdocs-material, v8.1.3-insiders-4.5.0
# markdown-include, v0.6.0
# mkdocs, v1.2.3
# Doxygen, v1.9.1
# doxybook2, v1.3.3
# mike, v1.0.0

set -e

COLOR_OFF='\033[0m'
COLOR_RED='\033[1;31m'
COLOR_GREEN='\033[1;32m'
COLOR_YELLOW='\033[1;33m'

WORKSPACE=$(git rev-parse --show-toplevel)
BASENAME=$(basename "$WORKSPACE")
WEBREPO="git@github.com:eclipse-iceoryx/iceoryx-web.git"

DO_CLEANUP=false
BRANCH=""
HAS_BRANCH=false
VERSION=""
DO_PREPARE_PUBLISH=false
DO_SERVE=false
NO_DOXYGEN=false

if [[ "$BASENAME" -ne "iceoryx" ]]; then
    echo "Git folder must be named iceoryx!"
    exit 1
fi

CMAKE_CXX_FLAGS=""

while (( "$#" )); do
  case "$1" in
    -b|--branch)
        BRANCH=$2
        HAS_BRANCH=true
        shift 2
        ;;
    "clean")
        DO_CLEANUP=true
        shift 1
        ;;
    "no-doxygen")
        NO_DOXYGEN=true
        shift 1
        ;;
    "--prepare-publish")
        VERSION=$2
        DO_PREPARE_PUBLISH=true
        shift 2
        ;;
    "serve")
        DO_SERVE=true
        shift 1
        ;;
    "help")
        echo "Website generator script for iceoryx."
        echo ""
        echo "Usage:"
        echo "    website-generator.sh serve [--branch <b>]"
        echo "Options:"
        echo "    -b --branch           Specify the branch to use for the website generation"
        echo "    --prepare-publish     Specify the version to prepare to publish and commit"
        echo "                          the generated website to the website repo but do not publish"
        echo "Args:"
        echo "    clean                 Delete the 'build' and 'build_website' directories before"
        echo "                          generating the website"
        echo "    help                  Print this help"
        echo "    no-doxygen            Do not generate doxygen documentation"
        echo "    serve                 Generate the website and serve it on 'localhost'"
        echo ""
        echo "e.g. website-generator.sh clean serve no-doxygen"
        exit 0
        ;;
    *)
        echo "Invalid argument '$1'. Try 'help' for options."
        exit 1
        ;;
  esac
done

cd "$WORKSPACE"

# clean build folders
if [[ $DO_CLEANUP == true ]]; then
    cd "$WORKSPACE"
    echo " [i] Cleaning build directories"
    if [[ -d "$WORKSPACE/build" ]]; then
        rm -rf "$WORKSPACE/build/"*
    fi
    if [[ -d "$WORKSPACE/build_website" ]]; then
        rm -rf "$WORKSPACE/build_website/"*
    fi
fi

if [[ $HAS_BRANCH == true ]]; then
    git checkout "$BRANCH"
fi

# copy relevant files to build_website folder for further adjustments like fixing links
mkdir -p "$WORKSPACE"/build_website
cp -R "$WORKSPACE"/doc/website "$WORKSPACE"/build_website/
cp -R "$WORKSPACE"/iceoryx_examples "$WORKSPACE"/build_website/

mkdir -p "$WORKSPACE"/build_website/website/API-reference/hoofs
cp "$WORKSPACE"/iceoryx_hoofs/README.md "$WORKSPACE"/build_website/website/API-reference/hoofs
mkdir -p "$WORKSPACE"/build_website/website/API-reference/c-binding
cp "$WORKSPACE"/iceoryx_binding_c/README.md "$WORKSPACE"/build_website/website/API-reference/c-binding

# fix some links (print0 is used to avoid issues with files containing spaces in their name)
DOWN="\.\.\/"
sed -i "s/${DOWN}iceoryx_examples/${DOWN}${DOWN}examples/g" "$WORKSPACE"/build_website/website/API-reference/c-binding/README.md
sed -i 's/\.\.\/doc\/design/https\:\/\/github\.com\/eclipse-iceoryx\/iceoryx\/tree\/main\/doc\/design/g' "$WORKSPACE"/build_website/website/API-reference/hoofs/README.md
find "$WORKSPACE"/build_website/website -type f -name "*.md" -print0 | xargs -0 sed -i 's/\.\.\/\.\.\/iceoryx_examples/examples/g'
find "$WORKSPACE"/build_website/website -type f -name "*.md" -print0 | xargs -0 sed -i 's/\.\.\/iceoryx_examples/examples/g'

if [[ $NO_DOXYGEN == false ]]; then
    echo " [i] Generating doxygen"

    cmake -Bbuild -Hiceoryx_meta -DBUILD_DOC=ON
    cd "$WORKSPACE"/build
    make -j8 doxygen_iceoryx_hoofs doxygen_iceoryx_posh doxygen_iceoryx_binding_c

    cd "$WORKSPACE"

    PACKAGES="hoofs posh c-binding"

    # Generate doxybook2 config files, to have correct links in doxygen docu
    mkdir -p "$WORKSPACE"/build_website/generated/

    for PACKAGE in ${PACKAGES}  ; do
        FILE=$WORKSPACE/build_website/generated/doxybook2-$PACKAGE.json
        rm -f "$FILE"
        echo "{" >> "$FILE"
        if [[ $DO_SERVE == true ]]; then
            echo "\"baseUrl\": \"/API-reference/$PACKAGE/\","  >> "$FILE"
        elif [[ $DO_PREPARE_PUBLISH ]]; then
            echo "\"baseUrl\": \"/$VERSION/API-reference/$PACKAGE/\","  >> "$FILE"
        fi
        echo "\"indexInFolders\": false,"  >> "$FILE"
        echo "\"linkSuffix\": \"/\","  >> "$FILE"
        echo "\"mainPageInRoot\": false"  >> "$FILE"
        echo "}"  >> "$FILE"
    done

    BUILD_FOLDER="$WORKSPACE"/build/doc
    mkdir -p "$BUILD_FOLDER"
    DOC_FOLDER="$WORKSPACE"/build_website/website/API-reference
    mkdir -p "$DOC_FOLDER"
    CONFIG_FOLDER="$WORKSPACE"/build_website/generated/

    echo " [i] Generating markdown from doxygen"
    mkdir -p "$DOC_FOLDER"/hoofs
    doxybook2 --input $BUILD_FOLDER/iceoryx_hoofs/xml/ --output $DOC_FOLDER/hoofs --config $CONFIG_FOLDER/doxybook2-hoofs.json

    mkdir -p "$DOC_FOLDER"/posh
    doxybook2 --input $BUILD_FOLDER/iceoryx_posh/xml/ --output $DOC_FOLDER/posh --config $CONFIG_FOLDER/doxybook2-posh.json

    mkdir -p "$DOC_FOLDER"/c-binding
    doxybook2 --input $BUILD_FOLDER/iceoryx_binding_c/xml/ --output $DOC_FOLDER/c-binding --config $CONFIG_FOLDER/doxybook2-c-binding.json

    # Remove index files

    FILES="index_classes.md index_examples.md index_files.md index_modules.md index_namespaces.md index_pages.md index_groups.md"

    for PACKAGE in ${PACKAGES}  ; do
        for FILE in ${FILES}  ; do
            rm -f "$DOC_FOLDER"/"$PACKAGE"/"$FILE"
        done
    done
fi

if [[ $DO_SERVE == true ]]; then
    echo " [i] starting local webserver"
    mkdocs serve --config-file ../iceoryx/mkdocs.yml
fi

if [[ $DO_PREPARE_PUBLISH == true ]]; then
    # Generate HTML and push to GitHub pages
    if [ ! -d "$WORKSPACE"/build_website/git/iceoryx-web ]; then
        mkdir -p "$WORKSPACE"/build_website/git
        cd "$WORKSPACE"/build_website/git
        git clone $WEBREPO
    fi
    cd "$WORKSPACE"/build_website/git/iceoryx-web
    mike deploy --branch main --config-file "$WORKSPACE"/mkdocs.yml --update-aliases "$VERSION" latest
    echo "you need to push the changes to upstream"
fi
