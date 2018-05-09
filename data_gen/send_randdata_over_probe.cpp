/**
 * @author: Gursimran Singh
 * @github: https://github.com/gursimransinghhanspal
 *
 * This script creates random data of specified bytes and uses the probe stuffing script to send data.
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <getopt.h>

static char *ARG_IFACE = NULL;
static int ARG_BYTES = 200;

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
				{"iface", required_argument, 0, 'i'},
				{"bytes", required_argument, 0, 'b'},
				{0,      0,                 0, 0}
		};

		/* getopt_long stores the option index here. */
		int option_index = 0;
		int opt_long;
		opt_long = getopt_long(argc, argv, "i:b:", long_options, &option_index);

		/* Detect the end of the options. */
		if (opt_long == -1)
			break;

		switch (opt_long) {
			case 0:
				/* If this option set a flag, do nothing else now. */
				if (long_options[option_index].flag != 0)
					break;
				break;

			case 'i':
				ARG_IFACE = (char *) calloc(strlen(optarg), sizeof(char));
				strcpy(ARG_IFACE, optarg);
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

	if (!ARG_IFACE || strcmp(ARG_IFACE, "") == 0) {
		error("Argument `--iface (-i)' required!");
	} else {
		printf("Argument `--iface (-i)': `%s'", ARG_IFACE);
		printf("\n");
	}
	printf("\n");

	char *randstr = gen_rand_string();
	char *command = (char *) calloc(ARG_BYTES + 100, sizeof(char));
	snprintf(command, ARG_BYTES + 100, "probe.out -i%s -d\"%s\"", ARG_IFACE, randstr);
	printf("%s\n", command);
	// system(command);
	return 0;
}