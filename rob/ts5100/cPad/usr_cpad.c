/**********************
 *
 * usr_cpad
 *
 *  a user space client to access the Synaptics cPad driver functions.
 *
 *  copyright (c) Rob Miller Nov 2002 -- April 2003
 *
 */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "ucpad.h"

/*
 * the 1335 LCD controller and the RAM which seems to be present on the cPad makes me think
 * that we should be able to have 2 graphics layers and one layer of 1335 character generated
 * text, which the 1335 mixes for us on demand.  unfortunately, I can't seem to get the 
 * initialisation right for this, so the driver is currently stuck with the single layer 
 * supported by the default initialisation.  I am optimistic, though, so for the time being
 * this program works with 3 layers and XORs them together similar to what the 1335 should do.
 * sorry, I can't be bothered to work out the character generation (should be some easy routines 
 * available in the console code?)
 *
 * thus the data for 1 image on the cPad is referred to as a 'layer'
 */

unsigned char layer0[IMG_SIZE],layer1[IMG_SIZE],layer2[IMG_SIZE],layer3[IMG_SIZE];

// buffer for reading messages back from the cPad, which seems to be required for 
// every command sent.

#define RLEN 32
unsigned char readval[RLEN]= {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };

// read from the cPad 
void read_chk(int fd) {
  read(fd,readval,RLEN);
}

// debug routine to show whatever came back 
void prt_read() {
  int i=0;
  if ( readval[0] != 0 ) {
    fprintf(stderr,"cpad: "); for (i=0;i<RLEN;i++) fprintf(stderr,"%x ",readval[i]); fprintf(stderr,"\n");
    for (i=0;i<RLEN;i++) readval[i]=0;
  }
}

// wipe an image layer
void clr_layer(uchar *img) {
  int i;
  for (i=0;i<IMG_SIZE;i++) *img++ = 0;
}

// debug routine to see what the image layer looks like in ascii
void prtImg(uchar *img) {
  int i,j;
  for (j=0;j<160;j++) {
    for (i=0;i<30;i++) {
      fprintf(stderr,"%02x ",*img++);
    }
    fprintf(stderr,"\n");
  }
}


/***************************************************************************/
/** routines to read xpixmaps **/

// text color names we will show as black 
#define BCOLS 2
char * blacks[BCOLS] = {
  "black",
  "blue"
};

// map text color names to bit
int mapcol(char *col) {
  int i;
  for (i=0;i<BCOLS;i++) 
    if (!strcmp(col,blacks[i])) 
      return 0x80;
  return 0;
}

// read the color map section of an xpixmap -- text color names or hex values

int load_cmap(FILE *infil, int c, char *cmapc, int *cmapv) {
  char col[32]={0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0 };
  char mode='x';
  int i;
  unsigned int v;

  for (i=0;i<c;i++) {
    char *tail;
    fscanf(infil,"\"%c %c %s\n",&(cmapc[i]),&mode,col);
    tail = strchr(&(col[0]),'\"'); 
    if (tail != (char *) 0) *tail = '\0';
    //fprintf(stderr,"%d: .%c. mode %c .%s.\n",i,cmapc[i],mode,col);

    switch (mode) {
    case 'g':
      if (col[0] == '#') {
	sscanf(col,"#%x",&v);
	cmapv[i] = ( v > 0x777777 ? 0 : 0x80 );
	//fprintf(stderr,".%c. v: %x -> %x\n",cmapc[i],v,cmapv[i]);
      } else if (!strcmp(col,"None")) {
	cmapv[i] = 0;
      } else 
	goto FAIL;
      break;
    case 'c': 
      cmapv[i] = mapcol(col);
      break;
    default:
      goto FAIL;
    }

    //    fprintf(stderr,".%c. -> %x\n",cmapc[i],cmapv[i]);
  }

  return 1;

 FAIL:
  return 0;
}
  
