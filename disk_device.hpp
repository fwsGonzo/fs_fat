#ifndef DISK_DEVICE_HPP
#define DISK_DEVICE_HPP

#include <cstdint>
#include <functional>
#include <memory>

class IDiskDevice
{
public:
  // To be used by caching mechanism for disk drivers
  typedef std::shared_ptr<void*> buffer;
  // Delegate for result of reading a disk sector
  typedef std::function<void(const void*)> on_read_func;
  
  virtual void read_sector(uint32_t blk, on_read_func func) = 0;
  
};

#endif
