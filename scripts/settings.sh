#!/bin/bash

#####################################
# A file for user-editable settings #
#####################################

# Assume the server and client will run on single target: PI_SERVER_HOST
# It ensures both server and client certificates are deployed on server.
export SINGLE_TARGET=false

# Target IPs
export PI_SERVER_HOST=192.168.x.x
export PI_CLIENT_HOST=192.168.x.x

# A path to CROSSCON demo repositories.
# https://github.com/3mdeb/CROSSCON-Hypervisor-and-TEE-Isolation-Demos/
# Used to sync contents of this repository with buildroot in demo repo.
export CROSSCON_REPO_PATH=<path_to_crosscon_demos>
