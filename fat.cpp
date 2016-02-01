#include "fat.hpp"

#include <cassert>
#include "mbr.hpp"

#include <cstring>
#include <memory>
#include <locale>
#include <codecvt>

namespace fs
{
  FAT32::FAT32(std::shared_ptr<IDiskDevice> dev)
    : device(dev)
  {
    
  }
  
  void FAT32::init(const void* base_sector)
  {
    // assume its the master boot record for now
    auto* mbr = (MBR::mbr*) base_sector;
    
    MBR::BPB* bpb = mbr->bpb();
    this->sector_size = bpb->bytes_per_sector;
    
    // Let's begin our incantation
    // To drive out the demons of old DOS we have to read some PBP values
    printf("Bytes per sector: \t%u\n", bpb->bytes_per_sector);
    printf("Sectors per cluster: \t%u\n", bpb->sectors_per_cluster);
    printf("Reserved sectors: \t%u\n", bpb->reserved_sectors);
    printf("Number of FATs: \t%u\n", bpb->fa_tables);
    
    printf("Small sectors (FAT16): \t%u\n", bpb->small_sectors);
    
    printf("Sectors per FAT: \t%u\n", bpb->sectors_per_fat);
    printf("Sectors per Track: \t%u\n", bpb->sectors_per_track);
    printf("Number of Heads: \t%u\n", bpb->num_heads);
    printf("Hidden sectors: \t%u\n", bpb->hidden_sectors);
    printf("Large sectors: \t%u\n", bpb->large_sectors);
    printf("Disk number: \t0x%x\n", bpb->disk_number);
    printf("Signature: \t0x%x\n", bpb->signature);
    
    printf("System ID: \t%.8s\n", bpb->system_id);
    
    // initialize FAT
    if (bpb->small_sectors) // FAT16
    {
        this->fat_type  = FAT32::T_FAT16;
        this->sectors = bpb->small_sectors;
        this->sectors_per_fat = bpb->sectors_per_fat;
        this->root_dir_sectors = ((bpb->root_entries * 32) + (sector_size - 1)) / sector_size;
        printf("Root dir sectors: %u\n", this->root_dir_sectors);
    }
    else
    {
        this->fat_type   = FAT32::T_FAT32;
        this->sectors = bpb->large_sectors;
        this->sectors_per_fat = *(uint32_t*) &mbr->boot[25];
        this->root_dir_sectors = 0;
    }
    // calculate index of first data sector
    this->data_index = bpb->reserved_sectors + (bpb->fa_tables * this->sectors_per_fat) + this->root_dir_sectors;
    printf("First data sector: %u\n", this->data_index);
    // number of reserved sectors is needed constantly
    this->reserved = bpb->reserved_sectors;
    printf("Reserved sectors: %u\n", this->reserved);
    // number of sectors per cluster is important for calculating entry offsets
    this->sectors_per_cluster = bpb->sectors_per_cluster;
    printf("Sectors per cluster: %u\n", this->sectors_per_cluster);
    // calculate number of data sectors
    this->data_sectors = this->sectors - this->data_index;
    printf("Data sectors: %u\n", this->data_sectors);
    // calculate total cluster count
    this->clusters = this->data_sectors / this->sectors_per_cluster;
    printf("Total clusters: %u\n", this->clusters);
    
    // now that we're here, we can determine the actual FAT type
    // using the official method:
    if (this->clusters < 4085)
    {
      this->fat_type = FAT32::T_FAT12;
      this->root_cluster = 2;
      printf("The image is type FAT12, with %u clusters\n", this->clusters);
    }
    else if (this->clusters < 65525)
    {
      this->fat_type = FAT32::T_FAT16;
      this->root_cluster = 2;
      printf("The image is type FAT16, with %u clusters\n", this->clusters);
    }
    else
    {
      this->fat_type = FAT32::T_FAT32;
      this->root_cluster = *(uint32_t*) &mbr->boot[33];
      printf("The image is type FAT32, with %u clusters\n", this->clusters);
    }
    printf("Root cluster index: %u\n", this->root_cluster);
    printf("System ID: %.8s\n", bpb->system_id);
  }
  
  void FAT32::mount(uint8_t partid, on_mount_func on_mount)
  {
    // read Master Boot Record (sector 0)
    device->read_sector(0,
    [this, partid, on_mount] (const void* data)
    {
      auto* mbr = (MBR::mbr*) data;
      assert(mbr != nullptr);
      
      // verify image signature
      printf("OEM name: \t%s\n", mbr->oem_name);
      printf("MBR signature: \t0x%x\n", mbr->magic);
      assert(mbr->magic == 0xAA55);
      
      /// the mount partition id tells us the LBA offset to the volume
      // assume MBR for now
      assert(partid == 0);
      
      // initialize FAT16 or FAT32 filesystem
      init(mbr);
      
      // determine which FAT version is mounted
      switch (this->fat_type)
      {
      case FAT32::T_FAT12:
          printf("--> Mounting FAT12 filesystem\n");
          break;
      case FAT32::T_FAT16:
          printf("--> Mounting FAT16 filesystem\n");
          break;
      case FAT32::T_FAT32:
          printf("--> Mounting FAT32 filesystem\n");
          break;
      }
      
      // on_mount callback
      on_mount(true);
    });
  }
  
  bool FAT32::int_dirent(
      const void* data, 
      dirvec_t dirents)
  {
      auto* root = (FAT32::cl_dir*) data;
      bool  found_last = false;
      
      for (int i = 0; i < 16; i++)
      {
        if (root[i].shortname[0] == 0x0)
        {
          printf("end of dir\n");
          found_last = true;
          // end of directory
          break;
        }
        else if (root[i].shortname[0] == 0xE5)
        {
          printf("unused index %d\n", i);
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
                
                dirents->emplace_back(dirname, D->cluster(), D->size, D->attrib);
              }
            }
            else
            {
              auto* D = &root[i];
              printf("Short name: %.11s\n", D->shortname);
              std::string dirname((char*) D->shortname, 11);
              
              dirents->emplace_back(dirname, D->cluster(), D->size, D->attrib);
            }
        }
      } // directory list
      
      return found_last;
  }
  
  void FAT32::int_ls(
      uint32_t sector, 
      dirvec_t dirents, 
      on_internal_ls_func callback)
  {
    std::function<void(uint32_t)> next;
    
    next = [this, sector, callback, &dirents, next] (uint32_t sector)
    {
      printf("int_ls: sec=%u\n", sector);
      device->read_sector(sector,
      [this, sector, callback, &dirents, next] (const void* data)
      {
        if (!data)
        {
          // could not read sector
          callback(false, dirents);
          return;
        }
        
        // parse entries in sector
        bool done = int_dirent(data, dirents);
        if (done)
        {
          // execute callback
          callback(true, dirents);
        }
        else
        {
          // go to next sector
          next(sector+1);
        }
        
      }); // read root dir
    };
    
    // start reading sectors asynchronously
    next(sector);
  }
  
  void FAT32::ls(const std::string& path, on_ls_func on_ls)
  {
    // Attempt to read the root directory
    uint32_t S = this->cl_to_sector(this->root_cluster);
    printf("Reading root cluster %u at sector %u\n", this->root_cluster, S);
    // NOTE: ON STACK -->
    auto dirents = std::make_shared<std::vector<Dirent>> ();
    // NOTE: <-- ON STACK
    
    int_ls(S, dirents,
    [=] (bool good, dirvec_t ents)
    {
      on_ls(good, ents);
    });
  }
  
}
