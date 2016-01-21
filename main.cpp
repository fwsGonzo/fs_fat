#include "fat.hpp"

#include <cstdio>
#include <cstdlib>
#include <cassert>

#include <cerrno>
#include <cstring>
#include <malloc.h>

#include <deque>
#include <iostream>
#include <locale>
#include <codecvt>

off64_t check_image(const char* path)
{
  FILE* f = fopen(path, "r");
  if (!f) return 0;
  
  fseek(f, 0L, SEEK_END);
  return ftell(f);
}


#define UNICODE_SURROGATE_PAIR -2
#define UNICODE_BAD_INPUT -1
// 2014 lovewilliam <ztong@vt.edu>
// http://www.lemoda.net/c/ucs2-to-utf8/ucs2-to-utf8.c
int ucs2_to_utf8 (int ucs2, unsigned char * utf8)
{
    if (ucs2 < 0x80) {
        utf8[0] = ucs2;
        //utf8[1] = '\0';
        return 1;
    }
    if (ucs2 >= 0x80  && ucs2 < 0x800) {
        utf8[0] = (ucs2 >> 6)   | 0xC0;
        utf8[1] = (ucs2 & 0x3F) | 0x80;
        //utf8[2] = '\0';
        return 2;
    }
    if (ucs2 >= 0x800 && ucs2 < 0xFFFF) {
	if (ucs2 >= 0xD800 && ucs2 <= 0xDFFF) {
	    /* Ill-formed. */
	    return UNICODE_SURROGATE_PAIR;
	}
        utf8[0] = ((ucs2 >> 12)       ) | 0xE0;
        utf8[1] = ((ucs2 >> 6 ) & 0x3F) | 0x80;
        utf8[2] = ((ucs2      ) & 0x3F) | 0x80;
        //utf8[3] = '\0';
        return 3;
    }
    if (ucs2 >= 0x10000 && ucs2 < 0x10FFFF) {
	/* http://tidy.sourceforge.net/cgi-bin/lxr/source/src/utf8.c#L380 */
	utf8[0] = 0xF0 | (ucs2 >> 18);
	utf8[1] = 0x80 | ((ucs2 >> 12) & 0x3F);
	utf8[2] = 0x80 | ((ucs2 >> 6) & 0x3F);
	utf8[3] = 0x80 | ((ucs2 & 0x3F));
        //utf8[4] = '\0';
        return 4;
    }
    return UNICODE_BAD_INPUT;
}

struct Disk
{
  struct Entry
  {
    Entry(uint32_t blk, uint8_t* dat)
      : block(blk), data(dat) {}
    
    uint32_t block;
    uint8_t* data;
  };
  
  Disk(const char* image)
    : path(image) {}
  
  static const int MAX_Q = 16;
  
  uint8_t* read_sector(uint32_t blk)
  {
    FILE* f = fopen(path, "r");
    if (!f) return nullptr;
    
    fseek(f, blk * 512, SEEK_SET);
    
    uint8_t* buffer = new uint8_t[512];
    int res = fread(buffer, 512, 1, f);
    if (res < 0) perror(strerror(errno));
    
    return buffer;
  }
  
  uint8_t* read_block(uint32_t blk)
  {
    // check for existing entry in cache
    for (auto& entry : cache)
    {
      if (entry.block == blk)
          return entry.data;
    }
    // make room if needed
    while (cache.size() >= MAX_Q)
        free_entry();
    // retrieve block from disk
    uint8_t* data = read_sector(blk);
    // save for later
    cache.emplace_back(blk, data);
    return data;
  }
  
  void free_entry()
  {
    const auto& entry = cache.front();
    delete[] entry.data;
    
    cache.pop_front();
  }
  
  const char* path;
  std::deque<Entry> cache;
};

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
  off64_t size = check_image(argv[1]);
  if (size == 0)
  {
    printf("Failed to open: %s\n", strerror(errno));
    return EXIT_FAILURE;
  }
  
  printf("Loaded image %s (%lu bytes)\n", argv[1], size);
  
  Disk disk(argv[1]);
  auto* mbr = (MBR::mbr*) disk.read_sector(0);
  assert(mbr != nullptr);
  
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
  printf("OEM name: %s\n", mbr->oem_name);
  
  // Let's begin our incantation
  // To drive out the demons of old DOS we have to read some PBP values
  auto* bpb = (FAT32::BPB*) mbr->boot;
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
      pfat.sectors_per_fat = *(uint32_t*) &mbr->boot[25];
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
    pfat.root_cluster = *(uint32_t*) &mbr->boot[33];
    printf("The image is type FAT32, with %u clusters\n", pfat.clusters);
  }
  printf("Root cluster index: %u\n", pfat.root_cluster);
  printf("System ID: %.8s\n", bpb->system_id);
  printf("--------------------------------------\n");
  
  // Attempt to read the root directory
  int S = pfat.cl_to_sector(pfat.root_cluster);
  
  printf("Root directory data sector: %u\n", S);
  auto* root = (FAT32::cl_dir*) disk.read_sector(S);
  for (int i = 0; i < 16; i++)
  {
    if (root[i].shortname[0] == 0x0)
    {
      // end of directory
      break;
    }
    else if (root[i].shortname[0] == 0xE5)
    {
      //printf("Unused index\n");
    }
    else
    {
        // convert from UCS-2 to wchar_t (UCS-4)
        std::wstring_convert<std::codecvt_utf16<wchar_t, 0x10ffff, std::little_endian>, wchar_t> conv;
        if (root[i].is_longname())
        {
          auto* L = (FAT32::cl_long*) &root[i];
          // the last long index is part of a chain of entries
          if (L->is_last())
          {
            setlocale(LC_ALL,"");
            
            // buffer for long filename
            char final_name[256];
            int  final_count = 0;
            
            int  total = L->long_index();
            // go to the last entry and work backwards
            i += total-1;
            L += total-1;
            
            for (int idx = total; idx > 0; idx--)
            {
              uint16_t longname[13];
              memcpy(longname+ 0, L->first, 10);
              memcpy(longname+ 5, L->second, 12);
              memcpy(longname+11, L->third, 4);
              
              int ui = 0;
              for (int j = 0; j < 13; j++)
              {
                //ui += ucs2_to_utf8(longname[j], &utf8name[ui]);
                // 0xFFFF indicates end of name
                if (longname[j] == 0xFFFF) break;
                
                final_name[final_count] = longname[j] & 0xFF;
                final_count++;
              }
              L--;
              
              if (final_count > 240)
              {
                printf("Suspicious long name length, breaking...\n");
                break;
              }
            }
            
            final_name[final_count] = 0;
            printf("Long name: %s\n", final_name);
          }
        }
        else
        {
          printf("Short name: %.11s\n", root[i].shortname);
        }
    }
  }
  
  return 0;
}
