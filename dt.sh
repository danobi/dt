#!/bin/env bash

NEWDIRFILE=.newdir.dt

function dt() {
    ./dt
    if [[ ! -f $NEWDIRFILE ]]; then
        return 0
    fi
    prevfile=$(realpath $NEWDIRFILE)
    cd $(cat $NEWDIRFILE)
    rm "$prevfile"
}
