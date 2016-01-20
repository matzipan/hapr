// Created 2014-02-06 by Alex Oyston
// Modified 2014-02-06 by Alex Oyston
//  - Created iap_entry and various basic command functions
//    e.g. erase_sector, copy_ram_flash
//  - Created error function
//  - Created basic mem_to_flash and flash_to_mem routines
// Modified 2014-02-07 by Alex Oyston
//  - Included serial.c and debugging lines
//  - Added a check byte to get_filter_chain_size and mem_to_flash
//     to check we between a length of 255 and unwritten flash
//  - Added REPL Commands to main.c + tests
// Modified 2014-02-08 by Alex Oyston
//  - Changed mem, src_addr, dest_addr and others from
//     char* to uint16_t* to match with filters_buf in main.c and
//     filter_chain.c
//  - Fixed all the various warnings
// Modified 2014-02-09 by Alex Oyston
//  - Changed from writing entire sectors per filter chain to
//     writing mutliple filter chains per sector to save some
//     of it for Michael
//  - Added functions to write/read filter chains and filter counts
//  - Changed flash_to_mem and mem_to_flash to more general purpose
// Modified 2014-02-10 by Alex Oyston
//  - Removed warnings and added final tests/debug messages
//  - Integrated and tested with main.c

// Important resources
//  - User Manual for MBED
//  - Page 632 for IAP documentation
//  - http://www.nxp.com/documents/user_manual/UM10360.pdf

// ****************Map of flash memory********************/
//
// Sector      Sector      Start                End
// Number      Size [kB]   Address             Address
// 0  (0x00)   4           0x0000 0000         0x0000 0FFF
// 1  (0x01)   4           0x0000 1000         0x0000 1FFF
// 2  (0x02)   4           0x0000 2000         0x0000 2FFF
// 3  (0x03)   4           0x0000 3000         0x0000 3FFF
// 4  (0x04)   4           0x0000 4000         0x0000 4FFF
// 5  (0x05)   4           0x0000 5000         0x0000 5FFF
// 6  (0x06)   4           0x0000 6000         0x0000 6FFF
// 7  (0x07)   4           0x0000 7000         0x0000 7FFF
// 8  (0x08)   4           0x0000 8000         0x0000 8FFF
// 9  (0x09)   4           0x0000 9000         0x0000 9FFF
// 10 (0x0A)   4           0x0000 A000         0x0000 AFFF
// 11 (0x0B)   4           0x0000 B000         0x0000 BFFF
// 12 (0x0C)   4           0x0000 C000         0x0000 CFFF
// 13 (0x0D)   4           0x0000 D000         0x0000 DFFF
// 14 (0x0E)   4           0x0000 E000         0x0000 EFFF
// 15 (0x0F)   4           0x0000 F000         0x0000 FFFF
// 16 (0x10)   32          0x0001 0000         0x0001 7FFF
// 17 (0x11)   32          0x0001 8000         0x0001 FFFF
// 18 (0x12)   32          0x0002 0000         0x0002 7FFF
// 19 (0x13)   32          0x0002 8000         0x0002 FFFF
// 20 (0x14)   32          0x0003 0000         0x0003 7FFF
// 21 (0x15)   32          0x0003 8000         0x0003 FFFF
// 22 (0x16)   32          0x0004 0000         0x0004 7FFF
// 23 (0x17)   32          0x0004 8000         0x0004 FFFF
// 24 (0x18)   32          0x0005 0000         0x0005 7FFF
// 25 (0x19)   32          0x0005 8000         0x0005 FFFF
// 26 (0x1A)   32          0x0006 0000         0x0006 7FFF
// 27 (0x1B)   32          0x0006 8000         0x0006 FFFF
// 28 (0x1C)   32          0x0007 0000         0x0007 7FFF
// 29 (0x1D)   32          0x0007 8000         0x0007 FFFF

#ifndef _HAPR_IAP
#define _HAPR_IAP

#include "lpc17xx_pinsel.h"

#include "serial.h"
#include "iap.h"

