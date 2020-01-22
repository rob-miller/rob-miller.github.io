/*
 *
 * tkeymon.c
 *
 *  monitor button/key presses on /proc/acpi/toshiba/keys, dispatch
 * programs according to config file.
 *
 */

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#define DFLTCONFIG "/etc/tkeymon.conf"
#define MAXCMD 256
#define MAXTOK 64
#define BUFLEN 512
#define KEYFILE "/proc/acpi/toshiba/keys"

typedef struct BtnCmd {
  struct BtnCmd *next;
  int keycode;
  int count;
  char mode;
  char cmd[MAXCMD];
  char *argv[MAXTOK];
} btncmd;

int verbose=0;
char *myname=0;
char buf[BUFLEN];


void usage(char *myname) {
  printf("usage: %s [options]\n",myname);
  printf(" options:\n");
  printf("  -v         verbose\n");
  printf("  -h         help\n");
  printf("  -f <file>  use specified config file\n");
}

void help() {
  printf(" default config file: %s\n",DFLTCONFIG);
  printf(" config file lines are either\n");
  printf("# (in column 0) = comment line\n");
  printf("<hex keycode> <mode> <command>\n");
  printf(" where\n");
  printf("  <hex keycode> is found in %s after button press\n",KEYFILE);
  printf("  <mode> is \n");
  printf("     a      always execute the command\n");
  printf("     t      toggle (execute every other time, only 2 per button)\n");
  printf("   <count>  execute command for only <count> button presses\n");
  printf(" order matters, the first rule to match is the one applied\n");
}
  
btncmd * getBC() {
  btncmd *bc=0;
  int i;

  if ((bc = (btncmd *) malloc(sizeof(btncmd))) == (btncmd *) 0) {
    printf("out of memory!\n");
    exit(-1);
  }

  bc->next = (btncmd *) 0;
  bc->keycode = 0;
  bc->count = 0;
  bc-> mode =0;
  bc->cmd[0] = '\0';
  for (i=0;i<MAXTOK;i++) {
    bc->argv[i] = (char *) 0;
  }

  return(bc);
}


btncmd * parseConfigFile(char *cfname) {
  FILE *cfile=0;
  int cfLineNo=0;
  btncmd *bc=0,*rbc;
  int i;

  if ((cfile = fopen(cfname,"r")) == (FILE *) NULL) {
    sprintf(buf,"error opening config file %s",cfname);
    perror(buf);
    usage(myname);
    exit(-1);
  }
  if (verbose) 
    printf("reading config file %s\n",cfname);

  rbc = getBC();

  while (fgets(buf,BUFLEN,cfile) != NULL) {
    int keycode,count=0;
    char mode=0;
    char *ndx=0,*ndx2=0;

    cfLineNo++;
    buf[strlen(buf)-1] = '\0';  // wipe the \n

    if (buf[0] == '#') continue;
    if (! strlen(buf)) continue;

    if ((sscanf(buf,"%i ",&keycode)) != 1) {
      printf("%s line %d, error reading keycode\n",cfname,cfLineNo);
      printf(" %s\n",buf);
      exit(-1);
    }
    
    if((ndx = strchr(buf,' ')) == (char *) NULL) {
      printf("%s line %d, error reading past keycode\n",cfname,cfLineNo);
      printf(" %s\n",buf);
      exit(-1);
    }
    ndx++;
    if((ndx2 = strchr(ndx,' ')) == (char *) NULL) {
      printf("%s line %d, error reading past mode\n",cfname,cfLineNo);
      printf(" %s\n",buf);
      exit(-1);
    }
    ndx2++;

    if (! strlen(ndx2)) {
      printf("%s line %d, no command found\n",cfname,cfLineNo);
      printf(" %s\n",buf);
      exit(-1);
    }

    if ((sscanf(ndx,"%d ",&count)) == 1) {
      if (verbose) 
	printf("%s line %d key %x count = %d command %s\n",
	       cfname,cfLineNo,keycode,count,ndx2);
    } else if ((sscanf(ndx,"%c ",&mode)) == 1) {
      switch (mode) {
      case 'a':
      case 't':
	break;
      default:
	printf("%s line %d, mode %c not recognised\n",cfname,cfLineNo,mode);
	printf(" %s\n",buf);
	printf("try %s -h\n",myname);
	exit(-1);
	break;
      }

      count++;

      if (verbose) 
	printf("%s line %d key %x mode = %c command %s\n",
	       cfname,cfLineNo,keycode,mode,ndx2);
    }

    if (bc) {
      bc->next = getBC();
      bc = bc->next;
    } else {
      bc = rbc;
    }

    bc->keycode = keycode;
    bc->count = count;
    bc->mode = mode;
    sprintf(bc->cmd,ndx2);
    i=0;
    //    bc->argv[i++] = &(bc->cmd[0]);
    bc->argv[i] = strtok(bc->cmd," ");
    while (bc->argv[i]) bc->argv[++i] = strtok((char *)NULL," ");
    
  }

  return(rbc);

}

