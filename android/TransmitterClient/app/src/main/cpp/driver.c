//
// Created by gursimran on 25/6/18.
//

//#include "driver.h"
#include <string.h>
#include <malloc.h>
#include <strings.h>
#include "probe_stuffing.h"

const char LOG_TAG[] = "driver.c";
char LOG_BUFFER[256];

int driver(char *wifi_interface, char *data_to_stuff, int effort) {

    int opt;

    /**
     * Reading command line parameters.
     *
     * i = WiFi interface on which scan request will be sent. The data
     *     will be stuffed inside the PReq packets sent during the scan.
     * d = Data to stuff inside the IEs.
     * f = Path to the file containing data to stuff.
     * e = Delivery effort (1 = Best delivery, 2 = Guaranteed delivery). Default = 2.
     *
     * NOTE: We can specify data to stuff inside IEs using either 'd' or 'f'
     * command line parameter. If both are specified, then we will give
     * preference to file, i.e. data to stuff will be read from the specified file.
     *
     * E.g. (read data from CLI): sudo ./probe_stuffing -i wlan0 -d "LoremIpsumDolor"
     * E.g. (read data from file): sudo ./probe_stuffing -i wlan0 -f abc.txt
     */
    // @gursimran: Not required!

    // Check if client provided a WiFi interface.
    if (!wifi_interface) {
        __android_log_write(ANDROID_LOG_ERROR, LOG_TAG, "driver(): Wifi Interface is required.");
        return 1;
    }

    if (effort == 2) {
        __android_log_write(ANDROID_LOG_INFO, LOG_TAG, "driver(): Guaranteed delivery.");
    } else if (effort == 1) {
        __android_log_write(ANDROID_LOG_INFO, LOG_TAG, "driver(): Best delivery.");
    } else {
        __android_log_write(ANDROID_LOG_ERROR, LOG_TAG, "driver(): Invalid Effort value.");
        return 1;
    }

    int interface_index = if_nametoindex(wifi_interface);
    int driver_id;
    struct nl_sock *socket = NULL;

    /*
     * We are sequencing each PReq frame.
     */

    // Sequence number of the current PReq frame.
    int seq_num = 0;
    // Sequence number expected in PRes frame.
    int ack_seq_num = -1;
    // Retries left for sending a scan request.
    // Each retry will occur after 15s.
    int scan_retries_left = 4;
    // Retries left for resending the PReq frame is wrong ACK is received.
    // Each retry will occur after 15s.
    int ack_retries_left = 4;

    // How much data we need to stuff inside PReq frames.
    int len_data_to_stuff = strlen(data_to_stuff);
    // How much data we have already stuffed inside PReq frames.
    int len_data_already_stuffed = 0;

    /*
     * We can stuff max 1452 bytes of IEs in a PReq frame.
     *
     * Each IE can be max 256 bytes out of which 3 bytes are for
     * OUI (Organizationally Unique Identifier), 1 byte for vendor specific type,
     * and 252 bytes for data.
     *
     * IEs will be structured inside a PReq packet as follows:
     *
     * IE0 Data: Seq. Number | Timestamp | More Fragment Bit
     * IE1...N Data: Data
     *
     * We will reserve 9 bytes for Seq. Number, 19 bytes for Timestamp, and
     * 2 bytes for More Fragment Bit, i.e. a total of 4 (reserved) + 9 + 19 + 2 = 34 bytes.
     *
     * So, the actual IEs (IE1...N) that contain the stuffed data can be 1452 - 34 = 1418 bytes.
     * So, we need a min of floor(1418 / 256) + 1 = 6 IEs.
     *
     * So, the actual bytes available to stuff data = 1418 - (6 * 4) = 1394 bytes.
     */
    int ies_max_data_size = 1394;

    // Current Timestamp (in ms). This will be same for all the PReq frames.
    struct timeval data_origin_tv;
    gettimeofday(&data_origin_tv, NULL);
    unsigned int data_origin_ts = (unsigned int) (data_origin_tv.tv_sec);
    unsigned int data_origin_ts_usec = (unsigned int) (data_origin_tv.tv_usec);
    char data_origin_ts_str[20];
    sprintf(data_origin_ts_str, "%012u%06u", data_origin_ts, data_origin_ts_usec);
    bzero(LOG_BUFFER, 256);
    snprintf(LOG_BUFFER, 256, "driver(): Timestamp: %s", data_origin_ts_str);
    __android_log_write(ANDROID_LOG_INFO, LOG_TAG, LOG_BUFFER);

    // Continue until we have no more data to stuff.
    while (len_data_already_stuffed < len_data_to_stuff) {
        char *data;
        int data_size;
        // Index in our 'data_to_stuff' string from where we will start stuffing.
        int data_start_idx = seq_num * ies_max_data_size;
        // More fragment bit.
        int mfb;

        // Check if we have more than 1402 bytes of data to stuff.
        // If so, we will need more PReq frames to stuff the data, i.e. mfb = 1.
        if (len_data_to_stuff - len_data_already_stuffed > ies_max_data_size) {
            data = (char *) calloc((size_t) (ies_max_data_size + 1), sizeof(char *));
            memcpy(data, &data_to_stuff[data_start_idx], (size_t) ies_max_data_size);
            data[ies_max_data_size] = '\0';
            len_data_already_stuffed += ies_max_data_size;
            mfb = 1;
        } else { // This is the last stuffed PReq frame we will send. mfb will be 0.
            // Amount of data left to stuff.
            int len_data_left = len_data_to_stuff - len_data_already_stuffed;
            data = (char *) calloc((size_t) (len_data_left + 1), sizeof(char *));
            memcpy(data, &data_to_stuff[data_start_idx], (size_t) len_data_left);
            data[len_data_left] = '\0';
            len_data_already_stuffed = len_data_to_stuff;
            mfb = 0;
        }

        socket = init_socket();
        // Find the nl80211 driver ID.
        driver_id = genl_ctrl_resolve(socket, "nl80211");

        // Number of IEs we will need to stuff the data.
        int n_ies = get_n_ies_reqd(strlen(data));
        struct ie_info **ies = (struct ie_info **) malloc((n_ies + 1) * sizeof(struct ie_info *));
        // Combined length of all IEs.
        size_t ies_len = 0;

        /*
         * The first IE contains the sequence number, timestamp, and more fragment bit.
         */

        // Sequence number.
        char seq_num_str[9];
        // More fragment bit.
        char mfb_str[2];
        // Additional data that we will send in the first IE.
        char additional_data[28];

        sprintf(seq_num_str, "%08d", seq_num);
        sprintf(mfb_str, "%d", mfb);
        strcpy(additional_data, seq_num_str);
        strcat(additional_data, data_origin_ts_str);
        strcat(additional_data, mfb_str);

        // Create the first IE.
        ies[0] = create_vendor_ie(additional_data);
        ies_len += ies[0]->ie_len;

        // Divide the data into 252 bytes chunks.
        char **raw_ies_data = split_data(data, n_ies);
        data_size = strlen(data);
        free(data);

        for (int i = 1; i < n_ies + 1; ++i) {
            // Create a vendor specific IE corresponding to the data chunk.
            ies[i] = create_vendor_ie(raw_ies_data[i - 1]);
            ies_len += ies[i]->ie_len;
        }

        // Freeing up.
        for (int i = 0; i < n_ies; ++i) {
            free(raw_ies_data[i]);
        }
        free(raw_ies_data);

        // Append all the IEs into one string. We will pass this string to nl80211
        // and it will split them into IEs appropriately.
        unsigned char *ies_data = (unsigned char *) malloc(ies_len);
        int k = 0;

        for (int i = 0; i < n_ies + 1; ++i) {
            for (int j = 0; j < ies[i]->ie_len; ++j) {
                ies_data[k] = ies[i]->ie_data[j];
                ++k;
            }
        }

        while (1) {
            /*
             * Send stuffed probe request frame.
             */

            // Data transmission timestamp.
            struct timeval data_tx_tv;
            gettimeofday(&data_tx_tv, NULL);
            unsigned int data_tx_ts = (unsigned int) (data_tx_tv.tv_sec);
            unsigned int data_tx_ts_usec = (unsigned int) (data_tx_tv.tv_usec);

            bzero(LOG_BUFFER, 256);
            snprintf(LOG_BUFFER, 256, "driver(): Data transmission timestamp: %u%06u", data_tx_ts,
                     data_tx_ts_usec);
            __android_log_write(ANDROID_LOG_INFO, LOG_TAG, LOG_BUFFER);

            // Issue NL80211_CMD_TRIGGER_SCAN to the kernel and wait for it to finish.
            int err = do_probe_stuffing(socket, interface_index, driver_id, ies_len, ies_data);

            if (err != 0) {
                // Scanning failed.
                bzero(LOG_BUFFER, 256);
                snprintf(LOG_BUFFER, 256, "driver(): do_probe_stuffing() failed with %d.", err);
                __android_log_write(ANDROID_LOG_ERROR, LOG_TAG, LOG_BUFFER);
                scan_retries_left--;

                if (scan_retries_left > 0) {
                    nl_socket_free(socket);
                    // Sleep for 15s before doing next scan.
                    bzero(LOG_BUFFER, 256);
                    snprintf(LOG_BUFFER, 256, "driver(): %d retries left. Will retry after 15s.",
                             scan_retries_left);
                    __android_log_write(ANDROID_LOG_INFO, LOG_TAG, LOG_BUFFER);
                    sleep(15);

                    // Re-initialize the socket.
                    socket = init_socket();
                } else {
                    break;
                }
            } else {
                // Scanning done.
                scan_retries_left = 4;
                // sleep(5);

                /*
                 * Now get ACK for the stuffed probe request frames.
                 */

                struct nl_msg *msg = nlmsg_alloc();
                // Run NL80211_CMD_GET_SCAN command.
                genlmsg_put(msg, 0, 0, driver_id, 0, NLM_F_DUMP, NL80211_CMD_GET_SCAN, 0);
                // Interface on which we scanned. Scan results will available on this interface only.
                nla_put_u32(msg, NL80211_ATTR_IFINDEX, (uint32_t) interface_index);
                // Callbacks.
                nl_socket_modify_cb(socket, NL_CB_VALID, NL_CB_CUSTOM, get_ack, &ack_seq_num);

                int ret = nl_send_auto(socket, msg);
                ret = nl_recvmsgs_default(socket);
                nlmsg_free(msg);

                if (ret < 0) {
                    // Didn't receive an ACK.
                    bzero(LOG_BUFFER, 256);
                    snprintf(LOG_BUFFER, 256, "driver(): nl_recvmsgs_default() returned %d (%s).",
                             ret, nl_geterror(-ret));
                    __android_log_write(ANDROID_LOG_ERROR, LOG_TAG, LOG_BUFFER);

                    // Retry only if it is guaranteed delivery effort.
                    if (effort == 2) {
                        ack_retries_left--;

                        if (ack_retries_left > 0) {
                            nl_socket_free(socket);
                            // Wait for 15 seconds before sending new scan request.
                            bzero(LOG_BUFFER, 256);
                            snprintf(LOG_BUFFER, 256,
                                     "driver(): %d retries left. Will retry after 15s.",
                                     ack_retries_left);
                            __android_log_write(ANDROID_LOG_INFO, LOG_TAG, LOG_BUFFER);

                            sleep(15);
                            socket = init_socket();
                        } else {
                            break;
                        }
                    } else {
                        // Don't retry for best delivery effort.
                        __android_log_write(ANDROID_LOG_INFO, LOG_TAG,
                                            "Will not retry since it is best delivery effort...");
                        seq_num += 1;
                        break;
                    }
                } else {
                    // Received an ACK.
                    bzero(LOG_BUFFER, 256);
                    snprintf(LOG_BUFFER, 256, "driver(): Expected ACK: %d", seq_num);
                    __android_log_write(ANDROID_LOG_INFO, LOG_TAG, LOG_BUFFER);

                    bzero(LOG_BUFFER, 256);
                    snprintf(LOG_BUFFER, 256, "driver(): Received ACK: %d", ack_seq_num);
                    __android_log_write(ANDROID_LOG_INFO, LOG_TAG, LOG_BUFFER);

                    // Invalid ACK. Retry.
                    if (ack_seq_num == -1 || ack_seq_num != seq_num) {
                        // Retry only if it is guaranteed delivery effort.
                        if (effort == 2) {
                            ack_retries_left--;
                            __android_log_write(ANDROID_LOG_ERROR, LOG_TAG,
                                                "Invalid received ACK.");

                            if (ack_retries_left > 0) {
                                nl_socket_free(socket);

                                // Wait for 15 seconds before sending new scan request.
                                bzero(LOG_BUFFER, 256);
                                snprintf(LOG_BUFFER, 256,
                                         "driver(): %d retries left. Will retry after 15s.",
                                         ack_retries_left);
                                __android_log_write(ANDROID_LOG_INFO, LOG_TAG, LOG_BUFFER);

                                sleep(15);
                                socket = init_socket();
                            } else {
                                break;
                            }
                        } else {
                            // Don't retry for best delivery effort.
                            __android_log_write(ANDROID_LOG_INFO, LOG_TAG,
                                                "Will not retry since it is best delivery effort...");
                            seq_num += 1;
                            break;
                        }
                    } else {
                        // Everything went well. Scan and ACK flow completed.
                        __android_log_write(ANDROID_LOG_INFO, LOG_TAG,
                                            "ACK received successfully.");
                        __android_log_write(ANDROID_LOG_INFO, LOG_TAG,
                                            "Waiting for 15s before sending more data.");

                        seq_num += 1;
                        free(ies_data);
                        ack_retries_left = 4;
                        break;
                    }
                }
            }
        }

        if (scan_retries_left == 0) {
            __android_log_write(ANDROID_LOG_INFO, LOG_TAG,
                                "No more retries left for scanning. Exiting...");
            break;
        } else if (ack_retries_left == 0) {
            __android_log_write(ANDROID_LOG_INFO, LOG_TAG,
                                "No more retries left for getting ACK from an AP. Exiting...");
            break;
        } else {
            // Everything went well :)
            // Sleep for 10 seconds before sending more data.
            __android_log_write(ANDROID_LOG_INFO, LOG_TAG,
                                "Going to sleep for 15s before sending new scan request.");

            if (len_data_already_stuffed != len_data_to_stuff) {
                sleep(15);
            }
        }

        nl_socket_free(socket);
    }

    return 0;
}


