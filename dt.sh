#!/bin/env bash

function dt() {
    ./dt
    prevfile=$(realpath .newdir.dt)
    cd $(cat .newdir.dt)
    rm "$prevfile"
}
