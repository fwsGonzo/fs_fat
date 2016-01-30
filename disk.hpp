#ifndef FS_DISK_HPP
#define FS_DISK_HPP

#include <functional>
#include <deque>
#include <memory>
#include <vector>
#include "fat.hpp"
#include "disk_device.hpp"

namespace fs
{
  template <int P, typename FS>
  class Disk
  {
  public:
    static const uint8_t PART_MBR = 0;
    static const uint8_t PART_VBR1 = 1;
    static const uint8_t PART_VBR2 = 2;
    static const uint8_t PART_VBR3 = 3;
    static const uint8_t PART_VBR4 = 4;
    
    struct Partition
    {
      Partition(uint8_t fl, uint8_t Id, uint32_t LBA, uint32_t sz)
          : flags(fl), id(Id), lba_begin(LBA), sectors(sz) {}
      
      uint8_t  flags;
      uint8_t  id;
      uint32_t lba_begin;
      uint32_t sectors;
      
      bool is_boot() const
      {
        return flags & 0x1;
      }
      
      std::string name() const;
    };
    
    /// callbacks  ///
    typedef std::function<void(bool, std::vector<Partition>&)> on_parts_func;
    typedef std::function<void(bool)> on_init_func;
    
    /// initialization ///
    Disk(std::shared_ptr<IDiskDevice> dev);
    
    /// filesystem functions
    FileSystem& fs()
    {
      return *filesys;
    }
    
    /// disk functions ///
    // returns a vector of the partitions on a disk
    // the disk does not need to be mounted beforehand
    void partitions(on_parts_func func);
    
  private:
    std::shared_ptr<IDiskDevice> device;
    std::unique_ptr<FS> filesys;
  };

  template <int P, typename FS>
  Disk<P, FS>::Disk(std::shared_ptr<IDiskDevice> dev)
    : device(dev)
  {
    filesys.reset(new FS(device));
  }
  
} // fs

#include "disk.inl"

#endif
