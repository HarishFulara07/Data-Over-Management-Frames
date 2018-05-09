/**
 * @author: Gursimran Singh
 * @github: https://github.com/gursimransinghhanspal
 *
 * This script creates random data of specified bytes and uses the probe stuffing script to send data.
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <getopt.h>
#include <ctime>

static char *ARG_HOST = NULL;
static int ARG_BYTES = 200;
static int ARG_PORT = -1;
static

void
error(const char *msg) {
	perror(msg);
	exit(1);
}

char *gen_rand_string() {
	static const char alphanum[] =
			"0123456789"
			"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
			"abcdefghijklmnopqrstuvwxyz";

	srand(time(NULL));
	char *s = (char *) calloc(ARG_BYTES +1, sizeof(char));
	for (int i = 0; i < ARG_BYTES; ++i) {
		s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
	}
	s[ARG_BYTES] = '\0';
	return s;
}

int
main(int argc, char *argv[]) {

	// **** Command line argument parsing ****
	while (1) {
		char arg_conntype[5];

		/* Define the arguments */
		static struct option long_options[] = {
				{"host", required_argument, 0, 'h'},
				{"port", required_argument, 0, 'p'},
				{"bytes", required_argument, 0, 'b'},
				{0,      0,                 0, 0}
		};

		/* getopt_long stores the option index here. */
		int option_index = 0;
		int opt_long;
		opt_long = getopt_long(argc, argv, "h:p:b:", long_options, &option_index);

		/* Detect the end of the options. */
		if (opt_long == -1)
			break;

		switch (opt_long) {
			case 0:
				/* If this option set a flag, do nothing else now. */
				if (long_options[option_index].flag != 0)
					break;
				break;

			case 'h':
				ARG_HOST = (char *) calloc(strlen(optarg), sizeof(char));
				strcpy(ARG_HOST, optarg);
				break;

			case 'p':
				ARG_PORT = atoi(optarg);
				break;

			case 'b':
				ARG_BYTES = atoi(optarg);
				break;

			case '?':
				/* getopt_long already printed an error message. */
				exit(1);
				break;

			default:
				abort();
		}
	}

	printf("Argument `--bytes (-b)': `%d'", ARG_BYTES);
	printf("\n");

	if (ARG_PORT == -1) {
		error("Argument `--port (-p)' required!");
	} else {
		printf("Argument `--port (-p)': `%d'", ARG_PORT);
		printf("\n");
	}

	if (!ARG_HOST || strcmp(ARG_HOST, "") == 0) {
		error("Argument `--host (-h)' required!");
	} else {
		printf("Argument `--host (-h)': `%s'", ARG_HOST);
		printf("\n");
	}
	printf("\n");

	char *randstr = gen_rand_string();
	char *command = (char *) calloc(ARG_BYTES + 100, sizeof(char));
	snprintf(command, ARG_BYTES + 100, "tcpclient.out --tcp -h%s -p%d -s\"%s\"", ARG_HOST, ARG_PORT, randstr);
	printf("%s\n", command);
	// system(command);
	return 0;
}