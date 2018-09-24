#!/usr/bin/env python


import GeyserProto as GP
from optparse import OptionParser
import time
import csv

import matplotlib.pyplot as plt
import numpy as np

ofile  = open('PresRecord1.csv', "wb")
DataWriter = csv.writer(ofile)

fig=plt.figure()
plt.ylim([-3,40])
    
x_var=[]
y_var=[]
plt.ion()
plt.show()


def MakeLivePlot(Y, x_start):
    x_var.append(x_start)
    y_var.append(Y)
    plt.scatter(x_var, y_var)
    #plt.set_ydata(Y_global)
    plt.xlim(len(y_var)-100,len(y_var))
    #plt.ylim(min(y_var),max(y_var))
    plt.draw()

    
def runPlotter():
    x_start = 0
    while True:
        Temp = GP.getPressure()
        MakeLivePlot(Temp, x_start)
        x_start += 1
        DataWriter.writerow([Temp])
        time.sleep(1)
#print GP.getTemperature(0)
    




if __name__ == '__main__':
    #parser = OptionParser()
    #parser.add_option("-n", "--peltnum", dest="npelt",
    #              help="Peltier system number", metavar="peltnum")
    #
    #
    #(options, args) = parser.parse_args()
    #
    #print GP.getTemperature(int(options.npelt))
    runPlotter()
