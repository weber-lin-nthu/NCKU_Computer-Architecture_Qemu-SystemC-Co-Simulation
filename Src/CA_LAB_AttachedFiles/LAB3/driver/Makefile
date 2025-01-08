# If KERNELRELEASE is defined, we've been invoked from the
# kernel build system and can use its language.
ifneq ($(KERNELRELEASE),)
	obj-m := calc_drv.o
	
# Otherwise we were called directly from the command
# line; invoke the kernel build system.
else

	SRC=
	OBJ=${SRC:.c=.o}
	TARGET=

	KERNELDIR ?= ~/workspace/linux-4.14.85/
	PWD := $(shell pwd)
	ARCH=arm
	ToolPath=~/workspace/arm-2014.05
	CROSS_COMPILE=$(ToolPath)/bin/arm-none-linux-gnueabi-


default:$(TARGET)
	@echo "MK\tmodules"
	@$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNELDIR) M=$(PWD) modules

$(TARGET):$(OBJ)
	@echo "LD\t$@"
	@$(CROSS_COMPILE)gcc $< -o $@

%.o:%.c
	@echo "CC\t$<"
	@$(CROSS_COMPILE)gcc -c $< -o $@


clean:
	@echo "RM\t$(OBJ) $(TARGET) Modules"
	@$(MAKE) -C $(KERNELDIR) M=$(PWD) clean
	@rm -f $(OBJ) $(TARGET)
endif

