#!/bin/bash

bochs -q &>/dev/null &
gdb -tui
