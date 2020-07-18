#!/bin/bash

if [ "$1" != "" ]; then
    pacman -S $1 --noconfirm
else
    echo "No arguments were provided!"
fi
