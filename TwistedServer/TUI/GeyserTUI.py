#!/usr/bin/env python

#################################################################
#This is the TUI interface for the geyser network operations    #
#control system, AutoGeyser v3. The TUI is based on urwid and   #
#the formatting is based on TextTable from pip                  #
#                                                               #
#Made by Pitam Mitra for PICO dark matter search                #
#                                                               #
#                                                               #
#Edit History:                                                  #
#    1. - Friday Nov 15, 2013 : Created the script              #
#                                                               #
#################################################################

import texttable as tt
import urwid, time


class AutoGetserTUI():
    
    def __init__(self):
        #Instances of the tables to draw the information
        self.tab_temperature = tt.Texttable()
        self.tab_runParams = tt.Texttable()
        
        #Variables to store the data to be displayed in tabs
        self.tTemp = [[]] # The empty row will have the header
        self.tRunPars = [[]] # The empty row will have the header
        self.isActiveRun = False
        self.displayData = "AutoGeyser - Geyser Control Operations\n\n"
        
        #Variables to store the process data
        self.activePressure = 0.0
        self.AllTemps = [0.0]*6
        self.AllSetTemps = [0.0]*3
        
        #Draw the container in urwid
        self.txt = urwid.Text(self.displayData, align='left')
        self.fill = urwid.Filler(self.txt, 'top')
        #self.loop = urwid.MainLoop(self.fill, unhandled_input=self.exit_on_q, event_loop=urwid.TwistedEventLoop())
        self.loop = urwid.MainLoop(self.fill, unhandled_input=self.exit_on_q)
    
    def start(self):
        #self.loop.set_alarm_in(2, self.updateDisplay)
        self.loop.run()
        
        
    def exit_on_q(self, key):
        if key in ('q', 'Q'):
            raise urwid.ExitMainLoop()
        if key in ('r', 'R'):
            self.loop.draw_screen()
        
        
    #Code to generate the display updates
    def UpdDisplay(self):
        
        #Generate the TC table
        #desc--set--right---left
        self.tTemp.append(["Top",self.AllSetTemps[0],self.AllTemps[0],self.AllTemps[1]])
        self.tTemp.append(["Mid",self.AllSetTemps[1],self.AllTemps[2],self.AllTemps[3]])
        self.tTemp.append(["Bot",self.AllSetTemps[2],self.AllTemps[4],self.AllTemps[5]])
        #generate the table string
        self.tab_temperature.add_rows(rows)
        #alignment of the columns
        self.tab_temperature.set_cols_align(['c','r','r','r'])
        #Headers
        self.tab_temperature.header(['Temp (C)','Set (C)','Right', 'Left'])
        #add to the list of data to be displayed
        self.displayData += self.tab_temperature.draw()
        ################
        self.displayData += "\n\nGeyser Run Parameters:\n\n"
        ################
        #Code for the pressure, run status and other parts
        if self.activePressure >=20.0:
            self.tRunPars.append([get_color_string(bcolors.RED,"Pressure "),get_color_string(bcolors.RED,self.activePressure)])
        else:
            self.tRunPars.append([get_color_string(bcolors.GREEN,"Pressure "),get_color_string(bcolors.GREEN,self.activePressure)])
        
        #add the rows and yada yada
        self.tab_runParams.add_rows(self.tRunPars)
        self.tab_runParams.set_cols_align(['c','r'])
        self.tab_runParams.header(['Run Parameters','Value'])
        #add to the final string
        self.displayData += self.tab_runParams.draw()
        ################
        
    #Code to refresh the display. Run manually.
    def updateDisplay(self, mainloop_object, userData):
        self.txt = urwid.Text(self.displayData, align='left')
        self.fill = urwid.Filler(self.txt, 'top')
        self.loop.widget = self.fill
        self.loop.draw_screen()
        #self.loop.set_alarm_in(2, self.updateDisplay)
        
    def updateTemperature(self, temperature_data):
        self.AllTemps = temperature_data
    
    def updatePressure(self, pressure_data):
        self.activePressure = pressure_data
        