// Error return codes
#define CMD_SUCCESS         0
#define INVALID_COMMAND     1
#define SRC_ADDR_ERROR      2
#define DST_ADDR_ERROR      3
#define SRC_ADDR_NOT_MAPPED 4
#define DST_ADDR_NOT_MAPPED 5
#define COUNT_ERROR         6
#define INVALID_SECTOR      7
#define SECTOR_NOT_BLANK    8
#define SECTOR_NOT_PREPARED 9
#define COMPARE_ERROR       10
#define BUSY                11
#define SECTOR_ERROR        12
#define START_OFFSET_ERROR  13
#define START_OFFSET_ERROR2 14
#define SIZE_ERROR          15

#define IAP_LOCATION 0x1FFF1FF1

// Size of a filter count block - 256 is the smallest possible
#define FILTER_COUNT_BLOCK_SIZE  256
// Size of a filter buffer block - 4096 bytes = 2048 uint16_t
#define FILTER_BUFFER_BLOCK_SIZE  4096
#define BLOCK_SIZE (FILTER_COUNT_BLOCK_SIZE + FILTER_BUFFER_BLOCK_SIZE)


uint16_t *sector_starts[14] = { (uint16_t *)0x00010000,
                                (uint16_t *)0x00018000,
                                (uint16_t *)0x00020000,
                                (uint16_t *)0x00028000,
                                (uint16_t *)0x00030000,
                                (uint16_t *)0x00038000,
                                (uint16_t *)0x00040000,
                                (uint16_t *)0x00048000,
                                (uint16_t *)0x00050000,
                                (uint16_t *)0x00058000,
                                (uint16_t *)0x00060000,
                                (uint16_t *)0x00068000,
                                (uint16_t *)0x00070000,
                                (uint16_t *)0x00078000 };

unsigned command[5];
unsigned output[5];

/******* IAP COMMAND FUNCTIONS *******/

void iap_entry(unsigned commands[],unsigned outputs[]){
  void (*iap)(unsigned [],unsigned []);
  iap = (void (*)(unsigned [],unsigned []))IAP_LOCATION;
  iap(commands,outputs);
}

/*
  Prepares sector(s) for writing / erasing
  Must be executed before copy_ram_flash() or erase_sector()
    int start => start sector
    int end   => end sector (use same as start to prepare a single sector)
*/
void prepare_sector_write(int start, int end){
  command[0] = 50; // command code
  command[1] = (unsigned) start;
  command[2] = (unsigned) end;

  iap_entry(command, output);
}

/*
  Copies from RAM to Flash
  Writes to flash. prepare_sector_write() must be executed beforehand
    uint16_t *src_addr  => source ram address from which data bytes are to be read
    uint16_t *dest_addr => destination flash address where data bytes are to be written
                       this address must be a 256 byte boundary
    int size        => number of bytes to be written starting at dest_addr
                       must be 256 | 512 | 1024 | 4096
*/
void copy_ram_flash(uint16_t *src_addr, uint16_t *dest_addr, int size){
  command[0] = 51; // command code
  command[1] = (unsigned) dest_addr;
  command[2] = (unsigned) src_addr;
  command[3] = (unsigned) size;
  command[4] = (unsigned) 100000; // CPU Clock Freq of MBED in KHz

  iap_entry(command,output);
}

/*
  Erases sector(s)
    int start => start sector
    int end   => end sector (use same as start to erase a single sector)
*/
void erase_sector(int start, int end){
  command[0] = 52; // command code
  command[1] = (unsigned) start;
  command[2] = (unsigned) end;
  command[3] = 72000; // CPU Clock Freq of MBED in KHz

  iap_entry(command,output);
}

/*
  Check if sector(s) are blank
    int start => start sector
    int end   => end sector (use same as start to check a single sector)
*/
void blank_check_sector(int start, int end){
  command[0] = 53; // command code
  command[1] = (unsigned) start;
  command[2] = (unsigned) end;

  iap_entry(command, output);
}

/*
  Compare 2 memory locations
    uint16_t *src_addr  => ram or flash address of bytes to be compared
    uint16_t *dest_addr => ram or flash address of bytes to be compared
    int size        => number of bytes to be compared
                       must be a multiple of 4
*/
void compare_flash_ram(uint16_t *src_addr, uint16_t *dest_addr, int size){
  command[0] = 56; // command code
  command[1] = (unsigned) dest_addr;
  command[2] = (unsigned) src_addr;
  command[3] = (unsigned) size;

  iap_entry(command,output);
}

