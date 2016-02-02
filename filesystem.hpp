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
#ifndef FS_FILESYS_HPP
#define FS_FILESYS_HPP

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace fs
{
  class FileSystem
  {
  public:
    struct Dirent
    {
      Dirent(std::string n, uint32_t cl, uint32_t sz, uint8_t attr)
        : name(n), cluster(cl), size(sz), attrib(attr) {}
      
      std::string name;
      uint32_t cluster;
      uint32_t size;
      uint8_t  attrib;
    };
    
    typedef std::shared_ptr<std::vector<Dirent>> dirvec_t;
    
    typedef std::function<void(bool)> on_mount_func;
    typedef std::function<void(bool, dirvec_t)> on_ls_func;
    
    
    // 0   = Mount MBR
    // 1-4 = Mount VBR 1-4
    virtual void mount(uint8_t partid, on_mount_func on_mount) = 0;
    
    // path is a path in the mounted filesystem
    virtual void ls(const std::string& path, on_ls_func) = 0;
  };
}

#endif
