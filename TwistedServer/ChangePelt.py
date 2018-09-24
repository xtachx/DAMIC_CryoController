#!/usr/bin/env python


import GeyserProto as GP
from optparse import OptionParser



#print GP.getTemperature(0)

if __name__ == '__main__':
    parser = OptionParser()
    parser.add_option("-n", "--peltnum", dest="npelt",
                  help="Peltier system number", metavar="peltnum")
    parser.add_option("-t", "--settemp", dest="stemp",
                  help="temperature", metavar="settemp")
    
    (options, args) = parser.parse_args()
    print "Changing: Peltier system "+options.npelt+" set temperature to "+options.stemp
    
    GP.ChangePelt1(int(options.npelt), float(options.stemp))