void write_error(unsigned code){
  #if DEBUG==1
  switch(code){
    case INVALID_COMMAND:
      tty_writeln("Invalid command");
      break;
    case SRC_ADDR_ERROR:
      tty_writeln("Source address is not on a word boundary");
      break;
    case DST_ADDR_ERROR:
      tty_writeln("Destination address is not on a correct boundary");
      break;
    case SRC_ADDR_NOT_MAPPED:
      tty_writeln("Source address is not mapped in the memory map");
      break;
    case DST_ADDR_NOT_MAPPED:
      tty_writeln("Destination address is not mapped in the memory map");
      break;
    case COUNT_ERROR:
      tty_writeln("Byte count is not a multiple of 4 or is not a permitted value");
      break;
    case INVALID_SECTOR:
      tty_writeln("Sector number is invalid");
      break;
    case SECTOR_NOT_BLANK:
      tty_writeln("Sector is not blank");
      break;
    case SECTOR_NOT_PREPARED:
      tty_writeln("Sector was not prepared for write");
      break;
    case COMPARE_ERROR:
      tty_writeln("Source and destination data is not the same");
      break;
    case BUSY:
      tty_writeln("Flash programming hardware interface is busy");
      break;
    case SECTOR_ERROR:
      tty_writeln("Sector index must be between 0 and 13 inclusive");
      break;
    case START_OFFSET_ERROR:
      tty_writeln("Offset value must be a multiple of 256");
      break;
    case START_OFFSET_ERROR2:
      tty_writeln("Offset value must be less that 32768");
      break;
    case SIZE_ERROR:
      tty_writeln("Write size must be 256 | 512 | 1024 | 4096 - anything under 4096 will be rounded up");
      break;
  }
  #endif
}

// Check if a sector is blank and if not, erase it
// wipe_sector isn't working for unknown reasons
// Doesn't return an error, just seems to freeze the MBED
// uint16_t sector => sector id to wipe
unsigned wipe_sector(uint16_t sector){
  #if DEBUG==1
  tty_writeln("Wiping sector");
  #endif
  blank_check_sector(sector, sector);
  if(output[0] == SECTOR_NOT_BLANK){
    prepare_sector_write(sector, sector);
    if (output[0] !=0){
      write_error(output[0]);
      return output[0];
    }
    erase_sector(sector, sector);
    if (output[0] !=0){
      write_error(output[0]);
      return output[0];
    }
  }
  #if DEBUG==1
  tty_writeln("Sector wipe");
  #endif
  return 0;
}

// Get the filter chain size from flash from the requested block
// uint16_t *filters_count => address to write filters_count to
// uint16_t block => block id to read from
unsigned get_filter_chain_size(uint16_t *filters_count, uint16_t block){
  unsigned output;
  uint16_t sector;

  #if DEBUG==1
  tty_writeln("Getting filter chain size");
  #endif

  // Calculate the sector and offset from the block number
  //  using the defined sizes of filter chain and filter count
  block = block*BLOCK_SIZE;
  sector = 11 + 2*(block > 32768 - BLOCK_SIZE); //Using (11 + 2*...) because 12th sector of flash was not writing
  block = block % (32768 - BLOCK_SIZE);

  output = read_flash(filters_count, 1, sector, block);

  // 0xFFFF is unwritten flash
  if(*filters_count == 0xFFFF){
    *filters_count = 0;
  }
  #if DEBUG==1
  tty_writeln("Filter chain size retrieved");
  #endif

  return output;
}

// Copy filter chain from requested flash block
// uint16_t *mem => memory address to start writing to
// uint16_t filters_count => number of filters to read from flash - each filter is 8 bytes
// uint16_t block => block id to read from
unsigned flash_to_filter_chain(uint16_t *mem, uint16_t filters_count, uint16_t block){
  uint16_t sector;
  unsigned output;

  #if DEBUG==1
  tty_writeln("Reading filter chain from flash");
  #endif

  block = block*BLOCK_SIZE;
  sector = 11 + 2*(block > 32768 - BLOCK_SIZE);
  block = block % (32768 - BLOCK_SIZE);

  // multiply filters_count by 8 because there are 8 bytes per filter
  output = read_flash(mem, filters_count*8, sector, block+FILTER_COUNT_BLOCK_SIZE);

  #if DEBUG==1
  tty_writeln("Filter chain retrieved");
  #endif

  return output;
}

