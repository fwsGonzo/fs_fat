#include <cstdio>
#include <cstdlib>
#include <cassert>

#include <cerrno>
#include <cstring>

#include "disk.hpp"
#include "memdisk.hpp"
#include <memory>

using namespace fs;

off64_t check_image(const char* path)
{
  FILE* f = fopen(path, "r");
  if (!f) return 0;
  
  fseek(f, 0L, SEEK_END);
  return ftell(f);
}

void print_bits(size_t const size, void const * const ptr)
{
    unsigned char *b = (unsigned char*) ptr;
    unsigned char byte;
    int i, j;

    for (i=size-1;i>=0;i--)
    {
        for (j=7;j>=0;j--)
        {
            byte = b[i] & (1<<j);
            byte >>= j;
            printf("%u", byte);
        }
    }
    puts("");
}

int main(int argc, const char** argv)
{
  assert(argc > 1);
  off64_t size = check_image(argv[1]);
  if (size == 0)
  {
    printf("Failed to open: %s\n", strerror(errno));
    return EXIT_FAILURE;
  }
  
  printf("Image %s is %lu bytes\n", argv[1], size);
  printf("--------------------------------------\n");
  
  using MountedDisk = Disk<0, FAT32>;
  auto device = std::make_shared<MemDisk> (argv[1], 16);
  auto disk   = std::make_shared<MountedDisk> (device);
  
  // mount the partition described by the Master Boot Record
  disk->fs().mount(MountedDisk::PART_MBR,
  [&disk] (bool good)
  {
    if (!good)
    {
      printf("Could not mount MBR\n");
      return;
    }
    
    printf("-= ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ =-\n");
    printf("-=           DISK MOUNTED             =-\n");
    printf("-= ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ =-\n");
    
    printf("--------------------------------------\n");
    disk->partitions(
    [] (bool good, std::vector<MountedDisk::Partition>& parts)
    {
      if (!good)
      {
        printf("Failed to retrieve volumes on disk\n");
        return;
      }
      
      for (auto& part : parts)
      {
        printf("Volume: %s at LBA %u\n",
            part.name().c_str(), part.lba_begin);
      }
    });
    
    printf("--------------------------------------\n");
    
    disk->fs().ls("/",
    [] (bool good, std::vector<FileSystem::Dirent> ents)
    {
      if (!good)
      {
        printf("Could not list root directory");
      }
      
      for (auto& e : ents)
      {
        printf("Entry: %s of size %u bytes\n",
            e.name.c_str(), e.size);
        printf("Entry cluster: %u\n", e.cluster);
      }
    });
    
    /*
    disk.open("/Koala.jpg",
    [] (bool good, const Disk::Dirent& ent, Disk::File& file)
    {
      file.read(
    });*/
    
  });
  
  return 0;
}
