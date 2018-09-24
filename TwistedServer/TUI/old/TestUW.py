#!/usr/bin/env python

import texttable as tt
import time, sys
import random
import urwid, time


class TestTT():
    
    def __init__(self):
        self.tab = tt.Texttable()
        self.x = [[]] # The empty row will have the header
        self.initfill()
        self.txt = urwid.Text((self.tab.draw()), align='left')
        self.fill = urwid.Filler(self.txt, 'top')
        #self.loop = urwid.MainLoop(self.fill, unhandled_input=self.exit_on_q, event_loop=urwid.TwistedEventLoop())
        self.loop = urwid.MainLoop(self.fill, unhandled_input=self.exit_on_q)
    
    def start(self):
        self.loop.set_alarm_in(2, self.updateDisplay)
        self.loop.run()
        
        
    def exit_on_q(self, key):
        if key in ('q', 'Q'):
            raise urwid.ExitMainLoop()
        if key in ('r', 'R'):
            self.loop.draw_screen()


    
    def initfill(self):
        for i in range(1,3):
            self.x.append([i,i**2,i**3])
        self.tab.add_rows(self.x)
        self.tab.set_cols_align(['r','r','r'])
        self.tab.header(['Number', 'Number Squared', 'Number Cubed'])

    def updateDisplay(self, mainloop_object, userData):
        #time.sleep(1)
        random_int=random.randint(0,9)
        self.tab.reset()
        self.tab.add_rows([[],[random_int,random_int**2,random_int**3]])
        self.tab.set_cols_align(['r','r','r'])
        self.tab.header(['Number', 'Number Squared', 'Number Cubed'])
        self.txt = urwid.Text((self.tab.draw()), align='left')
        #self.txt = urwid.Text(("wwwwww"), align='left')
        self.fill = urwid.Filler(self.txt, 'top')
        self.loop.widget = self.fill
        self.loop.draw_screen()
        self.loop.set_alarm_in(2, self.updateDisplay)




t = TestTT()
t.start()