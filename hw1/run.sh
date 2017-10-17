#!/bin/bash
# path to VisIt must be changed
export VISIT=/opt/apps/visit12_12_0-wmesa-parallel/2.12.0/linux-x86_64/
export LD_LIBRARY_PATH=$VISIT/lib
export VISITPLUGINDIR=$VISIT/plugins
gdb ./perlin.out

