/**
 * @author: Gursimran Singh
 * @github: https://github.com/gursimransinghhanspal
 *
 * Sending data over TCP/IP or UDP:
 * The following client side script sends data over a TCP or UDP connection to an open socket at the server.
 * The script can take as input simple strings or path to a file.
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ctime>
#include <sys/time.h>

#define CONNTYPE_TCP 1
#define CONNTYPE_UDP 2

#define SOH 0x1
#define EOT 0x4
#define US 0x1F
#define STR "STR"
#define FIL "FIL"

const int BUFFER_SZ = 1200;
const int MAX_STR_SZ = 1000;

static int FLAG_CONNTYPE = CONNTYPE_TCP;
static int ARG_PORT = -1;
static char *ARG_HOST_ADDR = NULL;
static char *ARG_INPUT_STRING = NULL;
static char *ARG_INPUT_FILE = NULL;

char *get_timestamp_as_string() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    unsigned long long ts = tv.tv_sec * (unsigned long long) 1000000 + tv.tv_usec;

    char *as_string = (char *) malloc(19 * sizeof(char));
    snprintf(as_string, 19, "%018llu", ts);
    return as_string;
}

void
error(const char *msg) {
    perror(msg);
    exit(1);
}

int
connect_client() {
    int client_sockfd;
    struct sockaddr_in server_addr;
    struct hostent *server;

    // create client socket
    if (FLAG_CONNTYPE == CONNTYPE_TCP) {
        client_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    } else if (FLAG_CONNTYPE == CONNTYPE_UDP) {
        client_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    }
    if (client_sockfd < 0) {
        error("Could not open fd");
    }

    // get host
    server = gethostbyname(ARG_HOST_ADDR);
    if (server == NULL) {
        error("ERROR, no such host");
    }

    // clear the server address struct
    bzero((char *) &server_addr, sizeof(server_addr));
    // populate the server address struct
    server_addr.sin_family = AF_INET;
    bcopy((char *) server->h_addr, (char *) &server_addr.sin_addr.s_addr, server->h_length);
    server_addr.sin_port = htons(ARG_PORT);

    char *ts = get_timestamp_as_string();
    printf("attempting to connect() <ts: %s>", ts);
    while (connect(client_sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        printf("Could not connect! Retrying in 5 seconds ...\n");
        sleep(5);

        free(ts);
        ts = get_timestamp_as_string();
        printf("attempting to connect() <ts: %s>", ts);
    }

    free(ts);
    ts = get_timestamp_as_string();
    printf("connection successful <ts: %s>", ts);

    return client_sockfd;
}

/**
 * Creates a command for the controller to decode and sends it to the server
 * @param client_sockfd:
 */
void
transmit(int client_sockfd) {
    char *buffer;

    if (ARG_INPUT_STRING) {
        buffer = (char *) calloc(BUFFER_SZ, sizeof(char));
        if (buffer == NULL) {
            error("Out of memory");
        }

        int bn = snprintf(buffer, BUFFER_SZ - 1, "%c%s%c%s%c", SOH, STR, US, ARG_INPUT_STRING, EOT);

        if (buffer[0] != SOH || buffer[bn - 1] != EOT) {
            error("Buffer size is too small");
        }

        printf("Command:\n%s", buffer);
        printf("\n");

        int wn = write(client_sockfd, buffer, bn);
        if (wn < 0) {
            error("ERROR writing to socket");
        } else {
            printf("Wrote %d bytes to fd", wn);
            printf("\n");
        }

        // free the memory
        //free(buffer);
    }
}

void
cleanup(int client_sockfd) {
    close(client_sockfd);
}

int
main(int argc, char *argv[]) {

    // **** Command line argument parsing ****
    while (1) {
        char arg_conntype[5];

        /* Define the arguments */
        static struct option long_options[] = {
                {"tcp",  no_argument,       &FLAG_CONNTYPE, CONNTYPE_TCP},
                {"udp",  no_argument,       &FLAG_CONNTYPE, CONNTYPE_UDP},
                {"host", required_argument, 0, 'h'},
                {"port", required_argument, 0, 'p'},
                {"str",  required_argument, 0, 's'},
                {"file", required_argument, 0, 'f'},
                {0,      0,                 0, 0}
        };

        /* getopt_long stores the option index here. */
        int option_index = 0;
        int opt_long;
        opt_long = getopt_long(argc, argv, "p:h:s:f:", long_options, &option_index);

        /* Detect the end of the options. */
        if (opt_long == -1)
            break;

        switch (opt_long) {
            case 0:
                /* If this option set a flag, do nothing else now. */
                if (long_options[option_index].flag != 0)
                    break;
                break;

            case 'p':
                ARG_PORT = atoi(optarg);
                break;

            case 'h':
                ARG_HOST_ADDR = (char *) calloc(strlen(optarg), sizeof(char));
                strcpy(ARG_HOST_ADDR, optarg);
                break;

            case 's':
                if (strlen(optarg) > MAX_STR_SZ) {
                    printf("Input string too large. Limit to %d chars", MAX_STR_SZ);
                    printf("\n");
                    exit(1);
                }
                ARG_INPUT_STRING = (char *) calloc(strlen(optarg), sizeof(char));
                strcpy(ARG_INPUT_STRING, optarg);
                break;

            case 'f':
                ARG_INPUT_FILE = (char *) calloc(strlen(optarg), sizeof(char));
                strcpy(ARG_INPUT_FILE, optarg);
                break;

            case '?':
                /* getopt_long already printed an error message. */
                exit(1);
                break;

            default:
                abort();
        }
    }

    if (FLAG_CONNTYPE) {
        printf("Connection type: `%d'", FLAG_CONNTYPE);
        printf("\n");
    }
    if (ARG_PORT == -1) {
        error("Argument `--port (-p)' required!");
    } else {
        printf("Argument `--port (-p)': `%d'", ARG_PORT);
        printf("\n");
    }
    if (!ARG_HOST_ADDR || strcmp(ARG_HOST_ADDR, "") == 0) {
        error("Argument `--host (-h)' required!");
    } else {
        printf("Argument `--host (-h)': `%s'", ARG_HOST_ADDR);
        printf("\n");
    }
    if (ARG_INPUT_STRING) {
        printf("Argument `--str (-s)': `%s'", ARG_INPUT_STRING);
        printf("\n");
    } else if (ARG_INPUT_FILE) {
        printf("Argument `--file (-f)': `%s'", ARG_INPUT_FILE);
        printf("\n");
    } else if (!ARG_INPUT_STRING && !ARG_INPUT_FILE) {
        error("Argument `--str (-s)' or `--file (-f)' required!");
    }
    printf("\n");

    // **** Connect client ****
    char *b1 = get_timestamp_as_string();
    printf("calling connect_client() <ts: %s>", b1);
    int client_sockfd = connect_client();
    char *b1_b = get_timestamp_as_string();
    printf("returned from connect_client() <ts: %s>", b1_b);

    // **** Send data [string or file] ****
    char *b2 = get_timestamp_as_string();
    printf("calling transmit() <ts: %s>", b2);
    transmit(client_sockfd);
    char *b2_b = get_timestamp_as_string();
    printf("returned from transmit() <ts: %s>", b2_b);

    // **** Close the connection gracefully ****
    cleanup(client_sockfd);
    free(b1);
    free(b1_b);
    free(b2);
    free(b2_b);

    return 0;
}
