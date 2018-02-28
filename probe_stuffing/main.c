/*
 *  Main function.
 */

#include "headers.h"

int main(int argc, char** argv) {
    if (argc < 3) {
        printf("Invalid number of arguments.\n");
        return -1;
    }

    int interface_index = if_nametoindex(argv[1]);

    /*
     * Open socket to kernel.
     */

    // Allocate new netlink socket in memory.
    struct nl_sock *socket = nl_socket_alloc();
    // Create file descriptor and bind socket.
    genl_connect(socket);
    // Find the nl80211 driver ID.
    int driver_id = genl_ctrl_resolve(socket, "nl80211");

    // Create a vendor specific IE corresponding to the data sent by the client.
    struct ie_info *ie = create_vendor_ie(argv[2]);
    // Issue NL80211_CMD_TRIGGER_SCAN to the kernel and wait for it to finish.
    int err = do_probe_stuffing(socket, interface_index, driver_id, ie);

    if (err != 0) {
        printf("do_probe_stuffing() failed with %d.\n", err);
        return err;
    }

    printf("Probe stuffing completed successfully.\n");

    return 0;
}
