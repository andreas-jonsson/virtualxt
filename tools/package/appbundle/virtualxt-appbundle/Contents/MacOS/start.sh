#!/bin/bash

osascript -e 'tell app "Terminal" to do script "\"'"$(cd "$(dirname "$0")" && pwd -P)/env.sh" "$@"'\""'