void doCmd(int kcode, btncmd *bc) {
  int done=0;
  int fork_rslt=0;

  clearenv();

  while (bc != (btncmd *) 0) {
    if (bc->keycode == kcode) {
      if (!done && bc->count) {
	if (verbose) {
	  int i=0;
	  printf("execute command: ");
	  
	  while (bc->argv[i]) {
	    printf("%s ",bc->argv[i]);
	    i++;
	  }
	  printf("\n");
	}

	fork_rslt = fork();

	if ( fork_rslt > 0 ) {
	  // I am the child process
	  if (daemon(0,0) >= 0) 
	    execv(bc->argv[0],bc->argv);
	  fprintf(stderr,"%s failed daemon() or execv()\n",myname);  // hope this shows up somewhere...
	  exit(0);  // there was an error
	} else if (fork_rslt < 0) 
	  fprintf(stderr,"%s fork() failed %d\n",myname,fork_rslt);  // hope this shows up somewhere...
	
	done=1;
	switch(bc->mode) {
	case 0:   // count only
	case 't': // toggle
	  bc->count--;
	  break;
	case 'a': // always
	  break;
	default:
	  printf("unrecognised mode %c how did we get here?\n",bc->mode);
	  exit(-1);
	}
      } else if (bc->mode == 't') {
	// this is why we must scan the entire list
	bc->count = 1;   // toggle back on 
      }
    }
    bc = bc->next;
  }
}


int main(int argc, char **argv) {
  FILE *keyf=0;
  char *cfname=DFLTCONFIG;
  btncmd *bcSet=0;


  myname=*argv;

  while (--argc) {
    argv++;
    if (*argv[0] == '-') {
      switch ((*argv)[1]) {
      case 'v': 
	verbose=1;
	break;
      case 'h':
	usage(myname);
	help();
	exit(-1);
	break;
      case 'f':
	if (argc < 1) {
	  printf("option -f requires a filename argument\n");
	  usage(myname);
	  exit(-1);
	}
	--argc;
	argv++;
	cfname = *argv;
	break;
      default:
	printf("unrecognised option %s\n",*argv);
	usage(myname);
	exit(-1);
	break;
      }
    } else {
      printf("unrecognised parameter %s\n",*argv);
      usage(myname);
      exit(-1);
    }
  }

  bcSet = parseConfigFile(cfname);

  if (!bcSet) {
    printf("no rules loaded from %s\n",cfname);
    exit(-1);
  }

  while (1) {
    int rval;
    int kcode;

    if ((keyf = fopen(KEYFILE,"r")) == (FILE *) NULL) {
      sprintf(buf,"unable to open %s for reading",KEYFILE);
      perror(buf);
      //exit(-1);
      sleep(5);
    } else {

      if (! fscanf(keyf,"hotkey_ready: %d\n",&rval)) {
	fprintf(stderr,"'hotkey_ready:' read failed - format changed?\n");
	//exit(-1);
	sleep(5);
      }
      if (! fscanf(keyf,"hotkey: %i\n",&kcode)) {
	fprintf(stderr,"'hotkey:' read failed - format changed?\n");
	//exit(-1);
	sleep(5);
      }

      fclose(keyf);

      if (rval) {
	if (verbose) 
	  printf("status %d  keycode %x\n",rval,kcode);

	if ((keyf = fopen(KEYFILE,"w")) == (FILE *) NULL) {
	  sprintf(buf,"unable to open %s for writing",KEYFILE);
	  perror(buf);
	  //exit(-1);
	  sleep(5);
	}
	fprintf(keyf,"hotkey_ready: 0\n");
	fclose(keyf);
	
	doCmd(kcode,bcSet);

      }
      usleep(333333);
    }
  }
}
