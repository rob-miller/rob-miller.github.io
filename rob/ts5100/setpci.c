
#include <stdio.h>
#include <asm/io.h>

#include "pci.h"

#define BASE 0x2e

int main()
{
        struct pci_access *acc;
        struct pci_dev *dev;
        byte onebyte;
        word twobyte;

        acc = pci_alloc();
        pci_init(acc);

        dev = pci_get_dev(acc, 0x00, 0x1f, 0x00);  // 5100 also dev 1f

        onebyte = pci_read_byte(dev, 0xe0);  // COM_DEC register
        onebyte &= 0x8f;     // wipe COMB decode range bits
	onebyte |= 0x10;     // set to 2f8-2ff
	//onebyte |= 0x00;     // set to 3f8-3ff
        pci_write_byte(dev, 0xe0, onebyte);

        twobyte = pci_read_word(dev, 0xe6);  // LPC_EN register
        twobyte &= 0xfffd;   // wipe bit 1
        twobyte |= 0x0002;   // set bit 1 : COMB addr range enable
        pci_write_word(dev, 0xe6, twobyte);

        twobyte = pci_read_word(dev, 0x90);  // PCI_DMA register
	twobyte &= 0xfff3;   // wipe bits 2,3
	twobyte |= 0x000c;   // set bits 3,2 - channel 1 select
	//twobyte = 0xc0c0;       // LPC I/F DMA on, channel 3  -- rtm (?? PCI DMA ?)
        pci_write_word(dev, 0x90, twobyte);

        pci_write_word(dev, 0xec, 0x131);  // LPC I/F 2nd decode range

	pci_write_byte(dev, 0x60, 0x0c);  // PIRQA_ROUT  route irq 12  (rtm)
	//	pci_write_byte(dev, 0x60, 0x03);  // PIRQA_ROUT  route irq 03  (rtm)
        pci_free_dev(dev);

        pci_cleanup(acc);
}
