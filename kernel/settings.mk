# Libraries
LIBS= carpisLib commons pthread m

# Custom libraries' paths
SHARED_LIBPATHS=
STATIC_LIBPATHS= ../carpisLib

# Compiler flags
CDEBUG=-g -Wall -DDEBUG
CRELEASE=-O3 -Wall -DNDEBUG

# Arguments when executing with start, memcheck or helgrind
ARGS=kernel_integral.config

# Valgrind flags
MEMCHECK_FLAGS=--track-origins=yes --log-file="memcheck.log"
HELGRIND_FLAGS=--log-file="helgrind.log"
