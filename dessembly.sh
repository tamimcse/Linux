#Print all header
objdump -x samples/bpf/mf.o
#Deassembly
objdump -d -m i386:x86-64 samples/bpf/mf.o
