  ######################
 #    FAT32 reader    #
######################

FILES  = fat.cpp mbr.cpp
OUTPUT = FAT

CC = g++ -std=c++11

###########################
CFLAGS = -Wall -Wextra -O0 -g -march=native
LFLAGS = -static-libgcc -static-libstdc++

OBJS = $(FILES:.cpp=.o)

.cpp.o:
	$(CC) -c $(CFLAGS) $< -o $@

all: $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $(OUTPUT)
