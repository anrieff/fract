/***************************************************************************
 *   Copyright (C) 2004 by Veselin Georgiev                                *
 *   vesko@ViruS                                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *-------------------------------------------------------------------------*
 *                                                                         *
 *   cmdline.cpp - parses the command line                                 *
 *                                                                         *
 ***************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "MyGlobal.h"
#include "MyTypes.h"
#include "cmdline.h"


cmdinfo *cmdroot = NULL;
char cmdnullterm[2] = "0";

void OptionAdd(char *argv)
{
	unsigned int i, found;
	static cmdinfo *p=NULL, *q;

	if (cmdroot==NULL) {
		p = (cmdinfo *) malloc(sizeof(cmdinfo));
		cmdroot = p;
		p->next = NULL;
		q = p;
	} else {
		q = (cmdinfo *) malloc(sizeof(cmdinfo));
		q->next = NULL;
		p->next = q;
		p = q;
	}
	found = 0;
	for (i=0;i<strlen(argv);i++) {
		if (argv[i]=='=') {
			p->data = (char *) malloc(i+1);
			strncpy(p->data, argv, i);
			p->data[i] = 0;
			i++;
			p->value = (char*) malloc(1+strlen(argv+i));
			strcpy(p->value, argv+i);
			found = 1;
			break;
		}
	}
	if (!found) {
		p->value = NULL;
		p->data = (char *) malloc(strlen(argv)+1);
		strcpy(p->data, argv);
	}
}

void initcmdline(int argc, char * argv[])
{
 	for (int j=1; j< argc; j++) {
		OptionAdd(argv[j]);
 	}
}
// int OptionExists
//    return 1 if the given string is found within the command line parameters
//    returns 0 otherwise
int OptionExists(char *opt)
{cmdinfo *p;
 p = cmdroot;
 while (p) {
 	if (strcmp(p->data, opt)==0)
		return 1;
 	p = p->next;
 	}
 return 0;
}

// searches for commandline parameters in the  --option=VALUE style. 
//If everything is in order it returns a pointer to that VALUE
char *OptionValueString(char *opt)
{cmdinfo *p;
 p = cmdroot;
 while (p) {
 	if (strcmp(p->data, opt)==0)
		if (p->value)
			return p->value;
			else
			return cmdnullterm;
	p = p->next;
 	}
 return cmdnullterm;
}

// same as above, except that the value is transformed to int
int OptionValueInt(char *opt)
{int z;
 if (!sscanf(OptionValueString(opt),"%d",&z))
 	z = 0;
 return z;
}

//... and the float version
float OptionValueFloat(char *opt)
{float f;
 if (!sscanf(OptionValueString(opt), "%f", &f))
 	f = 0.0;
 return f;
}

void DisplayUsage(void)
{
 printf("fract %s By Vesselin Georgiev\n", Fract_Version);
 printf("Usage: fract [options]\n");
 printf(" OPTIONS: \n");
 printf("  --scene=xxx     - load the given scene instead of the default one\n");
 printf("  --no-mem        - disable bigarr allocation\n");
 printf("  --force-overlay - use overlay video drawing mode\n");
 printf("                    even if it's not hardware accelerated\n");
 printf("  --no-overlay    - use surface drawing mode even\n");
 printf("                    if it's not hardware accelerated\n");
 printf("  --no-sse, --no-mmx, --no-mmx2	- disable usage of\n");
 printf("                     SSE, MMX, MMX2 respectively\n");
 printf("  --has-sse, --has-mmx, --has-mmx2	- force usage of\n");
 printf("                     SSE, MMX, MMX2 respectively\n");
 printf("  --force-mem   - use bigarr even if insufficient memory is reported by linux\n");
 printf("  --xres=xxx    - use custom video mode: xxx by yyy pixels, yyy = xxx * 3/4\n");
 printf("  --benchmark   - benchmark your machine by running the RGB2YUV \n");
 printf("                  functions for 20 seconds\n");
 printf("  --fullscreen  - run in fullscreen mode, if supported\n");
 printf("  --windowed    - run in a window, if supported\n");
 printf("  --fsaa=x      - Use selected Fullscreen Antialiasing. Invoke with\n");
 printf("                  --fsaa=? to see a list\n");
 printf("  --cpus=n      - Use the specified number of processors\n");
 printf("  --thread      - Use all (detected) processors in the system (default)\n");
 printf("  --no-thread   - Use only one processor\n");
 printf("  --shadows, --no-shadows - Enable/Disable shadows\n");
 printf("  --save-video  - Save a sequence of .BMPs to a video/ subfolder\n");

}

void cmdline_close(void)
{cmdinfo *p, *q;
// free some occupied memory
 p = cmdroot;
 while (p!=NULL) {
 	q = p->next;
	if (p->data)
		free(p->data);
	if (p->value)
		free(p->value);
	free(p);
	p = q;
 	}
}

/** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * @class ArgumentIterator                                                    *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
ArgumentIterator::ArgumentIterator()
{
	iter = cmdroot;
}

bool ArgumentIterator::ok() const
{
	return (iter != NULL);
}

const char *ArgumentIterator::value() const
{
	return iter->data;
}

void ArgumentIterator::operator++()
{
	iter=iter->next;
}