// read an xpixmap file into an image layer
int xpm2img(char *fname, uchar *img) {
  FILE *infil=0;
  int x,y,c,foo,rv;
  char *linebuf=0,*cmapc=0,*lndx=0;
  int *cmapv=0;
  char name[64];
  int i;
  int rtn=0;

  if ((infil = fopen(fname,"r")) == (FILE *) NULL) {
    perror(fname);
    return(rtn);
  } 

  if ((fscanf(infil,"/* XPM */\n")) == EOF) goto FAIL;

  rv = fscanf(infil,"static char * %s = {\n",name);

  if (rv != 1) goto FAIL;
  
  fscanf(infil,"/* columns rows colors chars-per-pixel */\n"); // optional

  rv = fscanf(infil,"\"%d %d %d %d\",\n",&x,&y,&c,&foo);

  if (rv != 4) goto FAIL;

  if (x != 240) goto FAIL;

  if (y != 160) goto FAIL;


  if ((linebuf = (char *) malloc((x+5) * sizeof(char))) == (char *) 0) 
    goto FAIL;
  if ((cmapc = (char *) malloc(c * sizeof(char))) == (char *) 0) 
    goto FAIL;
  if ((cmapv = (int *) malloc(c * sizeof(int))) == (int *) 0) 
    goto FAIL;

  if (!load_cmap(infil,c,cmapc,cmapv)) 
    goto FAIL;

  fscanf(infil,"/* pixels */\n");  // optional

    /* read the image data */
  for (i=0;i<y;i++) {
    int j,k,s;
    //char *tail;

    fgets(linebuf,x+5,infil);

    lndx= &(linebuf[1]); // skip first \"
    //tail = strchr(&(linebuf[1]),'\"'); 
    //if (tail != (char *) 0) *tail = '\0'; // wipe second \"

    for (j=0;j<x;j++) {
      char ch;
      int v=0;
      ch = lndx[j];
      for (k=0;k<c;k++) {
	if (ch == cmapc[k]) {
	  v = cmapv[k];
	  k=c;
	}
      }
      
      s = j%8;
      *img |= (v>>s);
      //fprintf(stderr,"j %d(%d) s %d v %02x c %c i: %02x\n",j,(j/8),s,v,c,*img);
      if (s == 7) img++;
    }
  }
      
  
  rtn=1;

 FAIL:

    free(cmapv);
    free(cmapc);
    free(linebuf);
    fclose(infil);
    return rtn;
}

/** end of xpixmap routines **/
/***************************************************************************/
/** line drawing routines **/

