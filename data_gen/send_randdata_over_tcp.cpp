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
#include <unistd.h>
#include <errno.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/time.h>

static char *ARG_HOST = NULL;
static int ARG_BYTES = 200;
static int ARG_PORT = -1;

void log_info(unsigned long long int data_origin_ts, char* data) {

	// Log file name = client's wifi interface name + data origin timstamp
	char log_file_path[50] = "logs/clienttcp_log.log";
	//strcat(log_file_path, wifi_interface);
	//strcat(log_file_path, "_");
	//strcat(log_file_path, data_origin_ts_str);
	//strcat(log_file_path, ".txt");

	FILE *file;
	file = fopen(log_file_path, "a");

	if (file == NULL) {
		printf("\nUnable to open '%s' file.\n", log_file_path);
		return;
	}

	fprintf(file, "Data Size: %d\n", strlen(data));
	fprintf(file, "Data Origin Timestamp: %018llu\n", data_origin_ts);
	fprintf(file, "Data: %s\n", data);
	fprintf(file, "\n");
	fclose(file);
}


void
error(const char *msg) {
	perror(msg);
	exit(1);
}

char *gen_rand_string() {
	static const char alphanum[] = "0123456789"
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

	// get data received timestamp
	struct timeval tv;
	gettimeofday(&tv, NULL);
	unsigned long long gen_ts = tv.tv_sec * (unsigned long long) 1000000 + tv.tv_usec;

	char *command = (char *) calloc(ARG_BYTES + 100, sizeof(char));
	snprintf(command, ARG_BYTES + 300, "/home/mate/Desktop/domf-code/bin/tcpclient.out --tcp -h%s -p%d -s\"NA,%018llu,0,0,0,0,%d,%s\"", 
			ARG_HOST, ARG_PORT, gen_ts, strlen(randstr), randstr);
	printf("%s\n", command);

	printf("issuing command...\n");
	system(command);
	log_info(gen_ts, randstr);
	return 0;
}
