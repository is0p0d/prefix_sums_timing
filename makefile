# 2023 Jim Moroney
# Look Dr. Rogers it even runs on M1!
uname_p := $(shell uname -p) # run uname to find architecture
$(info uname_p=$(uname_p))

ifeq ($(strip $(uname_p)),arm)
$(info makefile: arm64 detected, changing defaults)
CPP = /opt/homebrew/opt/llvm/bin/clang
CPPFLAGS = -I/opt/homebrew/opt/libomp/include -fopenmp
LDFLAGS = -L/opt/homebrew/opt/libomp/lib
else
$(info makefile: no arm64 detected, preserving defaults)
CPP = /usr/bin/gcc
CPPFLAGS = -fopenmp
LDFLAGS = -lm
# leave defaults
endif

plot: prefix_sums output.dat
	./prefix_sums 30 output.dat && ./plotting.sh

prefix_sums: prefix_sums.c
	$(CPP) $(CPPFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm prefix_sums
	rm *.o
	rm *.png
	rm *.dat