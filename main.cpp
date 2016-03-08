#include <cstdio>
#include <cstdlib>
#include <cassert>

#include <cerrno>
#include <cstring>

#include <fs/disk.hpp>
#include <fs/fat.hpp>
#include <memdisk.hpp>

using namespace fs;
MemDisk device;

off64_t check_image(const char* path)
{
  FILE* f = fopen(path, "r");
  if (!f) return 0;
  
  fseek(f, 0L, SEEK_END);
  off64_t sz = ftell(f);
  fclose(f);
  
  return sz;
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
  
  device.set_image(argv[1]);
  
  using MountedDisk = Disk<FAT>;
  auto disk = std::make_shared<MountedDisk> (device);
  
  disk->partitions(
  [] (fs::error_t err, auto& parts)
  {
    if (err)
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
  
  // mount the partition described by the Master Boot Record
  disk->mount(
  [disk] (bool err)
  {
    if (err)
    {
      printf("Could not mount MBR\n");
      return;
    }
    
    printf("-= ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ =-\n");
    printf("-=           DISK MOUNTED             =-\n");
    printf("-= ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ =-\n");
    
    disk->fs().ls("/TestDir",
    [] (bool err, FileSystem::dirvec_t ents)
    {
      if (err)
      {
        printf("Could not list root directory");
        return;
      }
      
      for (auto& e : *ents)
      {
        printf("Entry: %s of size %lu bytes\n",
            e.name().c_str(), e.size);
        printf("Entry cluster: %lu\n", e.block);
      }
    });
    printf("--------------------------------------\n");
    
    /*
    disk.open("/Koala.jpg",
    [] (bool good, const Disk::Dirent& ent, Disk::File& file)
    {
      file.read(
    });*/
    
  });
  
  return 0;
}
