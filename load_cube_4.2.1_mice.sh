
CUBE=/opt/cube/4.2.1

export CUBE_LIBRARY_PATH="$CUBE/lib"
export CUBE_INCLUDE_PATH="$CUBE/include/cube"

export PATH="$CUBE/bin:$PATH"

export LD_LIBRARY_PATH="$CUBE_LIBRARY_PATH:$LD_LIBRARY_PATH"

