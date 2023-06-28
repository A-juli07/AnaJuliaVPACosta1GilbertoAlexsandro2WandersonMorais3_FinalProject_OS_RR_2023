obj-y += islenefs.o

islenefs-objs :=  namei.o file.o inode.o

KVERSION = 5.19.0-43-generic
all:
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) clean
