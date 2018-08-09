from subprocess import call

with open("SAIL_FIB_50K_veth.txt") as f:
    for line in f:
        # Do something with 'line'
	arr = line.replace("\r\n", "").split("\t")
    	print arr
	call('sudo ip route add {0} table main dev {1}'.format(arr[0], arr[1]), shell=True)