/*
 * Function implementing probe stuffing.
 *
 * It starts the scan and stuffs data as vendor
 * specific IEs inside probe request packets.
 */
int do_probe_stuffing(struct nl_sock *socket, int interface_index,
                      int driver_id, int ies_len, unsigned char *ies_data) {

    struct trigger_results results = {
            .done = 0,
            .aborted = 0,
    };
    struct nl_msg *msg;
    struct nl_cb *cb;
    struct nl_msg *ssids_to_scan;
    int err;
    int ret;
    int mcid = nl_get_multicast_id(socket, "nl80211", "scan");
    // Without this, callback_trigger() won't be called.
    nl_socket_add_membership(socket, mcid);

    // Allocate the messages and callback handler.
    msg = nlmsg_alloc();
    if (!msg) {
        __android_log_write(ANDROID_LOG_ERROR, LOG_TAG,
                            "do_probe_stuffing(): Failed to allocate netlink message for msg.");
        return -ENOMEM;
    }

    ssids_to_scan = nlmsg_alloc();
    if (!ssids_to_scan) {
        __android_log_write(ANDROID_LOG_ERROR, LOG_TAG,
                            "do_probe_stuffing(): Failed to allocate netlink message for ssids_to_scan.");
        nlmsg_free(msg);
        return -ENOMEM;
    }

    cb = nl_cb_alloc(NL_CB_DEFAULT);
    if (!cb) {
        __android_log_write(ANDROID_LOG_ERROR, LOG_TAG,
                            "do_probe_stuffing(): Failed to allocate netlink callbacks.");
        nlmsg_free(msg);
        nlmsg_free(ssids_to_scan);
        return -ENOMEM;
    }

    /*
     * Setup the messages and callback handler.
     */

    // Setup which command to run.
    genlmsg_put(msg, 0, 0, driver_id, 0, 0, NL80211_CMD_TRIGGER_SCAN, 0);
    // Add message attribute, which interface to use.
    nla_put_u32(msg, NL80211_ATTR_IFINDEX, (uint32_t) interface_index);
    // Stuff IEs.
    nla_put(msg, NL80211_ATTR_IE, ies_len, ies_data);
    // Scan all SSIDs.
    nla_put(ssids_to_scan, 1, 0, "");
    // Add message attribute, which SSIDs to scan for.
    nla_put_nested(msg, NL80211_ATTR_SCAN_SSIDS, ssids_to_scan);

    nlmsg_free(ssids_to_scan);

    // Add the callback.
    nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, callback_trigger, &results);
    nl_cb_err(cb, NL_CB_CUSTOM, error_handler, &err);
    nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, &err);
    nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, &err);
    // No sequence checking for multicast messages.
    nl_cb_set(cb, NL_CB_SEQ_CHECK, NL_CB_CUSTOM, no_seq_check, NULL);

    // Send NL80211_CMD_TRIGGER_SCAN to start the scan.
    // The kernel may reply with NL80211_CMD_NEW_SCAN_RESULTS on
    // success or NL80211_CMD_SCAN_ABORTED if another
    // scan was started by another process.
    err = 1;
    ret = nl_send_auto(socket, msg);

    bzero(LOG_BUFFER, 256);
    snprintf(LOG_BUFFER, 256,
             "do_probe_stuffing(): NL80211_CMD_TRIGGER_SCAN sent %d bytes to the kernel.",
             ret);
    __android_log_write(ANDROID_LOG_INFO, LOG_TAG, LOG_BUFFER);
    __android_log_write(ANDROID_LOG_INFO, LOG_TAG, "Broadcasting stuffed probe request frames.");

    while (err > 0) {
        // First wait for ack_handler(). This helps with basic errors.
        ret = nl_recvmsgs(socket, cb);
    }

    if (err < 0) {
        bzero(LOG_BUFFER, 256);
        snprintf(LOG_BUFFER, 256, "do_probe_stuffing(): err has a value of %d.", err);
        __android_log_write(ANDROID_LOG_WARN, LOG_TAG, LOG_BUFFER);
    }

    if (ret < 0) {
        bzero(LOG_BUFFER, 256);
        snprintf(LOG_BUFFER, 256, "do_probe_stuffing(): nl_recvmsgs() returned %d (%s).", ret,
                 nl_geterror(-ret));
        __android_log_write(ANDROID_LOG_ERROR, LOG_TAG, LOG_BUFFER);
        nl_geterror(-ret);
        return ret;
    }

    while (!results.done) {
        // Now wait until the scan is done or aborted.
        nl_recvmsgs(socket, cb);
    }

    if (results.aborted) {
        __android_log_write(ANDROID_LOG_ERROR, LOG_TAG, "Kernel aborted scan.");
        return 1;
    }

    // Cleanup.
    nlmsg_free(msg);
    nl_cb_put(cb);
    nl_socket_drop_membership(socket, mcid);

    return 0;
}