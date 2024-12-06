#!/bin/bash

set -e

CUR_DIR="$(pwd)"
BASHSRC="$HOME/.bashrc"

if ! grep -Fq $CUR_DIR <<< $PATH; then
	echo "Adding $CUR_DIR to PATH in $BASHSRC"
	echo "export PATH=\"$CUR_DIR:$PATH\"" >> "$BASHSRC"
	echo "Don't forget to source the new bash config"
else 
	echo "$CUR_DIR already in PATH"
fi

if [ -f Makefile ]; then
	echo "Running make"
	make
	echo "Build complete"
else 
	echo "No Makefile found in $CUR_DIR"
	exit 1
fi

echo "Installation complete."