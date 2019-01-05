#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "Defs.h"
#include "TsParser.h"
#include "Utils.h"


char g_optFile [PATH_MAX] = {0};


uint32_t getOption (int argc, char* argv[])
{
	uint32_t r_optFlag = 0x00000000;
	int opt = 0;
    
	// disable getopt err msg
	opterr = 0;
    
	while ((opt = getopt(argc, argv, "f:")) != -1) {
		switch (opt) {
		case 'f':
printf("optarg %s\n", optarg);
			r_optFlag |= 0x00000001;
			memset (g_optFile, 0x00, PATH_MAX);
			strncpy (g_optFile, optarg, PATH_MAX -1);
			break;

		default:
			break;
		}
	}
    
    return r_optFlag;
}

int main (int argc, char *argv[])
{
	int fd = 0;
	int readSize = 0;
	int readTotal = 0;
	uint8_t buff [65536] = {0};


	CTsParser tp;


	uint32_t optFlag = getOption (argc, argv);
	if (optFlag & 0x00000001) {
		fd = open (g_optFile, O_RDONLY);
		if (fd < 0) {
			perror ("open");
			exit (EXIT_FAILURE);
		}
	} else {
		fd = STDIN_FILENO;
	}


	while (1) {

		memset (buff, 0x00, sizeof(buff));
		readSize = CUtils::readFile (fd, buff, sizeof(buff));
		if (readSize < 0) {
			fprintf (stdout, "CUtils::readFile() is failure.\n");
		} else if (readSize == 0) {
			break;
		}

		readTotal += readSize;

printf ("%d\n", readSize);

		tp.run (buff, readSize);
	}


	printf ("readTotal %d\n", readTotal);

	close (fd);
	exit (EXIT_SUCCESS);
}
