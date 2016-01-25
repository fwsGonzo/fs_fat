#ifndef FS_MBR_HPP
#define FS_MBR_HPP

#include <cstdint>
#include <string>

namespace fs
{
  struct MBR
  {
    static const int PARTITIONS = 4;
    
    struct type
    {
      std::string name;
      uint8_t     type;
    };
    
    struct partition
    {
      uint8_t flags;
      uint8_t CHS_BEG[3];
      uint8_t type;
      uint8_t CHS_END[3];
      uint32_t lba_begin;
      uint32_t sectors;
      
    } __attribute__((packed));
    
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
    
    struct mbr
    {
      uint8_t   jump[3];
      char      oem_name[8];
      uint8_t   boot[435]; // boot code
      partition part[PARTITIONS];
      uint16_t  magic; // 0xAA55
      
      inline BPB* bpb()
      {
        return (BPB*) boot;
      }
      
    } __attribute__((packed));
    
    static std::string id_to_name(uint8_t);
  };
  
} // fs

#endif
