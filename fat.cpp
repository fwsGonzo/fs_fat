#include "fat.hpp"
#include "mbr.hpp"

#include <cstdio>
#include <cstdlib>
#include <cassert>

#include <cerrno>
#include <cstring>
#include <malloc.h>

const char* read_image(const char* path, off64_t* size)
{
  FILE* f = fopen(path, "r");
  if (!f) return nullptr;
  
  fseek(f, 0L, SEEK_END);
  *size = ftell(f);
  fseek(f, 0L, SEEK_SET);
  
  char* buffer = (char*) malloc(*size);
  fread(buffer, *size, 1, f);
  return buffer;
}

int main(int argc, const char** argv)
{
  assert(argc > 1);
  
  off64_t size;
  const char* img = read_image(argv[1], &size);
  if (img == 0)
  {
    printf("Failed to open: %s\n", strerror(errno));
    return EXIT_FAILURE;
  }
  
  printf("Loaded image %s (%lu bytes)\n", argv[1], size);
  auto* mbr = (MBR::mbr*) img;
  
  printf("MBR signature: 0x%x\n", mbr->magic);
  assert(mbr->magic == 0xAA55);
  
  for (int i = 0; i < 4; i++)
  {
    printf("<P%u> ", i);
    printf("Flags: %u\t", mbr->part[i].flags);
    printf("Type: %s\t", MBR::id_to_name( mbr->part[i].type ).c_str() );
    printf("LBA begin: %x\n", mbr->part[i].lba_begin);
  }
  /*
    Loaded image ./ext-part.img (159989760 bytes)
    MBR signature: 0xaa55
    <P0> Flags: 0	Type: DOS 3.0+ 16-bit FAT	LBA begin: 3f
    <P1> Flags: 0	Type: DOS 3.0+ 16-bit FAT	LBA begin: ccc0
    <P2> Flags: 0	Type: DOS 3.0+ 16-bit FAT	LBA begin: 19980
    <P3> Flags: 0	Type: DOS 3.3+ Extended Partition	LBA begin: 26640
  */
  
  return 0;
}
