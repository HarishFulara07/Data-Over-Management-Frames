/**
 * @author: Gursimran Singh
 * @github: https://github.com/gursimransinghhanspal
 *
 *
 * Receiving data over TCP or UDP:
 * The following script defines the controller in the protocol.
 *
 * A controller receive data from clients (for benchmarking) or access points (in protocol) over TCP or UDP. 
 * Types of supported data and handling:
 *		** Parceable String - The comma separated tuple from a clients		- STR
 *		** Parceable File	- A file containing many `Parceable Strings`	- FIL
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sqlite3.h>

#define CONNTYPE_TCP 1
#define CONNTYPE_UDP 2

#define SOH 0x1
#define EOT 0x4
#define US 0x1F
#define STR "STR"
#define FIL "FIL"

const int BUFFER_SZ = 1200;
const int MAX_STR_SZ = 1000;

// db constants
const char *DB_TABLE_NAME = "aplog";
const char *DB_CREATE_TABLE_QUERY = "CREATE TABLE IF NOT EXISTS aplog (" \
                                    "client_mac_addr TEXT NOT NULL," \
                                    "data_cli_utc_timestamp NUMERIC NOT NULL," \
                                    "data_ap_utc_timestamp NUMERIC NOT NULL," \
                                    "data_con_utc_timestamp NUMERIC NOT NULL," \
                                    "frame_seq_number INTEGER NOT NULL," \
                                    "ie_index INTEGER NOT NULL," \
                                    "more_frag_bit INTEGER NOT NULL," \
                                    "data_length INTEGER NOT NULL," \
                                    "data TEXT NOT NULL," \
                                    "PRIMARY KEY(client_mac_addr, data_cli_utc_timestamp, " \
                                    "frame_seq_number, ie_index)" \
                                    ");";

static int FLAG_CONNTYPE = CONNTYPE_TCP;
static int ARG_PORT = -1;
static char *ARG_DB_FILE = "db.sqlite3";

/**
 * Simple error handling
 * @param msg: the error message
 */
void
error(const char *msg) {
	perror(msg);
	exit(1);
}

static int
db_callback(void *NotUsed, int argc, char **argv, char **azColName) {
	int i;
	for (i = 0; i < argc; i++) {
		printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
	}
	printf("\n");
	return 0;
}