// draw line 
void bresLine(int x1, int y1, int x2, int y2, unsigned char *ws, int draw) {
        int dx,dy,xstep,ystep,i,tx,ty,x,y,bw8,bh8;
        unsigned int adx,ady,start,stop;
	/*
        x1 -= BmpOffX;
        x2 -= BmpOffX;
        y1 -= BmpOffY;
        y2 -= BmpOffY;
	*/

        if (x1 < 0 && x2 < 0) return;
        if (y1 < 0 && y2 < 0) return;
        if (x1 > IMGBITWID && x2 > IMGBITWID) return;
        if (y1 > IMGLINES && y2 > IMGLINES) return;

        dx = x2-x1;
        dy = y2-y1;
        adx = abs(dx);
        ady = abs(dy);

        if (adx > ady) {
                xstep = 1<<8;
                if (dx) ystep = (dy << 8)/dx;
                else ystep = (dy << 8);
                if (dx>=0) {
                        x=x1; y=y1;
                        start = x1;
                        stop = x2;
                } else {
                        x=x2; y=y2;
                        start = x2;
                        stop = x1;
                }
        } else {
                ystep = 1<<8;
                if (dy) xstep = (dx << 8)/dy;
                else  xstep = (dx << 8);
                if (dy>=0) {
                        x=x1; y=y1;
                        start = y1;
                        stop = y2;
                } else {
                        x=x2; y=y2;
                        start = y2;
                        stop = y1;
                }
        }
        bw8 = (IMGBITWID-1) << 8;
        bh8 = (IMGLINES-1) << 8;
        if (draw) {
         for (i=start, tx = x<<8, ty = y<<8;
          i<=stop;
          i++, tx += xstep,ty += ystep) {
            if (tx < 0 || ty < 0) continue;
            if (tx > bw8 || ty > bh8) continue;
            x = (tx & (1<<7) ? (tx>>8) +1 : tx>>8);
            y = (ty & (1<<7) ? (ty>>8) +1 : ty>>8);
/*
StrPrintF(dbgp,"x %d  y %d         ",x,y);
WinDrawChars(dbgp,StrLen(dbgp),20,90);
*/
            *(ws+(y*IMGLLEN)+(x/8)) |= (0x80 >> (x % 8));
         }
        } else {
         for (i=start, tx = x<<8, ty = y<<8;
          i<=stop;
          i++, tx += xstep,ty += ystep) {
            if (tx < 0 || ty < 0) continue;
            if (tx > bw8 || ty > bh8) continue;
            x = (tx & (1<<7) ? (tx>>8) +1 : tx>>8);
            y = (ty & (1<<7) ? (ty>>8) +1 : ty>>8);
/*
StrPrintF(dbgp,"x %d  y %d         ",x,y);
WinDrawChars(dbgp,StrLen(dbgp),20,90);
*/
            *(ws+(y*IMGLLEN)+(x/8)) &= ~(0x80 >> (x % 8));
         }
        }
}

// draw a vertical bar at bit offset x*8.  y=0 is at bottom
void drawVBar(int x, int y, unsigned char mask, int draw, unsigned char *ws) {
  int i;

  if (y>IMGLINES) y = IMGLINES;
  if (draw) {
    for(i=0; i<y; i++) 
      *(ws + (((IMGLINES-1) -i)*IMGLLEN) + x) |= mask;
  } else {
    for(i=0; i<y; i++) 
      *(ws + (((IMGLINES-1) -i)*IMGLLEN) + x) &= ~mask;
  }

}

/** end of line drawing routines **/
/***************************************************************************/
/** support for horiz line and vertical bar graphs **/

// left shift and load;
// seems ugly but simpler than circ buffer and cache-able
void lshiftl_graph(short *g,short v) {
  int i;
  for (i=0;i<IMGBITWID-1;i++) g[i]=g[i+1];
  g[i]=v;
}

void clr_graph(short *g) {
  int i;
  for (i=0;i<IMGBITWID;i++) g[i] = 0;
}

/** end of graph support routines **/
/***************************************************************************/
/** read and graph system data **/

short graph0[IMGBITWID];
short bar0,bar1,bar2;

int scan_loadavg(short *major, short *minor) {
  FILE *pla=0;
  int retval=0;
  int imaj=0,imin=0;

  if ((pla=fopen("/proc/loadavg","r")) != (FILE *) NULL) {
    retval = fscanf(pla,"%2d.%2d ",&imaj, &imin); 
    *major = (short) imaj;
    *minor = (short) imin;
    //fprintf(stderr,"l:%d.%d\n",*major,*minor);
    fclose(pla);
    return retval;
  }
  return 0;
}

int scan_temp(short *temp) {
  FILE *pattt=0;
  int retval=0;
  int itemp=0;
  if ((pattt=fopen("/proc/acpi/thermal_zone/THRM/temperature","r")) != (FILE *) NULL) {
    retval = fscanf(pattt,"temperature: %d C ",&itemp);
    *temp = itemp;
    //fprintf(stderr,"t:%d\n",*temp);
    fclose(pattt);
    return retval;
  }
  return 0;
}

