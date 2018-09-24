#!/usr/bin/env python

import texttable as tt
import random



tab=tt.Texttable()
tab2=tt.Texttable()


rows = [[]]
#for i in xrange(0,2):
#    random_int=random.randint(0,9)
#    rows.append([random_int,random_int**2,random_int**3])

tc = [0,1,2,3,4,5,6]
tc_set = [10,11,12]

rows.append(["Top",tc_set[0],tc[0],tc[1]])
rows.append(["Mid",tc_set[1],tc[2],tc[3]])
rows.append(["Bot",tc_set[2],tc[4],tc[5]])
#print rows
tab.add_rows(rows)
tab.set_cols_align(['c','r','r','r'])
tab.header(['Temp (C)','Set (C)','Right', 'Left'])

final_print = tab.draw()
################
rows2 =[[]]

rows2.append(["Pressure "," 5 PSI"])



tab2.add_rows(rows2)
tab2.set_cols_align(['c','r'])
tab2.header(['Run Parameters','Value'])

################



final_print += "\nGeyser params:\n"

final_print += tab2.draw()

print final_print