  ######################
 #    FAT32 reader    #
######################

FILES  = main.cpp fat.cpp mbr.cpp vbr.cpp disk.cpp
OUTPUT = FAT

CC = clang++-3.8 -std=c++11

###########################
CFLAGS = -MMD -Wall -Wextra -O0 -g -march=native
LFLAGS = -static-libgcc -static-libstdc++

OBJS = $(FILES:.cpp=.o)
DEPS = $(OBJS:.o=.d)

.cpp.o:
	$(CC) -c $(CFLAGS) $< -o $@

all: $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $(OUTPUT)

clean:
	$(RM) $(OBJS) $(DEPS) $(OUTPUT)

-include $(DEPS)
