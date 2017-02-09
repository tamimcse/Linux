import csv
h1 = list(csv.reader(open("h1.data"), delimiter=" "))
h3 = list(csv.reader(open("h3.data"), delimiter=" "))
h5 = list(csv.reader(open("h5.data"), delimiter=" "))
#h1_im = list(csv.reader(open("h1-im.data"), delimiter=" "))
#h3_im = list(csv.reader(open("h3-im.data"), delimiter=" "))
#h5_im = list(csv.reader(open("h5-im.data"), delimiter=" "))

optimal = 333

for row in h1:
  time = row[0]
  x1 = (int(row[11]) * 8.0 / 1024)/optimal
  print x1
     

#print h1