// Copy filter chain to requested flash block
// uint16_t *filters_buf => memory address to start reading the filter chain from
// uint16_t *filters_count => memory address to read number of filters from
// uint16_t block => block id to write to
unsigned filter_chain_to_flash(uint16_t *filters_buf, uint16_t *filters_count, uint16_t block){
  unsigned output;
  uint16_t sector;

  #if DEBUG==1
  tty_writeln("Writing filter chain to flash");
  #endif

  block = block*BLOCK_SIZE;
  sector = 11 + 2*(block > 32768 - BLOCK_SIZE);
  block = block % (32768 - BLOCK_SIZE);

  output = write_flash(filters_count, 1, sector, block, 0);
  if(output)
    return output;
  output = write_flash(filters_buf, (*filters_count)*8, sector, block + FILTER_COUNT_BLOCK_SIZE, 0);

  #if DEBUG==1
  tty_writeln("Filter chain written");
  #endif

  return output;
}

// Read 'size' uint16_t's from the position in flash 'sector + start_offset'
// Copy the read data into the passed uint16_t array
// uint16_t *mem => memory address to write data to
// uint16_t size => number of bytes to write into mem
// uint8_t sector => sector of flash to read from
// uint16_t start_offset => offset from the sector start
unsigned read_flash(uint16_t *mem, uint16_t size, uint8_t sector, uint16_t start_offset){
  uint16_t *start = sector_starts[sector];
  int i;

  #if DEBUG==1
  tty_writeln("Reading Flash");
  #endif

  if(sector > 13){
    return SECTOR_ERROR;
  }
  for(i = 0; i < size; ++i){
    mem[i] = start[i+start_offset];
  }

  #if DEBUG==1
  tty_writeln("Flash read");
  #endif

  return 0;
}

// Write 'size' uint16_t's to flash from the passed array
// uint16_t *mem => memory address to read data from
// uint16_t size => number of bytes to read from mem - must be 256 | 512 | 1024 | 4096
// uint8_t sector => sector of flash to write to
// uint16_t start_offset => offset from the sector start - must be a multiple of 256
// blank_sector => 1 == blank sector before write (NOT WORKING SINCE erase_sector IS BROKEN)
unsigned write_flash(uint16_t *mem, uint16_t size, uint8_t sector, uint16_t start_offset, uint16_t blank_sector){

  #if DEBUG==1
  tty_writeln("Starting write to flash");
  #endif

  if(sector > 13){
    return SECTOR_ERROR;
  }

  // first 15 sectors are reserved for the program to store itself and have room to spare
  uint8_t target_sector = sector + 16;
  uint16_t *start_target_sector = sector_starts[sector];

  // start_offset must be a multiple of 256
  if((start_offset & 255) != 0){
    return START_OFFSET_ERROR;
  }

  // start_offset must be less than the size of the sector
  if(start_offset >= 32 * 1024){
    return START_OFFSET_ERROR2;
  }

  // mem_size must be 256 | 512 | 1024 | 4096 bytes
  int mem_size;
  if(size <= 256){
    mem_size = 256;
  }else if(size <= 512){
    mem_size = 512;
  }else if(size <= 1024){
    mem_size = 1024;
  }else if(size <= 4096){
    mem_size = 4096;
  }else{
    return SIZE_ERROR;
  }

  if(blank_sector){
    wipe_sector(target_sector);
  }

  //Prepare sector for writing
  prepare_sector_write(target_sector, target_sector);
  if (output[0] !=0){
    write_error(output[0]);
    return output[0];
  }

  //Copy the data to flash
  copy_ram_flash(mem, start_target_sector+start_offset, mem_size);
  if (output[0] !=0){
    write_error(output[0]);
    return output[0];
  }

  //Check the data was written correctly
  compare_flash_ram(mem, start_target_sector+start_offset, mem_size);
  if (output[0] !=0){
    write_error(output[0]);
    return output[0];
  }

  #if DEBUG==1
  tty_writeln("Write to flash successful");
  #endif

  return 0;
}

#endif
