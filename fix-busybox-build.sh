#!/bin/bash
# Fix BusyBox build by disabling TC (traffic control)
# CBQ structures are missing from kernel headers

if [ -f .config ]; then
    sed -i 's/^CONFIG_TC=y/# CONFIG_TC is not set/' .config
    echo "Disabled CONFIG_TC in .config"
else
    echo "Error: .config not found. Run this in the BusyBox source directory."
    exit 1
fi
