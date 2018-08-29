/*
* Utility functions.
*/

#include "headers.h"

// Intialize the netlink socket.
struct nl_sock * init_socket() {
	/*
	 * Open socket to kernel.
	 */

	// Allocate new netlink socket in memory.
	struct nl_sock *socket = nl_socket_alloc();
	// Create file descriptor and bind socket.
	genl_connect(socket);
	// Set socket as a non-blocking socket.
	nl_socket_set_nonblocking(socket);

	return socket;
}

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

// Extract data from Vendor IE inside beacon frame.
char * extract_beacon_data(unsigned char len, unsigned char *data) {
	// OUI of our vendor.
	unsigned char ps_oui[3] = {0x01, 0x02, 0x03};   // 123
	char *beacon_data = (char *)malloc((len - 2) * sizeof(char));

    if (len >= 4 && memcmp(data, ps_oui, 3) == 0) {
		int j = 0;
        for (int i = 0; i < len - 3; i++) {
			beacon_data[j] = data[i + 3];
            j++;
		}
        beacon_data[j] = '\0';
		printf("%s\n", beacon_data);
    }

	return NULL;
}

// Get data from beacon frames received during passive scan.
int get_beacon_info(struct nl_msg *msg, void *arg) {
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
	struct nlattr *tb[NL80211_ATTR_MAX + 1];
	struct nlattr *bss[NL80211_BSS_MAX + 1];
	static struct nla_policy bss_policy[NL80211_BSS_MAX + 1] = {
		[NL80211_BSS_TSF] = { .type = NLA_U64 },
		[NL80211_BSS_FREQUENCY] = { .type = NLA_U32 },
		[NL80211_BSS_BSSID] = { },
		[NL80211_BSS_BEACON_INTERVAL] = { .type = NLA_U16 },
		[NL80211_BSS_CAPABILITY] = { .type = NLA_U16 },
		[NL80211_BSS_INFORMATION_ELEMENTS] = { },
		[NL80211_BSS_SIGNAL_MBM] = { .type = NLA_U32 },
		[NL80211_BSS_SIGNAL_UNSPEC] = { .type = NLA_U8 },
		[NL80211_BSS_STATUS] = { .type = NLA_U32 },
		[NL80211_BSS_SEEN_MS_AGO] = { .type = NLA_U32 },
		[NL80211_BSS_BEACON_IES] = { },
	};

	// Parse and error check.
	nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0), genlmsg_attrlen(gnlh, 0), NULL);

	if (!tb[NL80211_ATTR_BSS]) {
		return NL_SKIP;
	}

	if (nla_parse_nested(bss, NL80211_BSS_MAX, tb[NL80211_ATTR_BSS], bss_policy)) {
		return NL_SKIP;
	}

	if (!bss[NL80211_BSS_BSSID])
		return NL_SKIP;

	if (!bss[NL80211_BSS_INFORMATION_ELEMENTS])
		return NL_SKIP;

    // Extract out the IEs.
	if (bss[NL80211_BSS_INFORMATION_ELEMENTS]) {
		unsigned char *ies = nla_data(bss[NL80211_BSS_INFORMATION_ELEMENTS]);
		int ies_len = nla_len(bss[NL80211_BSS_INFORMATION_ELEMENTS]);

        while (ies_len >= 2 && ies_len >= ies[1]) {
			// Vendor Specific IE.
			if (ies[0] == 221) {
                extract_beacon_data(ies[1], ies + 2);
			}
            ies_len -= ies[1] + 2;
			ies += ies[1] + 2;
		}
	}

	return NL_SKIP;
}
