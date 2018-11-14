#This scripts reads actual-routes.txt and converts all the next-hops with veth0-31 

from subprocess import call
import random

out = open("transformed-routes.txt", "a")

with open("actual-routes.txt") as f:
    for line in f:
	arr = line.replace("\r\n", "").split("\t")
#        fib_index = int(arr[1]) % 32
        fib_index = random.randint(0,31)
        out.write("{0}\tveth{1}\r\n".format(arr[0], fib_index))

