//
// Created by gursimran on 25/6/18.
//

#ifndef TRANSMITTERCLIENT_PROBE_STUFFING_H
#define TRANSMITTERCLIENT_PROBE_STUFFING_H

#ifdef __cplusplus
// tells the compiler that we are using C headers
extern "C" {
#endif

/*
 * Include libraries.
 */
#include <errno.h>
#include <linux/nl80211.h>
#include <netlink/errno.h>
#include <netlink/genl/genl.h>
#include <netlink/netlink.h>
#include <netlink/genl/ctrl.h>
#include <netlink/attr.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>

#include <android/log.h>
#include <net/if.h>

#include <malloc.h>
#include <strings.h>

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

char **split_data(char *, int);

int get_n_ies_reqd(size_t);


// Probe stuffing function.
int do_probe_stuffing(struct nl_sock *, int, int, int, unsigned char *);

int driver(char *, char *, int);

#ifdef __cplusplus
}
#endif

#endif //TRANSMITTERCLIENT_PROBE_STUFFING_H
