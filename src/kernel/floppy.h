#ifndef FLOPPY_H
#define FLOPPY_H

extern volatile bool disk_interrupt_handled;

void init_floppy_driver();

void read_floppy_sector(int lba);

extern const void * floppy_dma_buffer;

#endif
