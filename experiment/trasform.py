from subprocess import call
import random

out = open("demofile.txt", "a")

with open("SAIL_FIB.txt") as f:
    for line in f:
	arr = line.replace("\r\n", "").split("\t")
#        fib_index = int(arr[1]) % 32
        fib_index = random.randint(0,31)
        out.write("{0}\tveth{1}\r\n".format(arr[0], fib_index))

