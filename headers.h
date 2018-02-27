#ifndef HEADER
#define HEADER

/*
 * Include libraries.
 */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <netlink/errno.h>
#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <linux/nl80211.h>

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
int nl_get_multicast_id(struct nl_sock *, const char *, const char *);
struct ie_info * create_vendor_ie(char *);

// Probe stuffing fuction.
int do_probe_stuffing(struct nl_sock *, int, int, struct ie_info *);

// Main function.
int if_nametoindex(char *);
int genl_ctrl_resolve(struct nl_sock *, char *);

#endif