int scan_batinfo(int *full) {
  FILE *pabbi=0;
  int retval=0;
  char buf[64];
  if ((pabbi=fopen("/proc/acpi/battery/BAT1/info","r")) != (FILE *) NULL) {
    fgets(buf,64,pabbi);  //present
    fgets(buf,64,pabbi);  //design cap
    retval = fscanf(pabbi,"last full capacity: %d mWh ",full);
    //fprintf(stderr,"lf:%d\n",*full);
    fclose(pabbi);
    return retval;
  }
  return 0;
  
}

int scan_batstate(int *remain) {
  FILE *pabbs=0;
  int retval=0;
  char buf[64];
  int i;
  if ((pabbs=fopen("/proc/acpi/battery/BAT1/state","r")) != (FILE *) NULL) {
    //present, cap state, chg state, present rate
    for (i=0;i<4;i++) fgets(buf,64,pabbs);  

    retval = fscanf(pabbs,"remaining capacity: %d mWh ",remain);
    //fprintf(stderr,"r:%d\n",*remain);
    fclose(pabbs);
    return retval;
  }
  return 0;
  
}

#define BAT_HORIZ  28
#define TEMP_HORIZ 27
#define LD_HORIZ   26

#define BAR 0xf0

#define BAT_FLASH_PCT 20
#define TEMP_FLASH 68

short maj,min,temp;
int full,remain;
short bat;
void do_graphs() {
  //short maj=0,min=0;
  int i;

  drawVBar(LD_HORIZ,maj*10,BAR,0,layer3);  // cpu full load units
  drawVBar(TEMP_HORIZ,temp,BAR,0,layer3);  // temperature
  drawVBar(BAT_HORIZ,bat,BAR,0,layer3);    // battery %

  if (scan_loadavg(&maj,&min)) {
    //fprintf(stderr,"major %d minor %d\n",maj,min);
    
    for (i=1;i<IMGBITWID;i++)
      bresLine(i-1,IMGLINES-graph0[i-1],i,IMGLINES-graph0[i],layer2,0);

    lshiftl_graph(graph0,min);
    
    for (i=1;i<IMGBITWID;i++)
      bresLine(i-1,IMGLINES-graph0[i-1],i,IMGLINES-graph0[i],layer2,1);

    drawVBar(LD_HORIZ,maj*10,BAR,1,layer3);
  }

  if (scan_temp(&temp)) {
    drawVBar(TEMP_HORIZ,temp,BAR,1,layer3);
  }

  if (scan_batstate(&remain)) {
    bat = (short) (100.0 * ( ((float)remain)/((float)full) ));
    drawVBar(BAT_HORIZ,bat,BAR,1,layer3);
  }

}


/** end of sys data graph routines **/
/***************************************************************************/
/** wrappers for specific 1335 LCD controller actions **/

/*
 * write a command to the cPad 1335 lcd controller
 */
int write_1335(int fd, unsigned char fn, unsigned char *buf, int blen) {
  unsigned char msg_1335[IMGLLEN+1];
  int i,j;

  msg_1335[0] = fn;  // the command -- see the 1335 datasheet

  for (i=0,j=1; i<blen && j<IMGLLEN+1;) 
    msg_1335[j++] = buf[i++];
  //fprintf(stderr,"w: blen %d ilen %d i %d j %d\n",blen,IMGLLEN+1,i,j);

  /*
  for (i=0,j=1; i<blen && j<IMGLLEN+1;i++,j++) 
    fprintf(stderr,"%02x ",msg_1335[j]);
  fprintf(stderr,"\n");
  */
  return ( (write(fd,msg_1335,j))-1 );  // return the length of the param buffer written

}

/*
 * write a 240 pixel by 160 line image via the 1335 LCD controller, starting 
 * from the current cursor position.  uses the driver 'file i/o' mechanism.
 */
void write_1335_img(int fd, unsigned char *img, int len) {
  int c=0,blen=0;

  while (c<len) {
    if ((blen = len-c) > IMGLLEN) blen = IMGLLEN; 
    
    c += ( (write_1335(fd, MWRITE_1335, &(img[c]), blen)) );

    // if (rv != i) fprintf(stderr,"%d/%d wrote %d bytes not %d\n",c,len,rv,i);
    //usleep(USTIME);

    read(fd,readval,0); 
  }
}

