import csv

def printme( h1, h3, h5, optimal, filename):
  f = open(filename, 'w')
  for row in h1:
    baseTime = float(row[0])

    x1 = (int(row[11]) * 8.0 / 1024)/optimal
  
    x3 = None
    x5 = None
    offset = None
  
    found = 100.0
    for r in h3:
      offset = abs(baseTime-float(r[0]))
      if offset < found:
        found = offset
        x3 = (int(r[11]) * 8.0 / 1024)/optimal

    found = 100.0
    for r in h5:
      offset = abs(baseTime-float(r[0]))
      if offset < found:
        found = offset
        x5 = (int(r[11]) * 8.0 / 1024)/optimal
    #Jain's fairness index
    fairness = pow(x1 + x3 + x5, 2)/(3 * (pow(x1, 2) + pow(x3, 2) + pow(x5, 2)))
    f.write(str(baseTime) +" "+ str(fairness) + "\n")
#    print baseTime, fairness
  f.close()


h1 = list(csv.reader(open("h1.data"), delimiter=" "))
h3 = list(csv.reader(open("h3.data"), delimiter=" "))
h5 = list(csv.reader(open("h5.data"), delimiter=" "))
h1_im = list(csv.reader(open("h1-im.data"), delimiter=" "))
h3_im = list(csv.reader(open("h3-im.data"), delimiter=" "))
h5_im = list(csv.reader(open("h5-im.data"), delimiter=" "))

optimal = 333

printme(h1, h3, h5, optimal, "fairness.data")
printme(h1_im, h3_im, h5_im, optimal, "fairness-im.data")
