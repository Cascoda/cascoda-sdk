# This script breaks on every malloc & calloc, prints out the amount
# of memory allocated by said function call and then prints out
# the backtrace, identifying the guilty malloc.

# example on how to run it:
# alexandru@CASCODA211:~/cascoda-sdk-priv$ arm-none-eabi-gdb -ex "target remote 192.168.202.98:2331" -ex "file build/bin/knx-iot-lamp" --command=../memory-profiling.gdb

# For a full understanding of the heap, this script could be extended
# to break & print backtrace of every free as well. With post-processing
# of the output & maybe printing the returned pointer, every bit of
# allocated memory can be tracked.

set pagination off
set backtrace limit 8
b malloc
b calloc

commands 1
	print $r1
	bt
	c
end

commands 2
	print $r2
	bt
	c
end

monitor reset
continue
