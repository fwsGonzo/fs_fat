#ifndef FS_DISK_HPP
#define FS_DISK_HPP

#include <functional>
#include <deque>
#include <memory>
#include <vector>
#include "fat.hpp"

namespace fs
{
  struct Disk
  {
    struct Dirent;
    typedef std::function<void(bool)> on_mount_func;
    typedef std::function<void(bool, std::vector<Dirent>)> on_ls_func;
    typedef std::function<void(uint8_t*)> on_read_func;
    
    static const uint8_t PART_MBR = 0;
    static const uint8_t PART_VBR1 = 1;
    static const uint8_t PART_VBR2 = 2;
    static const uint8_t PART_VBR3 = 3;
    static const uint8_t PART_VBR4 = 4;
    
    struct Volume
    {
      int flags;
      int id;
      uint32_t lba_begin;
      
      bool is_boot() const
      {
        return flags & 0x1;
      }
      
      std::string name() const
      {
        return MBR::id_to_name(id);
      }
    };
    
    struct Dirent
    {
      Dirent(std::string n, uint32_t cl, uint32_t sz, uint8_t attr)
        : name(n), cluster(cl), size(sz), attrib(attr) {}
      
      std::string name;
      uint32_t cluster;
      uint32_t size;
      uint8_t  attrib;
    };
    
    struct Entry
    {
      Entry(uint32_t blk, uint8_t* dat)
        : block(blk), data(dat) {}
      
      uint32_t block;
      uint8_t* data;
    };
    
    Disk(const char* image, int cached_sectors)
      : path(image), CACHE_SIZE(cached_sectors) {}
    
    // 0   = Mount MBR
    // 1-4 = Mount VBR 1-4
    void mount(uint8_t partid, on_mount_func);
    
    void ls(const std::string& path, on_ls_func);
    
    void read_sector(uint32_t blk, on_read_func func);
    
    // returns a vector of the volumes available on disk
    std::vector<Volume> volumes();
    
  private:
    void read_block(uint32_t blk, on_read_func func);
    void free_entry()
    {
      const auto& entry = cache.front();
      delete[] entry.data;
      
      cache.pop_front();
    }
    
    const char*  path;
    const size_t CACHE_SIZE;
    std::deque<Entry> cache;
    
    std::shared_ptr<FAT32> filesys;
  };
  
} // fs

#endif
