#ifndef _HAPR_IAP_H
#define _HAPR_IAP_H

void iap_entry(unsigned commands[],unsigned outputs[]);
void prepare_sector_write(int start, int end);
void copy_ram_flash(uint16_t *src_addr, uint16_t *dest_addr, int size);
void erase_sector(int start, int end);
void blank_check_sector(int start, int end);
void compare_flash_ram(uint16_t *src_addr, uint16_t *dest_addr, int size);
void write_error(unsigned code);
unsigned read_flash(uint16_t *mem, uint16_t size, uint8_t sector, uint16_t start_offset);
unsigned write_flash(uint16_t *mem, uint16_t size, uint8_t sector, uint16_t start_offset, uint16_t blank_sector);
unsigned filter_chain_to_flash(uint16_t *filters_buf, uint16_t *filters_count, uint16_t sector);
unsigned flash_to_filter_chain(uint16_t *mem, uint16_t filters_count, uint16_t sector);
unsigned get_filter_chain_size(uint16_t *filters_count, uint16_t sector);

#endif