#ifndef FS_FAT_HPP
#define FS_FAT_HPP

#include "filesystem.hpp"
#include "disk_device.hpp"
#include <functional>
#include <cstdint>
#include <memory>

namespace fs
{
  
  struct FAT32 : public FileSystem
  {
    // FAT types
    static const int T_FAT12 = 0;
    static const int T_FAT16 = 1;
    static const int T_FAT32 = 2;
    
    struct cl_dir
    {
      uint8_t  shortname[11];
      uint8_t  attrib;
      uint8_t  pad1[8];
      uint16_t cluster_hi;
      uint8_t  pad2[4];
      uint16_t cluster_lo;
      uint32_t size;
      
      inline bool is_longname() const
      {
        return (attrib & 0x0F) == 0x0F;
      }
      
      inline uint32_t cluster() const
      {
        return cluster_lo | (cluster_hi << 16);
      }
      
    } __attribute__((packed));
    
    struct cl_long
    {
      uint8_t  index;
      uint16_t first[5];
      uint8_t  attrib;
      uint8_t  entry_type;
      uint8_t  checksum;
      uint16_t second[6];
      uint16_t zero;
      uint16_t third[2];
      
      uint8_t long_index() const
      {
        return index & ~0x40;
      }
      uint8_t is_last() const
      {
        return (index & 0x40) != 0;
      }
    } __attribute__((packed));
    
    // helper functions
    uint32_t cl_to_sector(uint32_t cl)
    {
      return data_index + (cl - 2) * sectors_per_cluster;
    }
    
    uint16_t cl_to_entry_offset(uint32_t cl)
    {
      if (fat_type == T_FAT16)
          return (cl * 2) % sector_size;
      else // T_FAT32
          return (cl * 4) % sector_size;
    }
    uint16_t cl_to_entry_sector(uint32_t cl)
    {
      if (fat_type == T_FAT16)
          return reserved + (cl * 2 / sector_size);
      else // T_FAT32
          return reserved + (cl * 4 / sector_size);
    }
    
    // constructor
    FAT32(std::shared_ptr<IDiskDevice> idev);
    ~FAT32() {}
    
    // 0   = Mount MBR
    // 1-4 = Mount VBR 1-4
    virtual void mount(uint8_t partid, on_mount_func on_mount) override;
    
    // path is a path in the mounted filesystem
    virtual void ls(const std::string& path, on_ls_func) override;
    
  private:
    // initialize filesystem by providing base sector
    void init(const void* base_sector);
    // return a list of entries from directory entries at @sector
    typedef std::function<void(bool, std::vector<Dirent>&)> on_internal_ls_func;
    void int_ls(uint32_t sector, std::vector<Dirent>&, on_internal_ls_func);
    bool int_dirent(const void* data, std::vector<Dirent>&);
    
    // device we can read and write sectors to
    std::shared_ptr<IDiskDevice> device;
    
    // private members
    uint16_t sector_size; // from bytes_per_sector
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
  
} // fs

#endif
