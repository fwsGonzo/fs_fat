#include "fat.hpp"

namespace fs
{
  FAT32::FAT32(MBR::mbr* mbr)
    : SECTOR_SIZE(mbr->bpb()->bytes_per_sector)
  {
    MBR::BPB* bpb = mbr->bpb();
    
    // Let's begin our incantation
    // To drive out the demons of old DOS we have to read some PBP values
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
    
    // initialize FAT
    if (bpb->small_sectors) // FAT16
    {
        this->fat_type  = FAT32::T_FAT16;
        this->sectors = bpb->small_sectors;
        this->sectors_per_fat = bpb->sectors_per_fat;
        this->root_dir_sectors = ((bpb->root_entries * 32) + (SECTOR_SIZE - 1)) / SECTOR_SIZE;
        printf("Root dir sectors: %u\n", this->root_dir_sectors);
    }
    else
    {
        this->fat_type   = FAT32::T_FAT32;
        this->sectors = bpb->large_sectors;
        this->sectors_per_fat = *(uint32_t*) &mbr->boot[25];
        this->root_dir_sectors = 0;
    }
    // calculate index of first data sector
    this->data_index = bpb->reserved_sectors + (bpb->fa_tables * this->sectors_per_fat) + this->root_dir_sectors;
    printf("First data sector: %u\n", this->data_index);
    // number of reserved sectors is needed constantly
    this->reserved = bpb->reserved_sectors;
    printf("Reserved sectors: %u\n", this->reserved);
    // number of sectors per cluster is important for calculating entry offsets
    this->sectors_per_cluster = bpb->sectors_per_cluster;
    printf("Sectors per cluster: %u\n", this->sectors_per_cluster);
    // calculate number of data sectors
    this->data_sectors = this->sectors - this->data_index;
    printf("Data sectors: %u\n", this->data_sectors);
    // calculate total cluster count
    this->clusters = this->data_sectors / this->sectors_per_cluster;
    printf("Total clusters: %u\n", this->clusters);
    
    // now that we're here, we can determine the actual FAT type
    // using the official method:
    if (this->clusters < 4085)
    {
      this->fat_type = FAT32::T_FAT12;
      this->root_cluster = 2;
      printf("The image is type FAT12, with %u clusters\n", this->clusters);
    }
    else if (this->clusters < 65525)
    {
      this->fat_type = FAT32::T_FAT16;
      this->root_cluster = 2;
      printf("The image is type FAT16, with %u clusters\n", this->clusters);
    }
    else
    {
      this->fat_type = FAT32::T_FAT32;
      this->root_cluster = *(uint32_t*) &mbr->boot[33];
      printf("The image is type FAT32, with %u clusters\n", this->clusters);
    }
    printf("Root cluster index: %u\n", this->root_cluster);
    printf("System ID: %.8s\n", bpb->system_id);
  }
  
  
  
}
