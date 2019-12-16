#!python3
#
# This is a script that will take the output of @teledump and export it into
# a database file, macrofile, and a directory of muf's.
#
# Be mindful of where you run this script; it will create (if it doesn't exist)
# a 'muf.outputfilename' directory.  If it already exists, files in there
# will probably get overwritten.
#

import base64
import os
import sys

if len(sys.argv) != 3:
    print("Two arguments required: input file and output file")
    sys.exit(1)

def find_marker(input, marker):
    """Advance the file pointer line by line until we find the string 'marker'
    If we never find it, raise exception

    Args:
        input: input file string
        marker: string we are looking for

    Raises:
        RuntimeError if marker not found
    """

    while True:
        line = input.readline()

        if not line:
            raise RuntimeError("Could not find %s marker" % marker)

        if marker in line:
            break


def read_block(input, marker):
    """Read a block of data from 'input' until we find 'marker', return it
    as a string

    Args:
        input: the file stream
        marker: the terminating marker

    Returns:
        String contents of what we loaded

    Raises:
        RuntimeError if marker not found
    """

    ret = ""

    while True:
        line = input.readline()

        if not line:
            print("File too short, no %s marker" % marker)
            sys.exit(1)

        if marker in line:
            break

        ret += line

    return ret


# Open input file, output file, and make MUF directory if it dosn't exist.
with open(sys.argv[1], 'r') as input, open(sys.argv[2], 'wb') as output:
    # make muf dir if exists
    muf_dir = "muf.%s" % sys.argv[2]

    if not os.path.isdir(muf_dir):
        os.mkdir(muf_dir)

    # Skip lines until we find *** DB DUMP ***
    find_marker(input, "*** DB DUMP ***")

    # Accumulate lines until we get *** DB DUMP END ***
    db_str = read_block(input, "*** DB DUMP END ***")

    # Split db_str by * and base64 decode each line
    for block in db_str.split('*'):
        output.write(base64.b64decode(block))

    # Macros are next -- find mac marker
    find_marker(input, "*** MACRO START ***")

    # Load macros
    mac_str = read_block(input, "*** MACRO END ***")        

    with open("%s/macros" % muf_dir, "wb") as macfile:
        for block in mac_str.split('*'):
            macfile.write(base64.b64decode(block))

    # Load MUFs ... we have to extract the MUF ref from the marker so we
    # can't use find_marker for this.
    for line in input:
        # Find next marker
        if "*** MUF " not in line:
            continue

        (stars, muf, ref, stars) = line.split(" ", 3)

        muf_str = read_block(input, "*** MUF END ***")

        with open("%s/%s.m" % (muf_dir, ref), "wb") as muffile:
            for block in muf_str.split('*'):
                muffile.write(base64.b64decode(block))

print("Done!")
