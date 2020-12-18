__author__="tan"
__date__ ="$Jul 05, 2009 9:38:04 AM$"
import os
if __name__ == "__main__":
    print ("Download");
from optparse import OptionParser
parser = OptionParser()
parser.add_option("-f", "--file", dest="file")
(options, args) = parser.parse_args()
if len(args) < 0:
    parser.error("We need a download list!")
# reading contents
file = open(options.file, "r")
try:
    for line in file:
        line = line.rstrip('\n')
        #now download link
        h = os.popen('wget ' + line)
        h.close()
finally:
    file.close()
