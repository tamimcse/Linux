from subprocess import call

out = open("demofile1.txt", "a")

with open("SAIL_FIB.txt") as f:
    for line in f:
	arr = line.replace("\r\n", "").split("\t")
        fib_index = int(arr[1]) % 32
        out.write("{0}\tveth{1}\r\n".format(arr[0], fib_index))

