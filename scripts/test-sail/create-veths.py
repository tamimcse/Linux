from subprocess import call

for i in xrange(0,32,2):
    call('sudo ip link add veth{0} type veth peer name veth{1}'.format(i, i+1), shell=True)

for i in range(32):
    call('sudo ip link set dev veth{0} up'.format(i), shell=True)
