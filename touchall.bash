#!/bin/bash

options=""

if [ "$1" != "" ]; then
	options="${options} -d \"${1}\""
fi

find . -exec touch ${options} {} \;
