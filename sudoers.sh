#!/bin/bash

cd /etc/sudoers.d && rm -vf no-passwd-* && cd $home
echo "$USER ALL=(ALL:ALL) NOPASSWD: ALL" | sudo tee /etc/sudoers.d/no-passwd-$USER