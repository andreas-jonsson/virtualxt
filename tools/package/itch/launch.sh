#!/bin/bash

if [ "$(uname)" == "Darwin" ]; then
    HERE="$(cd "$(dirname "$0")" && pwd -P)"
    open -a VirtualXT.app --args -a "${HERE}/freedos.img"
else
    HERE="$(dirname "$(readlink -f "${0}")")"
    export LD_LIBRARY_PATH=${HERE}/lib:$LD_LIBRARY_PATH
    exec "${HERE}/bin/virtualxt" "$@"
fi