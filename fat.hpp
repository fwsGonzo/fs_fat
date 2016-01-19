#include <cstdint>


struct part_t
{
  uint8_t flags;
  uint8_t CHS_BEG[3];
  uint8_t type;
  uint8_t CHS_END[3];
  uint32_t lba_begin;
  uint32_t sectors;
};

struct mbr_t
{
  uint8_t bootcode[446];
  part_t  part1;
  part_t  part2;
  part_t  part3;
  part_t  part4;
  uint8_t magic[2]; // 0x55 0xAA
};

