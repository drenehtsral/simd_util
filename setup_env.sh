#!/bin/bash

if [ "${0}" == "${BASH_SOURCE[0]}" ]; then
    echo "To set up your environment to use the simd_util toolkit you must source (rather than execute) ${0}." >&2
    exit 1
fi

GCC_PATH="${GCC_PATH:-/opt/gcc9/bin}"
BINUTILS_PATH="${BINUTILS_PATH:-/opt/binutils/bin}"
GDB_PATH="${GDB_PATH:-/opt/gdb9/bin}"
SDE_PATH="${SDE_PATH:-/opt/sde}"

PATH_COMPS=( "${GCC_PATH}" "${BINUTILS_PATH}" "${GDB_PATH}" "${SDE_PATH}" $(echo "${PATH}" | tr ":" "\n" | egrep -v "^${GCC_PATH}|${BINUTILS_PATH}|${GDB_PATH}|${SDE_PATH}\$"))

export PATH="$(echo "${PATH_COMPS[@]}" | tr " " ":")"

fail=0

found_gcc="$(dirname $(which gcc))"
found_binutils="$(dirname $(which objdump))"
found_gdb="$(dirname $(which gdb))"
found_sde="$(dirname $(which sde64))"

if [ "${found_gcc}" != "${GCC_PATH}" ] ; then
    if [ -z "${found_gcc}" ]; then
        echo "found no gcc at all" >&2
    else
        echo "gcc found at ${found_gcc} NOT ${GCC_PATH}" >&2
    fi
    fail=1
fi

if [ "${found_binutils}" != "${BINUTILS_PATH}" ] ; then
    if [ -z "${found_binutils}" ]; then
        echo "found no binutils at all" >&2
    else
        echo "binutils found at ${found_binutils} NOT ${BINUTILS_PATH}" >&2
    fi
    fail=1
fi

if [ "${found_gdb}" != "${GDB_PATH}" ] ; then
    if [ -z "${found_gdb}" ]; then
        echo "found no gdb at all" >&2
    else
        echo "gdb found at ${found_gdb} NOT ${GDB_PATH}" >&2
    fi
    fail=1
fi

if [ "${found_sde}" != "${SDE_PATH}" ] ; then
    if [ -z "${found_sde}" ]; then
        echo "found no sde at all" >&2
    else
        echo "sde found at ${found_sde} NOT ${SDE_PATH}" >&2
    fi
    fail=1
fi

echo "gcc      version: $(gcc --version | head -n1)"
echo "binutils version: $(objdump --version | head -n1)"
echo "gdb      version: $(gdb --version | head -n1)"
echo "sde64    version: $(sde64 --version | head -n1)"

if [ ${fail} -gt 0 ]; then
    echo -e "\nEnvironment setup ERROR.\n"
    /bin/false
else
    echo -e "\nEnvironment setup OK.\n"
    /bin/true
fi
