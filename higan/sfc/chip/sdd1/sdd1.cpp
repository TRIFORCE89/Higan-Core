#include <sfc/sfc.hpp>

#define SDD1_CPP
namespace SuperFamicom {

SDD1 sdd1;

#include "decomp.cpp"
#include "serialization.cpp"

void SDD1::init() {
}

void SDD1::load() {
  //hook S-CPU DMA MMIO registers to gather information for struct dma[];
  //buffer address and transfer size information for use in SDD1::mcu_read()
  bus.map({&SDD1::read, &sdd1}, {&SDD1::write, &sdd1}, 0x00, 0x3f, 0x4300, 0x437f);
  bus.map({&SDD1::read, &sdd1}, {&SDD1::write, &sdd1}, 0x80, 0xbf, 0x4300, 0x437f);
}

void SDD1::unload() {
  rom.reset();
  ram.reset();
}

void SDD1::power() {
}

void SDD1::reset() {
  sdd1_enable = 0x00;
  xfer_enable = 0x00;
  dma_ready = false;

  mmc[0] = 0 << 20;
  mmc[1] = 1 << 20;
  mmc[2] = 2 << 20;
  mmc[3] = 3 << 20;

  for(unsigned i = 0; i < 8; i++) {
    dma[i].addr = 0;
    dma[i].size = 0;
  }
}

uint8 SDD1::read(unsigned addr) {
  addr &= 0xffff;

  if((addr & 0x4380) == 0x4300) {
    return cpu.mmio_read(addr);
  }

  switch(addr) {
  case 0x4804: return mmc[0] >> 20;
  case 0x4805: return mmc[1] >> 20;
  case 0x4806: return mmc[2] >> 20;
  case 0x4807: return mmc[3] >> 20;
  }

  return cpu.regs.mdr;
}

void SDD1::write(unsigned addr, uint8 data) {
  addr &= 0xffff;

  if((addr & 0x4380) == 0x4300) {
    unsigned channel = (addr >> 4) & 7;
    switch(addr & 15) {
    case 2: dma[channel].addr = (dma[channel].addr & 0xffff00) + (data <<  0); break;
    case 3: dma[channel].addr = (dma[channel].addr & 0xff00ff) + (data <<  8); break;
    case 4: dma[channel].addr = (dma[channel].addr & 0x00ffff) + (data << 16); break;

    case 5: dma[channel].size = (dma[channel].size &   0xff00) + (data <<  0); break;
    case 6: dma[channel].size = (dma[channel].size &   0x00ff) + (data <<  8); break;
    }
    return cpu.mmio_write(addr, data);
  }

  switch(addr) {
  case 0x4800: sdd1_enable = data; break;
  case 0x4801: xfer_enable = data; break;

  case 0x4804: mmc[0] = data << 20; break;
  case 0x4805: mmc[1] = data << 20; break;
  case 0x4806: mmc[2] = data << 20; break;
  case 0x4807: mmc[3] = data << 20; break;
  }
}

uint8 SDD1::mmc_read(unsigned addr) {
  return rom.read(mmc[(addr >> 20) & 3] + (addr & 0x0fffff));
}

//SDD1::mcu_read() is mapped to $c0-ff:0000-ffff
//the design is meant to be as close to the hardware design as possible, thus this code
//avoids adding S-DD1 hooks inside S-CPU::DMA emulation.
//
//the real S-DD1 cannot see $420b (DMA enable) writes, as they are not placed on the bus.
//however, $43x0-$43xf writes (DMAx channel settings) most likely do appear on the bus.
//the S-DD1 also requires fixed addresses for transfers, which wouldn't be necessary if
//it could see $420b writes (eg it would know when the transfer should begin.)
//
//the hardware needs a way to distinguish program code after $4801 writes from DMA
//decompression that follows soon after.
//
//the only plausible design for hardware would be for the S-DD1 to spy on DMAx settings,
//and begin spooling decompression on writes to $4801 that activate a channel. after that,
//it feeds decompressed data only when the ROM read address matches the DMA channel address.
//
//the actual S-DD1 transfer can occur on any channel, but it is most likely limited to
//one transfer per $420b write (for spooling purposes). however, this is not known for certain.
uint8 SDD1::mcurom_read(unsigned addr) {
  if(addr < 0x400000) {  //(addr & 0x408000) == 0x008000) {  //$00-3f|80-bf:8000-ffff
    return rom.read(addr);
  //addr = ((addr & 0x7f0000) >> 1) | (addr & 0x7fff);
  //return rom.read(addr);
  }

  //$40-7f|c0-ff:0000-ffff (MMC)
  if(sdd1_enable & xfer_enable) {
    //at least one channel has S-DD1 decompression enabled ...
    for(unsigned i = 0; i < 8; i++) {
      if(sdd1_enable & xfer_enable & (1 << i)) {
        //S-DD1 always uses fixed transfer mode, so address will not change during transfer
        if(addr == dma[i].addr) {
          if(!dma_ready) {
            //prepare streaming decompression
            decomp.init(addr);
            dma_ready = true;
          }

          //fetch a decompressed byte; once finished, disable channel and invalidate buffer
          uint8 data = decomp.read();
          if(--dma[i].size == 0) {
            dma_ready = false;
            xfer_enable &= ~(1 << i);
          }

          return data;
        }  //address matched
      }  //channel enabled
    }  //channel loop
  }  //S-DD1 decompressor enabled

  //S-DD1 decompression mode inactive; return ROM data
  return mmc_read(addr);
}

void SDD1::mcurom_write(unsigned addr, uint8 data) {
}

uint8 SDD1::mcuram_read(unsigned addr) {
  if((addr & 0x60e000) == 0x006000) {  //$00-3f|80-bf:6000-7fff
    return ram.read(addr & 0x1fff);
  }

  if((addr & 0xf08000) == 0x700000) {  //$70-7f:0000-7fff
    return ram.read(addr & 0x1fff);
  }

  return cpu.regs.mdr;
}

void SDD1::mcuram_write(unsigned addr, uint8 data) {
  if((addr & 0x60e000) == 0x006000) {  //$00-3f|80-bf:6000-7fff
    return ram.write(addr & 0x1fff, data);
  }

  if((addr & 0xf08000) == 0x700000) {  //$70-7f:0000-7fff
    return ram.write(addr & 0x1fff, data);
  }
}

}
