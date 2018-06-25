//
// Created by Gursimran Singh on 10/06/18.
//

#ifndef TRANSMITTERCLIENT_HEADERS_H
#define TRANSMITTERCLIENT_HEADERS_H

/*
 * Include libraries.
 */
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <cstdio>
#include <sys/time.h>
#include <unistd.h>

#include <linux/nl80211.h>

#include <net/if.h>
#include <netlink/errno.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <netlink/netlink.h>

#include <android/log.h>

/*
 * Define structures to be used in the code.
 */

// Results of triggering the scan.
struct trigger_results {
    int done;
    int aborted;
};

// For family_handler() and nl_get_multicast_id().
struct handler_args {
    const char *group;
    int id;
};

// For storing info about the IE.
struct ie_info {
    size_t ie_len;
    unsigned char *ie_data;
};

/*
 * Function prototypes.
 */

// Callback functions.
int error_handler(struct sockaddr_nl *, struct nlmsgerr *, void *);

int finish_handler(struct nl_msg *, void *);

int ack_handler(struct nl_msg *, void *);

int no_seq_check(struct nl_msg *, void *);

int family_handler(struct nl_msg *, void *);

int callback_trigger(struct nl_msg *, void *);

// Utility functions.
struct nl_sock *init_socket();

int nl_get_multicast_id(struct nl_sock *, const char *, const char *);

struct ie_info *create_vendor_ie(char *);

int get_ack(struct nl_msg *, void *);

char *read_file(char *);

char **split_data(char *, int);

int get_n_ies_reqd(size_t);

void
log_info(char *, unsigned int, unsigned int, char *, unsigned int, unsigned int, int, int, int);

// Probe stuffing fuction.
int do_probe_stuffing(struct nl_sock *, int, int, int, unsigned char *);

void driver(char *, char *, int);

// Main function.
//int if_nametoindex(char *);

//int genl_ctrl_resolve(struct nl_sock *, char *);

#endif //TRANSMITTERCLIENT_HEADERS_H
