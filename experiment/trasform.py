from subprocess import call

out = open("demofile.txt", "a")

with open("SAIL_FIB_50K.txt") as f:
    for line in f:
	arr = line.replace("\r\n", "").split("\t")
	fib_index = int(arr[1]) % 16
#    	print "{0}\tveth{1}".format(arr[0], fib_index)
	out.write("{0}\tveth{1}\r\n".format(arr[0], fib_index))
#	call('sudo ip route add {0} table main dev {1}'.format(arr[0], arr[1]), shell=True)
