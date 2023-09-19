uname_p := $(shell uname -p) # run uname to find architecture
$(info uname_p=$(uname_p))
ifeq ($(strip $(uname_p)),arm)
$(info makefile: arm64 detected, changing defaults)
CPP = /opt/homebrew/opt/llvm/bin/clang
CPPFLAGS = -I/opt/homebrew/opt/libomp/include -fopenmp
LDFLAGS = -L/opt/homebrew/opt/libomp/lib
else
$(info makefile: no arm64 detected, preserving defaults)
CPPFLAGS = -fopenmp
# leave defaults
endif

prefix_sums: prefix_sums.c
	$(CPP) $(CPPFLAGS) $^ -o $@ $(LDFLAGS)

sample_omp.out:sample_omp.c
	$(CPP) $(CPPFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm *.o
	rm *.out