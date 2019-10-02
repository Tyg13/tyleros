#!/bin/bash

bochs -q -log run.log &>/dev/null &
gdb -tui
