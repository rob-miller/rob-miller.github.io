
/*
**
**  dumpsmc.c 
**
**  A hack to put the SMSC LPC47N227 into loopback mode
**  and start dumping to / reading from the IR port.
**
**  update 14 Aug 2003 : switch to glibc headers for ioperm,
**       need additional ioperm call on sir_base.
**
*/

#include <stdio.h>
//#include <asm/io.h>
#include <unistd.h>
#include <sys/io.h>
#include <errno.h>

#define SMC_BASE 0x2e

#define RCV base
#define IER base+1
#define IIR base+2
#define FCR base+2
#define LCR base+3
#define MCR base+4
#define LSR base+5
#define MSR base+6
#define SPR base+7
#define DLSB base
#define DMSB base+1

typedef struct uart {
  short rcv,ier,iir,lcr,mcr,lsr,msr,spr,dlsb,dmsb;
} uart;

uart curr,last;

void init_uvals() {
  curr.rcv = curr.ier = curr.iir = curr.lcr = curr.mcr = curr.lsr = curr.spr = 0;
  curr.dlsb = curr.dmsb = 0;
}

void cpy_uart() {
  last.rcv = curr.rcv;
  last.ier = curr.ier;
  last.iir = curr.iir;
  last.lcr = curr.lcr;
  last.mcr = curr.mcr;
  last.lsr = curr.lsr;
  last.spr = curr.spr;
  last.dlsb = curr.dlsb;
  last.dmsb = curr.dmsb;
}

int cmp_uart() {
  if (last.rcv != curr.rcv) return 1;
  if (last.ier != curr.ier) return 1;
  if (last.iir != curr.iir) return 1;
  if (last.lcr != curr.lcr) return 1;
  if (last.mcr != curr.mcr) return 1;
  if (last.lsr != curr.lsr) return 1;
  if (last.spr != curr.spr) return 1;
  if (last.dlsb != curr.dlsb) return 1;
  if (last.dmsb != curr.dmsb) return 1;
  return 0;
}

void print_uart() {
  fprintf(stderr,"rcv %02x ie %02x ii %02x lc %02x mc %02x ls %02x ms %02x sp %02x dl %02x dm %02x\n",
	  curr.rcv,curr.ier,curr.iir,curr.lcr,curr.mcr,curr.lsr,curr.msr,curr.spr,curr.dlsb,curr.dmsb);
}

void set_dlab(base,bool) {
  int lcr;

  lcr = inb(LCR);
  
  if (bool) 
    lcr |= 0x80;  // set DLAB
   else 
    lcr &= ~(0x80);  // clr DLAB
  
  outb(lcr,LCR);
}


void get_curr_uart(base) {
  set_dlab(base,0);
  curr.rcv = inb(RCV);
  curr.ier = inb(IER);
  curr.iir = inb(IIR);
  curr.lcr = inb(LCR);
  curr.mcr = inb(MCR);
  curr.lsr = inb(LSR);
  curr.msr = inb(MSR);
  curr.spr = inb(SPR);

  set_dlab(base,1);
  curr.dlsb = inb(DLSB);
  curr.dmsb = inb(DMSB);

  set_dlab(base,0);
}
  
void set_baud(base,lsb,msb) {

  set_dlab(base,1);

  outb(lsb,DLSB);
  outb(msb,DMSB);

  set_dlab(base,0);

}

void out_smc(base,val) {
  outb(val,RCV);
}

int config_start() {
  int rslt=0;
  rslt = ioperm(SMC_BASE, 2, 1);
  if (rslt) {
    perror("ioperm call failed");
    exit(-1);
  }
  outb(0x55, SMC_BASE); // config mode on
  outb(0x0d, SMC_BASE);
  if (inb(SMC_BASE+1) == 0x5a)  // config mode ok ?
    return 1;
  return 0;
}

void config_stop() {
  int i;
  outb(0x00, SMC_BASE);   // CR00 - FDC Power/valid config cycle
  i = inb(SMC_BASE + 1);  // whatever already there
  outb(i | 0x80, SMC_BASE + 1);  // valid config cycle done
  outb(0xaa, SMC_BASE);   // ?? twiggle config register ??
}

void set_loopback(base,bool) {
  int i;
  i = inb(MCR);
  if (bool) 
    outb(i | 0x10,MCR);
  else
    outb(i & ~0x10,MCR);
}

int main()
{
  int sir_base;
  int rslt=0;

  if (!config_start()) {
    fprintf(stderr,"error enabling config mode\n");
    exit(1);
  }

  outb(0x25, SMC_BASE);  // UART2 base
  sir_base = inb(SMC_BASE+1) << 2;

  //  config_stop();

  fprintf(stderr,"SIR base port:  %04x\n", sir_base);

  rslt = ioperm(sir_base, 8, 1);
  if (rslt) {
    perror("ioperm call failed");
    exit(-1);
  }

  if (!sir_base) {
    fprintf(stderr,"should not be 0 -- run setpci and setsmc first?\n");
    exit(-1);
  }


  set_loopback(sir_base,0);


  if (sir_base) {
    int oval=0xa5a5;
    ioperm(sir_base,8,1);
    //    ioperm(sir_base+8,2,1);

    set_baud(sir_base,2,0);

    
    init_uvals;
    cpy_uart();

    for(;;) {
      get_curr_uart(sir_base);
      if (cmp_uart()) {
	print_uart();
	cpy_uart();
      }
      //sleep(1);
      out_smc(sir_base,oval);
      oval = ~oval;
    }
  }
  
  fprintf(stderr,"\n");

  return 0;
}

