/*
 *
 * battmon.c
 *
 *  monitor battery status, dispatch /usr/local/sbin/hibernate 
 *  when there are 2 successive, declining capacity reads below
 *  the alarm level.  optionally switch to performance state P1 
 *  when AC goes off-line, but note that current SWSUSP+ACPI
 *  (25 Apr 03) does not detect/update the AC status upon resume
 *  (so if you hibernate, plug-in, and resume it will still think
 *  the AC is off-line until you re-plug).
 *
 * **  note this needs to be re-started not left running through a resume **
 *
 *  Copyright (C) 2003 Robert T. Miller
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>

#define BATTFILE "/proc/acpi/battery/BAT1/state"
#define ACFILE "/proc/acpi/ac_adapter/ADP1/state"
#define PERFFILE "/proc/acpi/processor/CPU0/performance"
#define BALARMFILE "/proc/acpi/battery/BAT1/alarm"

#define TBUFSIZ 64
#define DFLTSLP 20

char * hib_cmd[] =  { "/usr/local/sbin/hibernate", "--force", "--kill", (char *) NULL };
int verbose=0;
int testmode=0;
char *myname=0;
char buf[TBUFSIZ];

void usage(char *myname) {
  printf("usage: %s [options]\n",myname);
  printf(" options:\n");
  printf("  -v         verbose\n");
  printf("  -h         this help\n");
  printf("  -l         write power status changes to syslog\n");
  printf("  -s <n>     sleep <n> seconds per loop (default %d)\n",DFLTSLP);
  printf("  -f         remain in foreground\n");
  printf("  -p         reduce performance when on battery\n");
  printf("  -n         no suspend - don't actually suspend\n");
  printf("  -t         test - suspend first time through\n");
}


int main(int argc, char **argv) {
  FILE *battf=0;
  FILE *ACf=0;
  FILE *baf=0;
  int sleeptime=DFLTSLP;
  int logmode=0;
  int l_ac=0;
  int l_remaining=0;
  int foreground=0;
  int perfmode=0;
  char l_batt_state[16];
  int batt_alarm=0;
  int no_suspend=0;

  l_batt_state[0] = '\0';

  myname=*argv;

  while (--argc) {
    argv++;
    if (*argv[0] == '-') {
      switch ((*argv)[1]) {
      case 'v': 
	verbose=1;
	break;
      case 't': 
	testmode=1;
	break;
      case 'n': 
	no_suspend=1;
	break;
      case 'l': 
	logmode=1;
	break;
      case 'p': 
	perfmode=1;
	break;
      case 'f': 
	foreground=1;
	break;
      case 's': 
	if (--argc) {
	  argv++;
	  sscanf(*argv,"%d",&sleeptime);
	  if (verbose) printf("sleep delay = %d seconds\n",sleeptime);
	} else {
	  printf("-s option needs number of seconds to sleep\n");
	  usage(myname);
	  exit(-1);
	}
	break;
      case '?':
      case 'h':
	usage(myname);
	exit(-1);
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

  if (!foreground)
    if (fork()) exit(0);

  if ((baf = fopen(BALARMFILE,"r")) == (FILE *) NULL) {
    sleep(15);
    if ((baf = fopen(BALARMFILE,"r")) == (FILE *) NULL) {
      sprintf(buf,"unable to open %s for reading",BALARMFILE);
      perror(buf);
      exit(-1);
    }
  } 
  
  if (! fscanf(baf,"alarm: %d mWh",&batt_alarm)) {
    sleep(15);
    if (! fscanf(baf,"alarm: %d mWh\n",&batt_alarm)) {
      fprintf(stderr,"%s 'alarm:' read failed - format changed?\n",BALARMFILE);
      exit(-1);
    }
  }
  
  fclose(baf);

  if (verbose) printf("battery alarm set at %d mWh\n",batt_alarm);

  while (1) {
    char ac_state[9];
    int ac=0;
    char batt_state[16];
    int voltage,remaining;

    if ((ACf = fopen(ACFILE,"r")) == (FILE *) NULL) {
      sleep(15);
      if ((ACf = fopen(ACFILE,"r")) == (FILE *) NULL) {
	sprintf(buf,"unable to open %s for reading",ACFILE);
	perror(buf);
	exit(-1);
      }
    } 

    if (! fscanf(ACf,"state: %s\n",ac_state)) {
      sleep(15);
      if (! fscanf(ACf,"state: %s\n",ac_state)) {
	fprintf(stderr,"%s 'state:' read failed - format changed?\n",ACFILE);
	exit(-1);
      }
    }
    
    fclose(ACf);
    
    if (!(strcmp(ac_state,"on-line"))) {
      ac=1;
    } 

    if ((battf = fopen(BATTFILE,"r")) == (FILE *) NULL) {
      sleep(15);
      if ((battf = fopen(BATTFILE,"r")) == (FILE *) NULL) {
	sprintf(buf,"unable to open %s for reading",BATTFILE);
	perror(buf);
	exit(-1);
      }
    } 
    
    batt_state[0] = '\0';
    remaining=0;
    voltage=0;

    while(fgets(buf,BUFSIZ,battf)) {
      if (!batt_state[0] && sscanf(buf,"capacity state: %s",batt_state)) continue;
      if (!remaining && sscanf(buf,"remaining capacity: %d",&remaining)) continue;
      if (!voltage && sscanf(buf,"present voltage: %d",&voltage)) continue;
    }
    
    fclose(battf);
    
    if (l_ac != ac) {

      if (logmode) {
	openlog(myname,LOG_NDELAY,LOG_USER);
	syslog(LOG_ALERT,"AC %s battery %s V= %d mV cap= %d mWh\n",ac_state,batt_state,voltage,remaining);
	closelog();
      }

      if (perfmode) {
	FILE *pfile=0;
	if ((pfile = (fopen(PERFFILE,"w"))) == (FILE *) NULL) {
	  sleep(15);
	  if ((pfile = (fopen(PERFFILE,"w"))) == (FILE *) NULL) {
	    sprintf(buf,"unable to open %s for writing",PERFFILE);
	    perror(buf);
	    exit(-1);
	  }
	}
	if (ac)
	  fprintf(pfile,"0");
	else
	  fprintf(pfile,"1");

	fclose(pfile);
      }
    }

    if (verbose)
      printf("ac %d battery state %s remaining %d mWh  voltage %d mV\n",
	   ac,batt_state,remaining,voltage);
    
    if (testmode 
	|| 
	// this seems to be only relevant and valid test after a resume
	// note this needs to be re-started not left running through a resume
	((remaining <= batt_alarm) && (remaining < l_remaining))
	) {

      if (verbose) printf("battery state critical and no AC!\n");
      openlog(myname,LOG_NDELAY,LOG_USER);
      syslog(LOG_ALERT,"battery below alarm level and declining, V= %d mV cap= %d mWh\n",voltage,remaining);
      closelog();

      if (!no_suspend) 
	if (! fork()) 
	  execv(hib_cmd[0],hib_cmd);

      if (testmode) 
	exit(0);

      sleep(120);

    }

    l_ac = ac;
    strcpy(l_batt_state,batt_state);
    l_remaining = remaining;

    sleep(sleeptime);
  }
}
