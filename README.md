# BusyBox Build with TC Disabled

This repository contains a BusyBox build configuration with TC (traffic control) disabled due to missing CBQ kernel headers.

## Fix Applied

Disabled `CONFIG_TC` to resolve build errors related to missing CBQ structures in modern kernel headers.
