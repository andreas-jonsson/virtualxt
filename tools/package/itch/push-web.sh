#!/bin/bash

butler validate virtualxt.zip
butler login
butler push virtualxt.zip phix/virtualxt-web:${RUNNER_OS}-${GITHUB_REF_NAME} --userversion ${VXT_VERSION}-${GITHUB_RUN_ID}
