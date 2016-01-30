#ifndef MEMDISK_HPP
#define MEMDISK_HPP

#include <cstdint>
#include <deque>
#include <functional>
#include "disk_device.hpp"

namespace fs
{
  class MemDisk : public IDiskDevice
  {
  public:
    struct Entry
    {
      Entry(uint32_t blk, uint8_t* dat)
        : block(blk), data(dat) {}
      
      uint32_t block;
      uint8_t* data;
    };
    
    MemDisk(std::string disk_image, size_t cache_size = 16)
      : image(disk_image), CACHE_SIZE(cache_size)
    {
      
    }
    
    virtual void read_sector(uint32_t blk, on_read_func func) override;
    
  private:
    void free_entry()
    {
      const auto& entry = cache.front();
      delete[] entry.data;
      
      cache.pop_front();
    }
    
    void read_block(uint32_t blk, on_read_func func);
    
    std::string  image;
    const size_t CACHE_SIZE;
    std::deque<Entry> cache;
  };
  
}

#endif
