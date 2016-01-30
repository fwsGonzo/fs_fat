#include "memdisk.hpp"

#include <cstring>
#include <cassert>

namespace fs
{
  void MemDisk::read_sector(uint32_t blk, on_read_func func)
  {
    // check for existing entry in cache
    for (auto& entry : cache)
    if (entry.block == blk)
    {
      func(entry.data);
      return;
    }
    // make room if needed
    while (cache.size() >= CACHE_SIZE)
        free_entry();
    // retrieve block from disk
    read_block(blk,
    [this, blk, func] (const void* data)
    {
      if (data == nullptr)
      {
          printf("Failed to read block %u\n", blk);
          return;
      }
      // save for later
      cache.emplace_back(blk, (uint8_t*) data);
      func(data);
    });
  }
  
  void MemDisk::read_block(uint32_t blk, on_read_func func)
  {
    FILE* f = fopen(image.c_str(), "r");
    if (!f)
    {
      func(nullptr);
      return;
    }
    
    fseek(f, blk * 512, SEEK_SET);
    
    auto* buffer = new decltype(Entry::data)[512];
    int res = fread(buffer, 512, 1, f);
    if (res < 0)
    {
      printf("read_block fread failed\n");
      func(nullptr);
      return;
    }
    // call event handler for successful block read
    func(buffer);
  }
}
