/*
 *  Main function.
 */

#include "headers.h"

int main(int argc, char **argv) {
    int opt;
    char *wifi_interface = NULL;

    /*
     * Reading command line parameters.
     *
     * i = WiFi interface on which passive scan request will be sent.
     * E.g.: sudo ./beacon_stuffing -i wlan0
     */
    while ((opt = getopt(argc, argv, "i:")) != -1) {
        switch (opt) {
            case 'i':
                wifi_interface = optarg;
                break;
        }
    }

    // Check if client provided a WiFi interface via CLI.
    if (!wifi_interface) {
        printf("Error: Invalid command line argument(s). Requires interface.\n");
        exit(EXIT_FAILURE);
    }

    int interface_index = if_nametoindex(wifi_interface);
    struct nl_sock *socket = init_socket();
    // Find the nl80211 driver ID.
    int driver_id = genl_ctrl_resolve(socket, "nl80211");

    while (1) {
        struct nl_msg *msg = nlmsg_alloc();

        // Issue NL80211_CMD_TRIGGER_SCAN to the kernel and wait for it to finish.
        int err = do_passive_scan(socket, interface_index, driver_id);

        if (err != 0) {
            printf("do_passive_scan() failed with %d.\n", err);
        } else {
            // Run NL80211_CMD_GET_SCAN command.
            genlmsg_put(msg, 0, 0, driver_id, 0, NLM_F_DUMP, NL80211_CMD_GET_SCAN, 0);
            // Interface on which we scanned. Scan results will available on this interface only.
            nla_put_u32(msg, NL80211_ATTR_IFINDEX, interface_index);
            // Callbacks.
            nl_socket_modify_cb(socket, NL_CB_VALID, NL_CB_CUSTOM, get_beacon_info, NULL);

            int ret = nl_send_auto(socket, msg);
            ret = nl_recvmsgs_default(socket);
            nlmsg_free(msg);

            if (ret < 0) {
                printf("ERROR: nl_recvmsgs_default() returned %d (%s).\n", ret, nl_geterror(-ret));
            }
        }
    }

    return 0;
}
