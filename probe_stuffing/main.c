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
    int driver_id;
    struct nl_sock *socket = NULL;
    int seq_num = -1;
    int ack_seq_num = -1;
    int scan_retries_left = 4;
    int ack_retries_left = 4;

    while (1) {
        socket = init_socket();
        driver_id = genl_ctrl_resolve(socket, "nl80211");

        struct ie_info **ies = (struct ie_info **) malloc((n_ies + 1) * sizeof(struct ie_info *));
        // Combined length of all IEs.
        size_t ies_len = 0;

        // The first IE contains the sequence number.
        char seq_num_str[17];
        seq_num++;
        sprintf(seq_num_str, "%d", seq_num);
        ies[0] = create_vendor_ie(seq_num_str);
        ies_len += ies[0]->ie_len;
        seq_num -= ies_len;

        for (int i = 1; i < n_ies + 1; ++i) {
            // Create a vendor specific IE corresponding to the data sent by the client.
            ies[i] = create_vendor_ie(data_to_stuff[i-1]);
            ies_len += ies[i]->ie_len;
        }

        seq_num += ies_len - n_ies * 5;

        unsigned char *ies_data = (unsigned char *)malloc(ies_len);
        int k = 0;

        for (int i = 0; i < n_ies + 1; ++i) {
            for (int j = 0; j < ies[i]->ie_len; ++j) {
                ies_data[k] = ies[i]->ie_data[j];
                ++k;
            }
        }

        while (1) {
            /*
             * Send stuffed probe request frames.
             */

            // Issue NL80211_CMD_TRIGGER_SCAN to the kernel and wait for it to finish.
            int err = do_probe_stuffing(socket, interface_index, driver_id, ies_len, ies_data);

            if (err != 0) {  // Scanning failed.
                printf("do_probe_stuffing() failed with %d.\n", err);
                scan_retries_left--;

                if (scan_retries_left > 0) {
                    nl_socket_free(socket);
                    // Sleep for 15s before doing next scan.
                    printf("%d retries left. Will retry after 15s.\n", scan_retries_left);
                    sleep(15);
                    // Re-initialize the socket.
                    socket = init_socket();
                } else
                    break;
            } else {  // Scanning done.
                scan_retries_left = 3;
                // sleep(1);

                /*
                 * Now get ACK for the stuffed probe request frames.
                 */

                struct nl_msg *msg = nlmsg_alloc();
                // Run NL80211_CMD_GET_SCAN command.
                genlmsg_put(msg, 0, 0, driver_id, 0, NLM_F_DUMP, NL80211_CMD_GET_SCAN, 0);
                // Interface on which we scanned. Scan results will available on this interface only.
                nla_put_u32(msg, NL80211_ATTR_IFINDEX, interface_index);
                // Callbacks.
                nl_socket_modify_cb(socket, NL_CB_VALID, NL_CB_CUSTOM, get_ack, &ack_seq_num);

                int ret = nl_send_auto(socket, msg);
                ret = nl_recvmsgs_default(socket);
                nlmsg_free(msg);

                if (ret < 0) {  // Didn't receive an ACK.
                    printf("ERROR: nl_recvmsgs_default() returned %d (%s).\n", ret, nl_geterror(-ret));
                    ack_retries_left--;

                    if (ack_retries_left > 0) {
                        nl_socket_free(socket);
                        // Wait for 15 seconds before sending new scan request.
                        printf("%d retries left. Will retry after 15s.\n", ack_retries_left);
                        sleep(15);
                        socket = init_socket();
                    } else
                        break;
                } else {  // Received an ACK.
                    printf("Expected ACK: %d\n", seq_num + 1);
                    printf("Received ACK: %d\n", ack_seq_num);
                    // Invalid ACK. Retry.
                    if (ack_seq_num == -1 || ack_seq_num != seq_num + 1) {
                        ack_retries_left--;
                        printf("Invalid received ACK.\n");
                        
                        if (ack_retries_left > 0) {
                            nl_socket_free(socket);
                            // Wait for 15 seconds before sending new scan request.
                            printf("%d retries left. Will retry after 15s.\n", ack_retries_left);
                            sleep(15);
                            socket = init_socket();
                        } else
                            break;
                    } else {  // Everything went well. Scan and ACK flow completed.
                        printf("ACK received successfully.\n\nWaiting for 15s before sending more data.");
                        fflush(stdout);
                        // seq_num += ack_seq_num;
                        free(ies_data);
                        ack_retries_left = 3;
                        break;
                    }
                }
            }
        }

        if (scan_retries_left == 0) {
            printf("No more retries left for scanning. Exiting...\n");
            break;
        } else if (ack_retries_left == 0) {
            printf("No more retries left for getting ACK from an AP. Exiting...\n");
            break;
        } else { // Everything went well :)
            // Sleep for 10 seconds before sending more data.
            printf("\nGoing to sleep for 15s before sending new scan request.\n");
            sleep(15);
        }

        nl_socket_free(socket);
        printf("\n");
    }

    nl_socket_free(socket);
    return 0;
}
