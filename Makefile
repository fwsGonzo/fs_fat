  ######################
 #    FAT32 reader    #
######################

FILES  = main.cpp memdisk.cpp fs/filesystem.cpp fs/fat.cpp fs/fat_sync.cpp fs/mbr.cpp fs/path.cpp
OUTPUT = FAT

CC = clang++-3.8 -std=c++14

###########################
CFLAGS = -MMD -Wall -Wextra -O0 -g -march=native -I.
LFLAGS = -static-libgcc -static-libstdc++

OBJS = $(FILES:.cpp=.o)
DEPS = $(OBJS:.o=.d)

.cpp.o:
	$(CC) -c $(CFLAGS) $< -o $@

all: $(OBJS)
	$(CC) -v $(LDFLAGS) $(OBJS) -o $(OUTPUT)

clean:
	$(RM) $(OBJS) $(DEPS) $(OUTPUT)

-include $(DEPS)