void
process_and_save(int sd, uint64_t con_ts, char *string, sqlite3 *db) {

	/* ************************************************************************
	 * expected format:
	 * client_mac, cli_ts, ap_ts, frame_seq_num, ie_index, mf, data len, data
	 * ************************************************************************/

	// insert data into database
	char *split_str[8];

	// split by comma
	int i = 0;

	char *token = strtok(string, ",");
	split_str[i] = token;
	i++;
	while (token != NULL) {
		// printf("%s\n", token);
		token = strtok(NULL, ",");
		split_str[i] = token;
		i++;
	}

	char sql_insert_template[1024] = "INSERT INTO %s ("
	                                 "client_mac_addr, " \
                                     "data_cli_utc_timestamp, " \
                                     "data_ap_utc_timestamp, " \
                                     "data_con_utc_timestamp, " \
                                     "frame_seq_number, " \
                                     "ie_index, " \
                                     "more_frag_bit, " \
                                     "data_length, " \
                                     "data" \
                                     ") " \
                                     "VALUES('%s', %s, %s, %018llu, %s, %s, %s, %s, '%s');";

    char sql_insert[1024];

    // if mac addr is not available, insert IP addr.
    if (strcmp(split_str[1], "NA") == 0) {
	    struct sockaddr_in client_addr;
		socklen_t client_len;
	    getpeername(sd, (struct sockaddr *) &client_addr, &client_len);
		printf("Host disconnected , ip %s , port %d \n",
		       inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

		snprintf(sql_insert, 1024, sql_insert_template,
	         DB_TABLE_NAME,
	         split_str[0], inet_ntoa(client_addr.sin_addr), split_str[2], con_ts, split_str[3], split_str[4], split_str[5], split_str[6],
	         split_str[7]);
	} else {
		snprintf(sql_insert, 1024, sql_insert_template,
	         DB_TABLE_NAME,
	         split_str[0], split_str[1], split_str[2], con_ts, split_str[3], split_str[4], split_str[5], split_str[6],
	         split_str[7]);
	}

	char *db_err_msg = NULL;
	int rc = sqlite3_exec(db, sql_insert, db_callback, NULL, &db_err_msg);
	if (rc != SQLITE_OK) {
		fprintf(stderr, "SQL error: %s\n", db_err_msg);
		sqlite3_free(db_err_msg);
	} else {
		fprintf(stdout, "Record created successfully\n");
	}

	printf("\n");
}

void
process_and_save_file(char *file, sqlite3 *db) {

}

/**
 * Initializes a server socket.
 * Binds it to the defined port.
 * Initiates listening on said port.
 */
int
init_server() {
	int server_sockfd;

	// create a socket
	// socket(int domain, int type, int protocol);
	if (FLAG_CONNTYPE == CONNTYPE_TCP) {
		server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	} else if (FLAG_CONNTYPE == CONNTYPE_UDP) {
		server_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	}
	if (server_sockfd < 0) {
		error("ERROR opening socket");
	}

	//set master socket to allow multiple connections ,
	//this is just a good habit, it will work without this
	int opt = 1;
	if (setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof(opt)) < 0) {
		error("setsockopt");
	}

	struct sockaddr_in server_addr;

	// clear address structure
	bzero((char *) &server_addr, sizeof(server_addr));
	/* setup the host_addr structure for use in bind call */
	// server byte order
	server_addr.sin_family = AF_INET;
	// automatically be filled with current host's IP address
	server_addr.sin_addr.s_addr = INADDR_ANY;
	// convert short integer value for port must be converted into network byte order
	server_addr.sin_port = htons(ARG_PORT);

	// bind(int fd, struct sockaddr *local_addr, socklen_t addr_length)
	// bind() passes file descriptor, the address structure,
	// and the length of the address structure
	// This bind() call will bind  the socket to the current IP address on given port
	if (bind(server_sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
		error("ERROR on binding");
	}

	// This listen() call tells the socket to listen to the incoming connections.
	// The listen() function places all incoming connection into a backlog queue
	// until accept() call accepts the connection.
	// Here, we set the maximum size for the backlog queue to 5.
	listen(server_sockfd, 10);
	printf("Waiting for connections...");
	printf("\n");

	return server_sockfd;
}


/**
 * Accepts any incoming connection.
 * Uses select to keep track of all file descriptors.
 */
void
run_server(int server_sockfd, sqlite3 *db) {

	int max_clients = 50;
	int client_sockfds[max_clients];
	int max_sockfd;

	struct sockaddr_in client_addr;
	socklen_t client_len;

	fd_set read_fdset;
	char buffer[BUFFER_SZ];

	// initialize all client fds to 0.
	for (int i = 0; i < max_clients; i++) {
		client_sockfds[i] = 0;
	}

	while (1) {
		// clear the socket set
		FD_ZERO(&read_fdset);

		// add server socket to set
		FD_SET(server_sockfd, &read_fdset);
		max_sockfd = server_sockfd;

		// add client sockets to set
		for (int i = 0; i < max_clients; i++) {

			// socket descriptor
			int sd = client_sockfds[i];

			// if valid socket descriptor then add to read list
			if (sd > 0)
				FD_SET(sd, &read_fdset);

			// highest file descriptor number, need it for the select function
			if (sd > max_sockfd)
				max_sockfd = sd;
		}

		// wait for some activity on one of the sockets , timeout is NULL,
		// so wait indefinitely
		int activity;
		activity = select(max_sockfd + 1, &read_fdset, NULL, NULL, NULL);

		if ((activity < 0) && (errno != EINTR)) {
			printf("select error");
		}

		// if something happened on the master socket,
		// then its an incoming connection
		if (FD_ISSET(server_sockfd, &read_fdset)) {
			int client_sockfd;

			// clear address structure
			bzero((char *) &client_addr, sizeof(client_addr));

			if ((client_sockfd = accept(server_sockfd, (struct sockaddr *) &client_addr, &client_len)) < 0) {
				error("accept");
			}

			// inform user of socket number - used in send and receive commands
			printf("New connection , socket fd is %d , ip is : %s , port : %d\n", client_sockfd,
			       inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

			// add new socket to array of sockets
			for (int i = 0; i < max_clients; i++) {
				// if position is empty
				if (client_sockfds[i] == 0) {
					client_sockfds[i] = client_sockfd;
					printf("Adding to list of sockets at index %d\n", i);
					break;
				}
			}
		}

		// else its some IO operation on some other socket
		for (int i = 0; i < max_clients; i++) {

			int sd = client_sockfds[i];
			if (FD_ISSET(sd, &read_fdset)) {
				// Check if it was for closing , and also read the
				// incoming message
				printf("Activity detected on fd: %d\n", sd);

				// clear the buffer
				bzero(buffer, BUFFER_SZ);

				int valread;
				if ((valread = read(sd, buffer, BUFFER_SZ - 1)) == 0) {
					// Somebody disconnected , get his details and print
					getpeername(sd, (struct sockaddr *) &client_addr, &client_len);
					printf("Host disconnected , ip %s , port %d \n",
					       inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

					// Close the socket and mark as 0 in list for reuse
					close(sd);
					client_sockfds[i] = 0;

				} else {
					// decode data
					// data format: CMD [STR|FIL] | DATA STREAM
					printf("Decoding data\n");
					// printf("%s\n", buffer);

					char *read_ptr = buffer;
					// 1. place read ptr at SOH
					read_ptr = index(buffer, SOH);
					read_ptr += 1;
					// printf("%s\n", read_ptr);

					// 2. Extract the command
					char *us_ptr = index(read_ptr, US);
					int sublen = us_ptr - read_ptr;
					char *command = (char *) calloc(sublen, sizeof(char));
					strncpy(command, read_ptr, sublen);
					// printf("%s\n", command);
					read_ptr = us_ptr + 1;

					if (strcmp(command, STR) == 0) {

						// 3. read the remaining string into buffer
						char *eot_ptr = index(read_ptr, EOT);
						sublen = eot_ptr - read_ptr;
						char *data = (char *) calloc(sublen, sizeof(char));
						strncpy(data, read_ptr, sublen);

						// get data received timestamp
						struct timeval tv;
						gettimeofday(&tv, NULL);
						uint64_t con_ts = tv.tv_sec * (uint64_t) 1000000 + tv.tv_usec;

						printf("Received Data: %s\n", data);
						process_and_save(sd, con_ts, data, db);

					} else if (strcmp(command, FIL) == 0) {

					}
				}
			}
		}
	}
}

int
main(int argc, char **argv) {

	sqlite3 *db;
	char *db_err_msg;

	// **** Command line argument parsing ****
	while (1) {
		char arg_conntype[5];

		/* Define the arguments */
		static struct option long_options[] = {
				{"tcp",  no_argument,       &FLAG_CONNTYPE, CONNTYPE_TCP},
				{"udp",  no_argument,       &FLAG_CONNTYPE, CONNTYPE_UDP},
				{"port", required_argument, 0, 'p'},
				{"db",   required_argument, 0, 'd'},
				{0,      0,                 0, 0}
		};

		/* getopt_long stores the option index here. */
		int option_index = 0;
		int opt_long;
		opt_long = getopt_long(argc, argv, "p:", long_options, &option_index);

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

			case 'd':
				ARG_DB_FILE = (char *) calloc(strlen(optarg), sizeof(char));
				strcpy(ARG_DB_FILE, optarg);
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
		printf("`Connection type': `%d'", FLAG_CONNTYPE);
		printf("\n");
	} else {
		printf("`Connection type' is required!");
		printf("\n");
		exit(1);
	}
	if (ARG_PORT == -1) {
		printf("Argument `--port (-p)' is required!");
		printf("\n");
		exit(1);
	} else {
		printf("Argument `--port (-p)': `%d'", ARG_PORT);
		printf("\n");
	}

	if (ARG_DB_FILE) {
		printf("Database file: %s", ARG_DB_FILE);
		printf("\n");

		// Open database
		int rc = sqlite3_open(ARG_DB_FILE, &db);
		if (rc) {
			fprintf(stderr, "%s\n", sqlite3_errmsg(db));
			error("Cannot open database");
		} else {
			printf("Database connection opened successfully!\n");
		}

		// Create table
		rc = sqlite3_exec(db, DB_CREATE_TABLE_QUERY, db_callback, NULL, &db_err_msg);
		if (rc != SQLITE_OK) {
			fprintf(stderr, "SQL error: %s\n", db_err_msg);
			sqlite3_free(db_err_msg);
		} else {
			fprintf(stdout, "Create Table query executed successfully.\n");
		}
	}
	printf("\n");


	// **** Server setup ****
	int server_sockfd = init_server();

	// **** Handle client requests ****
	run_server(server_sockfd, db);

	return 0;
}