/*
 * set the lcd controller cursor to the 16 bit address specified
 */
void set_1335_cursor(int fd, unsigned char csrl, unsigned char csrh) {
  unsigned char csr[2] = { csrl, csrh };
  write_1335(fd,CSRW_1335,csr,2);

  read_chk(fd);
}

/*
//grotty hack because the flash ioctl wasn't robust  (not used)
char lstate;
static void hack_flash(int fd,int delay) {
  lstate= (char) 1;
  if (ioctl(fd,CPAD_WLIGHT, (char *) &lstate)) {
    perror("cpad flash backlight on");
  }
  
  usleep((unsigned long) delay);
  lstate= (char) 0;

  if (ioctl(fd,CPAD_WLIGHT, (char *) &lstate)) {
    perror("cpad flash backlight off");
  }

}
*/

/** end of 1335 wrappers **/
/***************************************************************************/
/** main() stuff **/

void die_usage(char *me) {
  fprintf(stdout,"usage: %s -<opt> <param> [<dev>]\n",me);
  fprintf(stdout,"  -i <file>    display 160(vert) by 240(horiz) xpm file\n");
  fprintf(stdout,"  -b <1,0>     backlight on/off\n");
  fprintf(stdout,"  -c           read backlight state\n");
  fprintf(stdout,"  -l <1,0>     lcd on/off\n");
  fprintf(stdout,"  -m           read lcd state\n");
  fprintf(stdout,"  -r           reset\n");
  fprintf(stdout,"  -v           print driver version\n");
  fprintf(stdout,"  -x <n>       backlight flash n * 10msec\n");
  fprintf(stdout,"  -s <n>       set minimum touch pressure\n");
  fprintf(stdout,"  -t <n>       set mouse motion sensitivity\n");
  fprintf(stdout,"  -d <n>       display load/temp/batt graphs every n secs\n");
  exit(1);
}

#define DEVDFLT "/dev/usb/cpad0"

