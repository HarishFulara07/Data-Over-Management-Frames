/*
* Utility functions.
*/

#include "headers.h"

// Source: http://sourcecodebrowser.com/iw/0.9.14/genl_8c.html.
int nl_get_multicast_id(struct nl_sock *sock,
        const char *family, const char *group) {
    struct nl_msg *msg;
    struct nl_cb *cb;
    int ret, ctrlid;
    struct handler_args grp = { .group = group, .id = -ENOENT, };

    msg = nlmsg_alloc();
    if (!msg)
        return -ENOMEM;

    cb = nl_cb_alloc(NL_CB_DEFAULT);
    if (!cb) {
        ret = -ENOMEM;
        goto out_fail_cb;
    }

    ctrlid = genl_ctrl_resolve(sock, "nlctrl");

    genlmsg_put(msg, 0, 0, ctrlid, 0, 0, CTRL_CMD_GETFAMILY, 0);

    ret = -ENOBUFS;
    NLA_PUT_STRING(msg, CTRL_ATTR_FAMILY_NAME, family);

    ret = nl_send_auto_complete(sock, msg);
    if (ret < 0) goto out;

    ret = 1;

    nl_cb_err(cb, NL_CB_CUSTOM, error_handler, &ret);
    nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, &ret);
    nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, family_handler, &grp);

    while (ret > 0) nl_recvmsgs(sock, cb);

    if (ret == 0) ret = grp.id;

    nla_put_failure:
        out:
            nl_cb_put(cb);
        out_fail_cb:
            nlmsg_free(msg);
            return ret;
}

// Create a valid vendor specific IE using the data passed by the client.
struct ie_info * create_vendor_ie(char *data) {
    size_t vendor_tag_number_len = 1;
    size_t vendor_oui_len = 3;
    size_t data_size = strlen(data);
    size_t ie_size = vendor_oui_len + data_size;
    struct ie_info * ie = (struct ie_info *)malloc(sizeof(struct ie_info));
    ie->ie_len = vendor_tag_number_len + ie_size + 1;
    ie->ie_data = (unsigned char *)malloc(ie->ie_len);

    // Vendor tag number.
    ie->ie_data[0] = 221;
    // Size of IE.
    ie->ie_data[1] = ie_size;
    // Vendor OUI (= 123). This is also a random value.
    ie->ie_data[2] = 1;
    ie->ie_data[3] = 2;
    ie->ie_data[4] = 3;

    int data_idx = 0;
    for (int i = 5; i < data_size + 5; ++i) {
        ie->ie_data[i] = data[data_idx];
        ++data_idx;
    }

    return ie;
}
