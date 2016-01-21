#include "fat.hpp"

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

void print_bits(size_t const size, void const * const ptr)
{
    unsigned char *b = (unsigned char*) ptr;
    unsigned char byte;
    int i, j;

    for (i=size-1;i>=0;i--)
    {
        for (j=7;j>=0;j--)
        {
            byte = b[i] & (1<<j);
            byte >>= j;
            printf("%u", byte);
        }
    }
    puts("");
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
  printf("\n");
  
  /*
    Loaded image ./ext-part.img (159989760 bytes)
    MBR signature: 0xaa55
    <P0> Flags: 0	Type: DOS 3.0+ 16-bit FAT	LBA begin: 3f
    <P1> Flags: 0	Type: DOS 3.0+ 16-bit FAT	LBA begin: ccc0
    <P2> Flags: 0	Type: DOS 3.0+ 16-bit FAT	LBA begin: 19980
    <P3> Flags: 0	Type: DOS 3.3+ Extended Partition	LBA begin: 26640
  */
  
  const char* oem_name = &img[3];
  printf("OEM name: %s\n", oem_name);
  
  // Let's begin our incantation
  // To drive out the demons of old DOS we have to read some PBP values
  auto* bpb = (FAT32::BPB*) (img+11);
  /*
  printf("Bytes per sector: \t%u\n", bpb->bytes_per_sector);
  printf("Sectors per cluster: \t%u\n", bpb->sectors_per_cluster);
  printf("Reserved sectors: \t%u\n", bpb->reserved_sectors);
  printf("Number of FATs: \t%u\n", bpb->fa_tables);
  
  printf("Small sectors (FAT16): \t%u\n", bpb->small_sectors);
  
  printf("Sectors per FAT: \t%u\n", bpb->sectors_per_fat);
  printf("Sectors per Track: \t%u\n", bpb->sectors_per_track);
  printf("Number of Heads: \t%u\n", bpb->num_heads);
  printf("Hidden sectors: \t%u\n", bpb->hidden_sectors);
  printf("Large sectors: \t%u\n", bpb->large_sectors);
  printf("Disk number: \t0x%x\n", bpb->disk_number);
  printf("Signature: \t0x%x\n", bpb->signature);
  
  printf("System ID: \t%.8s\n", bpb->system_id);
  */
  
  // Access FA tables
  const int SECTOR_SIZE = bpb->bytes_per_sector;
  assert(SECTOR_SIZE >= 512);
  printf("--------------------------------------\n");
  
  FAT32 pfat(SECTOR_SIZE);
  
  if (bpb->small_sectors) // FAT16
  {
      pfat.fat_type  = FAT32::T_FAT16;
      pfat.sectors = bpb->small_sectors;
      pfat.sectors_per_fat = bpb->sectors_per_fat;
      pfat.root_dir_sectors = ((bpb->root_entries * 32) + (SECTOR_SIZE - 1)) / SECTOR_SIZE;
      printf("Root dir sectors: %u\n", pfat.root_dir_sectors);
  }
  else
  {
      pfat.fat_type   = FAT32::T_FAT32;
      pfat.sectors = bpb->large_sectors;
      pfat.sectors_per_fat = *(uint32_t*) &img[36];
      pfat.root_dir_sectors = 0;
  }
  // calculate index of first data sector
  pfat.data_index = bpb->reserved_sectors + (bpb->fa_tables * pfat.sectors_per_fat) + pfat.root_dir_sectors;
  printf("First data sector: %u\n", pfat.data_index);
  // number of reserved sectors is needed constantly
  pfat.reserved = bpb->reserved_sectors;
  printf("Reserved sectors: %u\n", pfat.reserved);
  // number of sectors per cluster is important for calculating entry offsets
  pfat.sectors_per_cluster = bpb->sectors_per_cluster;
  printf("Sectors per cluster: %u\n", pfat.sectors_per_cluster);
  // calculate number of data sectors
  pfat.data_sectors = pfat.sectors - pfat.data_index;
  printf("Data sectors: %u\n", pfat.data_sectors);
  // calculate total cluster count
  pfat.clusters = pfat.data_sectors / pfat.sectors_per_cluster;
  printf("Total clusters: %u\n", pfat.clusters);
  
  // now that we're here, we can determine the actual FAT type
  // using the official method:
  if (pfat.clusters < 4085)
  {
    pfat.fat_type = FAT32::T_FAT12;
    pfat.root_cluster = 2;
    printf("The image is type FAT12, with %u clusters\n", pfat.clusters);
  }
  else if (pfat.clusters < 65525)
  {
    pfat.fat_type = FAT32::T_FAT16;
    pfat.root_cluster = 2;
    printf("The image is type FAT16, with %u clusters\n", pfat.clusters);
  }
  else
  {
    pfat.fat_type = FAT32::T_FAT32;
    pfat.root_cluster = *(uint32_t*) (img + 44);
    printf("The image is type FAT32, with %u clusters\n", pfat.clusters);
  }
  printf("Root cluster index: %u\n", pfat.root_cluster);
  printf("System ID: %.8s\n", bpb->system_id);
  printf("--------------------------------------\n");
  
  // Attempt to read the root directory
  int S = pfat.cl_to_sector(pfat.root_cluster);
  
  printf("Root directory data sector: %u\n", S);
  auto* root = (FAT32::cl_dir*) (img + S * SECTOR_SIZE);
  for (int i = 0; i < 16; i++)
  {
    if (root[i].shortname[0] == 0x0)
        // end of directory
        break;
    else if (root[i].shortname[0] == 0xE5)
        printf("Unused index\n");
    else
        printf("Short name: %.11s\n", root[i].shortname);
  }
  
  return 0;
}
