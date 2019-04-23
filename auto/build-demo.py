#!/usr/bin/python3
#
# This python script uses the Fuzzball python library to initialize the demo
# MUCK database assuming a fresh advancedb install.
#
# This specifically targets muck.fuzzball.org at this time and is a way
# for FB developers to add to the spinup process.

from pyfuzzball.base import FuzzballBase

# Load password from a file
pw = None

with open('pw-file.txt', 'r') as i:
    pw = i.readline().strip()

conn = FuzzballBase("muck.fuzzball.org", 4202, True, True)
conn.login("One", "potrzebie")
conn.write("@password potrzebie=%s\r\n" % pw)
conn.write("@tune autolink_actions=no\r\n")
conn.write("@tune registration=no\r\n")
conn.write("lsedit #0=welcome\r\n")
conn.write(
    ".del 1 100\r\n"
    "           ,...                          ,,                   ,,    ,,\r\n"
    "         .d' \"\"                         *MM                 `7MM  `7MM\r\n"
    "         dM`                             MM                   MM    MM  \r\n"
    "        mMMmm`7MM  `7MM  M\"\"\"MMV M\"\"\"MMV MM,dMMb.   ,6\"Yb.    MM    MM\r\n"
    "         MM    MM    MM  '  AMV  '  AMV  MM    `Mb 8)   MM    MM    MM\r\n"
    "         MM    MM    MM    AMV     AMV   MM     M8  ,pm9MM    MM    MM\r\n"
    "         MM    MM    MM   AMV  ,  AMV  , MM.   ,M9 8M   MM    MM    MM\r\n"
    "       .JMML.  `Mbod\"YML.AMMmmmM AMMmmmM P^YbmdP'  `Moo9^Yo..JMML..JMML.\r\n"
    " \r\n"
    "             `7MMM.     ,MMF'`7MMF'   `7MF' .g8\"\"\"bgd `7MMF' `YMM'\r\n"
    "               MMMb    dPMM    MM       M .dP'     `M   MM   .M'\r\n"
    "               M YM   ,M MM    MM       M dM'       `   MM .d\"\r\n"
    "               M  Mb  M' MM    MM       M MM            MMMMM.\r\n"
    "               M  YM.P'  MM    MM       M MM.           MM  VMA\r\n"
    "               M  `YM'   MM    YM.     ,M `Mb.     ,'   MM   `MM.\r\n"
    "             .JML. `'  .JMML.   `bmmmmd\"'   `\"bmmmd'  .JMML.   MMb.\r\n"
    " \r\n"
    "                               Development Port\r\n"
    " \r\n"
    "To connect to your character use 'connect <playername> <password>'\r\n"
    "To create a new character use 'create <playername> <password>'\r\n"
    " \r\n"
    "'WHO' and 'help' also work in this context.\r\n"
    ".end\r\n"
)
conn.write("@chown #0=keeper\r\n")
conn.write("@pcreate Tanabi=%s\r\n" % pw)
conn.write("@set *Tanabi=W\r\n")
conn.quit()
