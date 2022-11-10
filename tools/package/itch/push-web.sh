#!/bin/bash

butler login
butler push virtualxt.zip phix/virtualxt:${GITHUB_REF_NAME} --userversion ${VXT_VERSION}-${GITHUB_RUN_ID}
