obj-m += tsulab.o

PWD := $(shell pwd)

all:
 $(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
 $(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean