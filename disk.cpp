#include "disk.hpp"

#include <cstring>
#include <cassert>

namespace fs
{
  template <>
  void Disk<FAT32>::mount(uint8_t partid, on_mount_func on_mount)
  {
    // read Master Boot Record (sector 0)
    this->read_sector(0,
    [this, partid, on_mount] (uint8_t* data)
    {
      auto* mbr = (MBR::mbr*) data;
      assert(mbr != nullptr);
      printf("--------------------------------------\n");
      
      // verify image signature
      printf("OEM name: \t%s\n", mbr->oem_name);
      printf("MBR signature: \t0x%x\n", mbr->magic);
      assert(mbr->magic == 0xAA55);
      printf("--------------------------------------\n");
      
      // print all the extended partitions
      /*
        <P0> Flags: 0	Type: DOS 3.0+ 16-bit FAT	LBA begin: 3f
        <P1> Flags: 0	Type: DOS 3.0+ 16-bit FAT	LBA begin: ccc0
        <P2> Flags: 0	Type: DOS 3.0+ 16-bit FAT	LBA begin: 19980
        <P3> Flags: 0	Type: DOS 3.3+ Extended Partition	LBA begin: 26640
      */
      for (int i = 0; i < 4; i++)
      {
        printf("<P%u> ", i+1);
        printf("Flags: %u\t", mbr->part[i].flags);
        printf("Type: %s\t", MBR::id_to_name( mbr->part[i].type ).c_str() );
        printf("LBA begin: %x\n", mbr->part[i].lba_begin);
      }
      // all the partitions are offsets to potential Volume Boot Records
      printf("--------------------------------------\n");
      
      /// the mount partition id tells us the LBA offset to the volume
      // assume MBR for now
      assert(partid == 0);
      // initialize FAT16 or FAT32 filesystem
      filesys = std::make_shared<FAT32> (mbr);
      // on_mount callback
      on_mount(true);
    });
    
  }
  
  template <>
  void Disk<FAT32>::ls(const std::string& path, on_ls_func on_ls)
  {
    /// FIXME: filesys is not an interface atm.
    /// FIXME: filesys is not an interface atm.
    /// FIXME: filesys is not an interface atm.
    (void) path;
    
    // Attempt to read the root directory
    int S = filesys->cl_to_sector(filesys->root_cluster);
    
    printf("Root directory data sector: %u\n", S);
    this->read_sector(S,
    [this, on_ls] (uint8_t* data)
    {
      std::vector<Dirent> dirents;
      
      auto* root = (FAT32::cl_dir*) data;
      if (!root)
      {
        // could not read sector
        on_ls(false, dirents);
        return;
      }
      
      for (int i = 0; i < 16; i++)
      {
        if (root[i].shortname[0] == 0x0)
        {
          // end of directory
          break;
        }
        else if (root[i].shortname[0] == 0xE5)
        {
          // unused index
        }
        else
        {
            // convert from UCS-2 to wchar_t (UCS-4)
            // in some parallell universe this works properly, or character encoding is not completely tarded:
            //std::wstring_convert<std::codecvt_utf16<wchar_t, 0x10ffff, std::little_endian>, wchar_t> conv;
            
            if (root[i].is_longname())
            {
              auto* L = (FAT32::cl_long*) &root[i];
              // the last long index is part of a chain of entries
              if (L->is_last())
              {
                setlocale(LC_ALL,"");
                
                // buffer for long filename
                char final_name[256];
                int  final_count = 0;
                
                int  total = L->long_index();
                // go to the last entry and work backwards
                i += total-1;
                L += total-1;
                
                for (int idx = total; idx > 0; idx--)
                {
                  uint16_t longname[13];
                  memcpy(longname+ 0, L->first, 10);
                  memcpy(longname+ 5, L->second, 12);
                  memcpy(longname+11, L->third, 4);
                  
                  for (int j = 0; j < 13; j++)
                  {
                    // 0xFFFF indicates end of name
                    if (longname[j] == 0xFFFF) break;
                    
                    final_name[final_count] = longname[j] & 0xFF;
                    final_count++;
                  }
                  L--;
                  
                  if (final_count > 240)
                  {
                    printf("Suspicious long name length, breaking...\n");
                    break;
                  }
                }
                
                final_name[final_count] = 0;
                printf("Long name: %s\n", final_name);
                
                i++; // skip over the long version
                // use short version for the stats
                auto* D = &root[i];
                std::string dirname(final_name, final_count);
                
                dirents.emplace_back(dirname, D->cluster(), D->size, D->attrib);
              }
            }
            else
            {
              auto* D = &root[i];
              printf("Short name: %.11s\n", D->shortname);
              std::string dirname((char*) D->shortname, 11);
              
              dirents.emplace_back(dirname, D->cluster(), D->size, D->attrib);
            }
        }
      } // directory list
      
      // callback
      on_ls(true, dirents);
      
    }); // read root dir
    
  }
  
  template <>
  void Disk<FAT32>::read_block(uint32_t blk, on_read_func func)
  {
    FILE* f = fopen(path, "r");
    if (!f)
    {
      func(nullptr);
      return;
    }
    
    fseek(f, blk * 512, SEEK_SET);
    
    uint8_t* buffer = new uint8_t[512];
    int res = fread(buffer, 512, 1, f);
    if (res < 0)
    {
      printf("read_block fread failed\n");
      func(nullptr);
      return;
    }
    // call event handler for successful block read
    func(buffer);
  }
  
  template <>
  void Disk<FAT32>::partitions(on_parts_func func)
  {
    // read Master Boot Record (sector 0)
    this->read_sector(0,
    [this, func] (uint8_t* data)
    {
      std::vector<Partition> parts;
      
      if (!data)
      {
        func(false, parts);
        return;
      }
      
      // first sector is the Master Boot Record
      auto* mbr = (MBR::mbr*) data;
      
      for (int i = 0; i < 4; i++)
      {
        printf("<P%u> ", i+1);
        printf("Flags: %u\t", mbr->part[i].flags);
        printf("Type: %s\t", MBR::id_to_name( mbr->part[i].type ).c_str() );
        printf("LBA begin: %x\n", mbr->part[i].lba_begin);
        
        parts.emplace_back(
            mbr->part[i].flags,    // flags
            mbr->part[i].type,     // id
            mbr->part[i].lba_begin, // LBA
            mbr->part[i].sectors);
      }
      
      func(true, parts);
    });
  }
  
  template <>
  void Disk<FAT32>::read_sector(uint32_t blk, on_read_func func)
  {
    // check for existing entry in cache
    for (auto& entry : cache)
    if (entry.block == blk)
    {
      func(entry.data);
      return;
    }
    // make room if needed
    while (cache.size() >= CACHE_SIZE)
        free_entry();
    // retrieve block from disk
    read_block(blk,
    [this, blk, func] (uint8_t* data)
    {
      if (data == nullptr)
      {
          printf("Failed to read block %u\n", blk);
          return;
      }
      // save for later
      cache.emplace_back(blk, data);
      func(data);
    });
  }
  
}
