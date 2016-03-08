#include "memdisk.hpp"

#include <cstring>
#include <cassert>

namespace fs
{
  void MemDisk::read(block_t blk, on_read_func callback)
  {
    auto buf = read_sync(blk);
    callback(buf);
  }
  MemDisk::buffer_t MemDisk::read_sync(block_t blk)
  {
    // check for existing entry in cache
    for (auto& entry : cache)
    if (entry.block == blk)
    {
      return entry.data;
    }
    // make room if needed
    while (cache.size() >= CACHE_SIZE)
        free_entry();
    // retrieve block from disk
    auto data = read_block(blk);
    if (!data)
    {
      printf("Failed to read block %lu\n", blk);
      return buffer_t();
    }
    // save for later
    cache.emplace_back(blk, data);
    return data;
  }
  
  MemDisk::buffer_t MemDisk::read_block(block_t blk)
  {
    FILE* f = fopen(image.c_str(), "r");
    if (!f)
    {
      return buffer_t();
    }
    
    fseek(f, blk * block_size(), SEEK_SET);
    
    auto* buffer = new uint8_t[block_size()];
    int res = fread(buffer, block_size(), 1, f);
    // fail when not reading block size
    if (res < 0)
    {
      printf("read_block (blk=%lu) fread failed: %s\n", blk, strerror(errno));
      return buffer_t();
    }
    // call event handler for successful block read
    return buffer_t(buffer, std::default_delete<uint8_t[]>());
  }
}
