#include <cstdint>
#include <string>

struct MBR
{
  static const int PARTITIONS = 4;
  
  struct type
  {
    std::string name;
    uint8_t     type;
  };
  
  struct partition
  {
    uint8_t flags;
    uint8_t CHS_BEG[3];
    uint8_t type;
    uint8_t CHS_END[3];
    uint32_t lba_begin;
    uint32_t sectors;
    
  } __attribute__((packed));

  struct mbr
  {
    uint8_t   boot[446]; // boot code
    partition part[PARTITIONS];
    uint16_t  magic; // 0xAA55
    
  } __attribute__((packed));
  
  
  static std::string id_to_name(uint8_t);
};
