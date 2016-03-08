// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once
#ifndef MEMDISK_HPP
#define MEMDISK_HPP

#include <cstdint>
#include <cstdio>
#include <deque>
#include <functional>
#include "hw/disk_device.hpp"

namespace fs
{
  class MemDisk : public hw::IDiskDevice
  {
  public:
    struct Entry
    {
      Entry(uint32_t blk, buffer_t dat)
        : block(blk), data(dat) {}
      
      uint32_t block;
      buffer_t data;
    };
    
    MemDisk(size_t cache_size = 16)
      : CACHE_SIZE(cache_size) {}
    
    void set_image(std::string disk_image)
    {
      image = disk_image;
    }
    
    virtual const char* name() const noexcept override
    {
      return "MemDisk";
    }
    virtual block_t size() const noexcept override
    {
      FILE* f = fopen(image.c_str(), "r");
      if (!f) return 0;
      
      fseek(f, 0L, SEEK_END);
      off64_t sz = ftell(f);
      fclose(f);
      
      return sz;
    }
    virtual block_t block_size() const noexcept override
    {
      return 512;
    }
    
    virtual void read(block_t blk, on_read_func func) override;
    virtual buffer_t read_sync(block_t) override;
    // not implemented here:
    virtual void read(block_t, block_t, on_read_func) override {}
    
  private:
    void free_entry()
    {
      cache.pop_front();
    }
    
    buffer_t read_block(block_t blk);
    
    std::string  image;
    const size_t CACHE_SIZE;
    std::deque<Entry> cache;
  };
  
}

#endif
