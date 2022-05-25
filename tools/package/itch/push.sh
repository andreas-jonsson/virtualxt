#!/bin/bash

cd "${GITHUB_WORKSPACE}/package"
butler validate virtualxt
butler login
butler push virtualxt phix/virtualxt:${RUNNER_OS}-${GITHUB_REF_NAME} --userversion ${VXT_VERSION}-${GITHUB_RUN_ID}
cd ..