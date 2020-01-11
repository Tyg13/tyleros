#ifndef FLOPPY_H
#define FLOPPY_H

extern volatile bool disk_interrupt_handled;

void init_floppy_driver();
void prepare_floppy_dma(void * buffer);

void read_floppy(void * buffer, int start_sector, int sector_count);

#endif
