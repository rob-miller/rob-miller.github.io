#include <stdio.h>
#include <asm/io.h>

#define BASE 0x2e

int main()
{
        int i;

        ioperm(BASE, 2, 1);
        outb(0x55, BASE);    // enter configuration state
        outb(0x0d, BASE);    // ?? twiggle config port ??
        if (inb(BASE+1) == 0x5a)  // ?? read something from index port ??
	{
	  outb(0x25, BASE);   // select CR25 - UART2 base addr
	  outb(0xBE, BASE+1); // bits 2-9 of 0x2f8
	  //outb(0xFE, BASE+1); // bits 2-9 of 0x3f8

	  outb(0x28, BASE);   // select CR28 - UART1,2 IRQ select
	  i = inb(BASE+1);    // get current setting for both
	  //	  outb((i & 0xf0) | 0x0a, BASE+1);  // low order bits to 0a=irq10
	  //	  outb((i & 0xf0) | 0x0c, BASE+1);  // low order bits to 0c=irq12
	  outb((i & 0xf0) | 0x03, BASE+1);  // low order bits to 03=irq03
	  // but want no irq for serial uart1 ?
    
	  outb(0x2B, BASE);   // CR2B - SCE (FIR) base addr
	  outb(0x26, BASE+1); // 0x130 bits 2-9

	  outb(0x2C, BASE);   // CR2C - SCE (FIR) DMA select
	  //	  outb(0x01, BASE+1); // DMA 1
	  outb(0x03, BASE+1); // DMA 3

	  outb(0x0C, BASE);   // CR0C - UART mode
	  i = inb(BASE+1);    // whatever already there
	  outb((i & 0xC7) | 0x08, BASE+1);   // enable IrDA (HPSIR) mode

	  outb(0x07, BASE);   // CR07 - Auto Pwr Mgt/boot drive sel
	  i = inb(BASE+1);    // whatever already there 
	  outb(i | 0x20, BASE+1);  // enable UART2 autopower down
	  //outb(i & ~0x20, BASE+1);  // disable UART2 autopower down

	  outb(0x0a, BASE);   // CR0a - ecp fifo / ir mux
	  i = inb(BASE+1);    // whatever already there 
	  outb(i | 0x40, BASE+1);  // send active device to ir port

	  outb(0x02, BASE);   // CR02 - UART 1,2 power
	  i = inb(BASE+1);    // whatever already there
	  outb(i | 0x80, BASE+1);  // UART2 power up mode

	  outb(0x00, BASE);   // CR00 - FDC Power/valid config cycle
	  i = inb(BASE + 1);  // whatever already there
	  outb(i | 0x80, BASE + 1);  // valid config cycle done

	  outb(0xaa, BASE);   // ?? twiggle config register ??
        }
}
