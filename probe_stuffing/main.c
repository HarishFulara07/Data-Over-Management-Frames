/*
 *  Main function.
 */

#include "headers.h"

int main(int argc, char **argv) {
    int opt, data_arg_idx, n_ies = 1;
    char *wifi_interface = NULL, **data_to_stuff = NULL;

    while ((opt = getopt(argc, argv, "n:i:d:")) != -1) {
        switch (opt) {
            case 'n': 
                n_ies = atoi(optarg);
                data_to_stuff = (char **) malloc(n_ies * sizeof(char *));
                data_arg_idx = 0;
                break;
            case 'i':
                wifi_interface = optarg;
                break;
            case 'd':
                if (data_arg_idx >= n_ies)
                    break;
                if (!data_to_stuff)
                    data_to_stuff = (char **) malloc(n_ies * sizeof(char *));
                data_to_stuff[data_arg_idx] = optarg;
                ++data_arg_idx;
                break;
        }
    }

    if (!(wifi_interface || data_to_stuff)) {
        printf("Error: Invalid command line argument(s).\n");
        exit(EXIT_FAILURE);
    }

    int interface_index = if_nametoindex(wifi_interface);

    /*
     * Open socket to kernel.
     */

    // Allocate new netlink socket in memory.
    struct nl_sock *socket = nl_socket_alloc();
    // Create file descriptor and bind socket.
    genl_connect(socket);
    // Find the nl80211 driver ID.
    int driver_id = genl_ctrl_resolve(socket, "nl80211");

    struct ie_info **ies = (struct ie_info **) malloc(n_ies * sizeof(struct ie_info *));
    // Combined length of all IEs.
    size_t ies_len = 0;

    for (int i = 0; i < n_ies; ++i) {
        // Create a vendor specific IE corresponding to the data sent by the client.
        ies[i] = create_vendor_ie(data_to_stuff[i]);
        ies_len += ies[i]->ie_len;
    }

    unsigned char *ies_data = (unsigned char *)malloc(ies_len);
    int k = 0;

    for (int i = 0; i < n_ies; ++i) {
        for (int j = 0; j < ies[i]->ie_len; ++j) {
            ies_data[k] = ies[i]->ie_data[j];
            ++k;
        }
    }

    // Issue NL80211_CMD_TRIGGER_SCAN to the kernel and wait for it to finish.
    int err = do_probe_stuffing(socket, interface_index, driver_id, ies_len, ies_data);

    if (err != 0) {
        printf("do_probe_stuffing() failed with %d.\n", err);
        return err;
    }

    /*
     * Now get ACK for the stuffed probe request frames.
     */

    struct nl_msg *msg = nlmsg_alloc();
    // Run NL80211_CMD_GET_SCAN command.
    genlmsg_put(msg, 0, 0, driver_id, 0, NLM_F_DUMP, NL80211_CMD_GET_SCAN, 0);
    // Interface on which we scanned. Scan results will available on this interface only.
    nla_put_u32(msg, NL80211_ATTR_IFINDEX, interface_index);
    // Callbacks.
    nl_socket_modify_cb(socket, NL_CB_VALID, NL_CB_CUSTOM, get_ack, NULL);

    int ret = nl_send_auto(socket, msg);
    ret = nl_recvmsgs_default(socket);
    nlmsg_free(msg);

    if (ret < 0) {
        printf("ERROR: nl_recvmsgs_default() returned %d (%s).\n", ret, nl_geterror(-ret));
        return ret;
    }

    // int success = 0;
    // struct timeval t1, t2;

    // gettimeofday(&t1, NULL);

    // char * tmp = malloc(strlen(argv[2]));
    // char * buffer = malloc(6);
        
    // for (int i = 0; i < 1; ++i) {
    //     // Create a vendor specific IE corresponding to the data sent by the client.
    //     strcpy(tmp, argv[2]);
    //     sprintf(buffer, "%d", 1);
    //     strcat(tmp, buffer);
    //     struct ie_info *ie1 = create_vendor_ie("Harish1Harish1Harish1Harish1Harish1Harish1Harish1Harish1Harish1Harish1Harish1Harish1Harish1Harish1Harish1Harish1Harish1Harish1Harish1Harish1Harish1Harish1Harish1Harish1Harish1Harish1Harish1Harish1Harish1");
    //     sprintf(buffer, "%d", 2);
    //     strcat(tmp, buffer);
    //     struct ie_info *ie2 = create_vendor_ie("Gursimran1");

    //     // Issue NL80211_CMD_TRIGGER_SCAN to the kernel and wait for it to finish.
    //     int err = do_probe_stuffing(socket, interface_index, driver_id, ie1, ie2);
    //     usleep(2000000);

    //     if (err != 0) {
    //         printf("do_probe_stuffing() failed with %d.\n", err);
    //         return err;
    //     } else {
    //         success ++;
    //     }

    //     // printf("Probe stuffing completed successfully.\n");
    //     // free(tmp);
    //     // free(buffer);
    // }

    // gettimeofday(&t2, NULL);
    // printf("Success: %d\n", success);
    // double time_taken = (t2.tv_sec - t1.tv_sec) * 1000;
    // time_taken += (t2.tv_usec - t1.tv_usec) / 1000; 
    // printf("%f\n", time_taken / 1000);
    return 0;
}
