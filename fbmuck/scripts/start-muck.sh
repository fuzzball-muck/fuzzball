#! /bin/bash

# /usr/bin/start-muck
#
# Global startup manager for fbmuck.
#
# This script references dir/config.
# This script is started with the full qualified path to dir
#      as the sole argument.
#
#
# What this script is supposed to do:
#
# load dir/config.
# run a heavilly-modified version of the old 'restart' script.
#
# Stuff that's being moved into dir/config:
#       port #s
#       name of DB file.