int main(int argc, char **argv) {
  char *dev=DEVDFLT;
  int i;
  int fd=-1;
  char **avbak;
  int param=0;
  int delay=0;
  //int flash_time=20;

  avbak = argv;

  // fprintf(stderr,"av: %s\n",*argv);

  //if (argc < 3) die_usage(*avbak);

  // parse cmd line twice to find device if present
  /* anything we can do 1st time around */
  for (i=1;i<argc;i++) {
    argv++;
    if (*argv[0] == '-') {
      switch ((*argv)[1]) {
      case 'h':
      case '?':
	die_usage(*avbak);
	break;
      case 'r':
      case 'c':
      case 'm':
      case 'v':  /* no parameter */
	break;
      default:
	if (((i+1) < argc) && (argv[1][0] != '-')) {
	  i++; argv++;  /* skip parameter as well */
	}
	break;
      }
    } else {
      /* but this is what's needed if present */
      dev = *argv;
      fprintf(stderr,"device %s",dev);
    }
  }

  if ((fd = open(dev, O_RDWR)) < 0) {
    perror("cpad dev open");
    exit(1);
  }

  argv = avbak;

  /* now do it for real */

  for (i=1;i<argc;i++) {
    argv++;
    if (*argv[0] == '-') {
      switch ((*argv)[1]) {
      case 'i':
        argv++; i++;
	if (!xpm2img(*argv,layer1)) 
	  fprintf(stderr,"failed reading xpm %s\n",*argv);

	set_1335_cursor(fd,0x00,0x00);
       	//write_1335_img(fd,img_align,IMG_SIZE);
	write_1335_img(fd,layer1,IMG_SIZE);
	
	break;

      case 'b': 
        argv++; i++;
	sscanf(*argv,"%d",&param);
	if (ioctl(fd,CPAD_WLIGHT, (char *) &param)) {
	  perror("cpad write backlight");
	}
	//read_chk(fd);
	break;
	
      case 'c':
	if (ioctl(fd,CPAD_RLIGHT)) {
	  perror("cpad read backlight");
	}
	read_chk(fd);
	printf("%d\n",readval[2]);
	break;

      case 'd':
        argv++; i++;
	sscanf(*argv,"%d",&delay);
	break;

      case 'x': 
        argv++; i++;
	sscanf(*argv,"%d",&param);

	if (ioctl(fd,CPAD_FLASH, (int *) &param)) {
	  perror("cpad flash backlight");
	}
	break;
	
      case 's': 
        argv++; i++;
	sscanf(*argv,"%d",&param);

	if (ioctl(fd,CPAD_SET_SENS, (int *) &param)) {
	  perror("cpad set touch sensitivity");
	}
	break;
	
      case 't': 
        argv++; i++;
	sscanf(*argv,"%d",&param);
	if ((param < 0) || (param > 255)) {
	  fprintf(stderr,"parameter %d out of range\n",param);
	  break;
	}

	if (ioctl(fd,CPAD_SET_STROKE, (int *) &param)) {
	  perror("cpad set motion sensitivity");
	}
	break;
	
      case 'l':
        argv++; i++;
	sscanf(*argv,"%d",&param);
	if (ioctl(fd,CPAD_WLCD,(char *) &param)) {
	  perror("cpad write lcd");
	}
	//read_chk(fd);
	break;

      case 'm':
	if (ioctl(fd,CPAD_RLCD)) {
	  perror("cpad read lcd");
	}
	read_chk(fd);
	printf("%d\n",readval[2]);
	break;

      case 'v': {
	int rv=0;
	
	if (ioctl(fd,CPAD_VERSION,&rv)) {
	  perror("ioctl cpad version");
	}

	fprintf(stdout,"cpad driver version %d\n",rv);
	break;
      }
	
      case 'r': 
	if (ioctl(fd,CPAD_RESET)) {
	  perror("ioctl cpad reset");
	}

	break;

      default:

	fprintf(stderr,"unknown option -%c \n",*argv[1]);
	if ((i < (argc-1)) && (argv[1][0] != '-')) {
	  i++; argv++;  /* skip parameter as well */
	}
	break;
      }

    } else {
      // skip the device this time
    }
  }

  close(fd);

  if (delay) {
    clr_layer(layer0);
    //clr_layer(layer1);
    clr_layer(layer2);
    clr_layer(layer3);
    clr_graph(graph0);

    maj=0;
    min=0;
    temp=0;
    remain=0;
    full=38880;
    bat=0;

    scan_batinfo(&full);

    while (delay) {
      int i=0;
      int flash_time=30;

      do_graphs();

      for(i=0;i<IMG_SIZE;i++) {
	layer0[i] = (layer1[i] ^ layer2[i]) ^ layer3[i];
      }

      if ((fd = open(dev, O_RDWR)) < 0) {
	perror("cpad dev open");
	//exit(1);
      } else {
	set_1335_cursor(fd,0x00,0x00);
	write_1335_img(fd,layer0,IMG_SIZE);
	if (bat < BAT_FLASH_PCT) {
	  if (ioctl(fd,CPAD_FLASH, (int *) &flash_time)) {
	    perror("cpad flash backlight");
	  }
	}
	//fprintf(stderr,"bat: %d %%\n",bat);
	
	if (temp > TEMP_FLASH) {
	  if (ioctl(fd,CPAD_FLASH, (int *) &flash_time)) {
	    perror("cpad flash backlight");
	  }
	  //fprintf(stderr,"temp: %d C\n",temp);
	}
	
	close(fd);
      }
      sleep(delay);

      //fprintf(stderr,".");
    }
  }

  //  close(fd);
  exit(0);

}
