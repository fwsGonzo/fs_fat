#ifndef FS_FAT_HPP
#define FS_FAT_HPP

#include <cstdint>

#include "mbr.hpp"

struct FAT32
{
  // FAT types
  static const int T_FAT12 = 0;
  static const int T_FAT16 = 1;
  static const int T_FAT32 = 2;
  
  // Legacy BIOS Parameter Block
  struct BPB
  {
    uint16_t bytes_per_sector;
    uint8_t  sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t  fa_tables;
    uint16_t root_entries;
    uint16_t small_sectors;
    uint8_t  media_type; // 0xF8 == hard drive
    uint16_t sectors_per_fat;
    uint16_t sectors_per_track;
    uint16_t num_heads;
    uint32_t hidden_sectors;
    uint32_t large_sectors; // used if small_sectors == 0
    uint8_t  disk_number;   // starts at 0x80
    uint8_t  current_head;
    uint8_t  signature;     // must be 0x28 or 0x29
    uint32_t serial_number; // unique ID created by mkfs
    char     volume_label[11]; // deprecated
    char     system_id[8];  // FAT12 or FAT16
    
  } __attribute__((packed));
  
  struct bootsect
  {
    uint8_t jump[3]; // jump instruction
    char    oem[8];  // OEM name (eg. MSDOS5.0)
    BPB     bpb;
    
  } __attribute__((packed));
  
  struct fat
  {
    
  };
  
  struct cl_dir
  {
    uint8_t  shortname[11];
    uint8_t  attrib;
    uint8_t  pad1[8];
    uint16_t cluster_hi;
    uint8_t  pad2[4];
    uint16_t cluster_lo;
    uint32_t size;
    
  } __attribute__((packed));
  
  // helper functions
  uint32_t cl_to_sector(uint32_t cl)
  {
    return data_index + (cl - 2) * sectors_per_cluster;
  }
  
  uint16_t cl_to_entry_offset(uint32_t cl)
  {
    if (fat_type == T_FAT16)
        return (cl * 2) % SECTOR_SIZE;
    else // T_FAT32
        return (cl * 4) % SECTOR_SIZE;
  }
  uint16_t cl_to_entry_sector(uint32_t cl)
  {
    if (fat_type == T_FAT16)
        return reserved + (cl * 2 / SECTOR_SIZE);
    else // T_FAT32
        return reserved + (cl * 4 / SECTOR_SIZE);
  }
  
  // constructor
  FAT32(int ssize) : SECTOR_SIZE(ssize) {}
  
  // private members
  const int SECTOR_SIZE;
  uint32_t sectors;   // total sectors in partition
  uint32_t clusters;  // number of indexable FAT clusters
  
  uint8_t  fat_type;  // T_FAT12, T_FAT16 or T_FAT32
  uint16_t reserved;  // number of reserved sectors
  
  uint32_t sectors_per_fat;
  uint16_t sectors_per_cluster;
  uint16_t root_dir_sectors; // FAT16 root entries
  
  uint32_t root_cluster;  // index of root cluster
  uint32_t data_index;    // index of first data sector (relative to partition)
  uint32_t data_sectors;  // number of data sectors
};

#endif
