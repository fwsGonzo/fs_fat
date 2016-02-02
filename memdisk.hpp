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
