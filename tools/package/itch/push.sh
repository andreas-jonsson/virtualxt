#!/bin/bash

if [ "$PUSH_SNAP" = "1" ]; then
    if [ "$TRAVIS_BRANCH" = "release" ]; then
        snapcraft push *.snap --release beta
    else
        snapcraft push *.snap --release edge
    fi
else
    cd "${TRAVIS_BUILD_DIR}/package"
    butler validate virtualxt
    butler login
    butler push virtualxt phix/virtualxt:${TRAVIS_OS_NAME}-${TRAVIS_BRANCH} --userversion ${VXT_VERSION}.${TRAVIS_BUILD_ID}
    cd ..
fi