#!/bin/bash

cd "${TRAVIS_BUILD_DIR}/package"
butler validate virtualxt
butler login
butler push virtualxt phix/virtualxt:${TRAVIS_OS_NAME}-${TRAVIS_BRANCH} --userversion ${VXT_VERSION}.${TRAVIS_BUILD_ID}
cd ..