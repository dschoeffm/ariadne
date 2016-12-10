#include "manager.hpp"

#define mnl_attr_for_each_cpp(attr, nlh, offset) \
	for ((attr) = (struct nlattr*) mnl_nlmsg_get_payload_offset((nlh), (offset)); \
		mnl_attr_ok((attr), (char *)mnl_nlmsg_get_payload_tail(nlh) - (char *)(attr)); \
		(attr) = mnl_attr_next(attr))

struct cb_opt {
	vector<unordered_set<uint32_t>>& own_IPs;
	vector<array<uint8_t, 6>>& interface_macs;
	vector<string>& interface_names;
	uint32_t cur_size;
};

// Adapted from:
// https://git.netfilter.org/libmnl/plain/examples/rtnl/rtnl-link-dump3.c
static int data_cb_ip(const struct nlmsghdr *nlh, void *data)
{
	struct ifaddrmsg *ifm = (struct ifaddrmsg *) mnl_nlmsg_get_payload(nlh);
	struct nlattr *attr;
	cb_opt* opt = (cb_opt*) data;

	opt->cur_size = max(opt->cur_size, ifm->ifa_index);
	opt->own_IPs.resize(opt->cur_size + 1);
	opt->interface_macs.resize(opt->cur_size + 1);
	opt->interface_names.resize(opt->cur_size + 1);

	mnl_attr_for_each_cpp(attr, nlh, sizeof(*ifm)) {
		int type = mnl_attr_get_type(attr);

		/* skip unsupported attribute in user-space */
		if (mnl_attr_type_valid(attr, IFLA_MAX) < 0)
			continue;

		switch(type) {
		case IFA_ADDRESS:
			if (mnl_attr_validate(attr, MNL_TYPE_U32) < 0) {
				perror("mnl_attr_validate");
				return MNL_CB_ERROR;
			}
			opt->own_IPs[ifm->ifa_index].insert(htonl(mnl_attr_get_u32(attr)));
			break;
		case IFA_LABEL:
			if (mnl_attr_validate(attr, MNL_TYPE_STRING) < 0) {
				perror("mnl_attr_validate");
				return MNL_CB_ERROR;
			}
			opt->interface_names[ifm->ifa_index] = mnl_attr_get_str(attr);
			break;
		}
	}

	return MNL_CB_OK;
}

static int data_cb_link(const struct nlmsghdr *nlh, void *data) {
	struct ifinfomsg *ifm = (struct ifinfomsg*) mnl_nlmsg_get_payload(nlh);
	struct nlattr *attr;

	cb_opt* opt = (cb_opt*) data;

	opt->cur_size = max(opt->cur_size, (uint32_t) ifm->ifi_index);
	opt->own_IPs.resize(opt->cur_size + 1);
	opt->interface_macs.resize(opt->cur_size + 1);
	opt->interface_names.resize(opt->cur_size + 1);

	mnl_attr_for_each_cpp(attr, nlh, sizeof(*ifm)) {
		int type = mnl_attr_get_type(attr);

		/* skip unsupported attribute in user-space */
		if (mnl_attr_type_valid(attr, IFLA_MAX) < 0)
			continue;

		switch(type) {
		case IFLA_ADDRESS:
			memcpy(&opt->interface_macs[ifm->ifi_index], RTA_DATA(attr), 6);
			break;
		}
	}
	printf("\n");

	return MNL_CB_OK;
}

// Adapted from:
// https://git.netfilter.org/libmnl/plain/examples/rtnl/rtnl-link-dump3.c
void Manager::fillNetLink() {
	struct mnl_socket *nl;
	char buf[MNL_SOCKET_BUFFER_SIZE];
	struct nlmsghdr *nlh;
	struct rtgenmsg *rt;
	int ret;
	unsigned int seq, portid;

	// Open NL socket
	nl = mnl_socket_open(NETLINK_ROUTE);
	if (nl == NULL) {
		perror("mnl_socket_open");
		exit(EXIT_FAILURE);
	}

	if (mnl_socket_bind(nl, 0, MNL_SOCKET_AUTOPID) < 0) {
		perror("mnl_socket_bind");
		exit(EXIT_FAILURE);
	}
	portid = mnl_socket_get_portid(nl);

	// do "ip a"
	nlh = mnl_nlmsg_put_header(buf);
	nlh->nlmsg_type	= RTM_GETADDR;
	nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
	nlh->nlmsg_seq = seq = time(NULL);
	rt = (struct rtgenmsg *) mnl_nlmsg_put_extra_header(nlh, sizeof(struct rtgenmsg));
	rt->rtgen_family = AF_PACKET;

	cb_opt opt = {own_IPs, interface_macs, interface_names, 0};

	if (mnl_socket_sendto(nl, nlh, nlh->nlmsg_len) < 0) {
		perror("mnl_socket_sendto");
		exit(EXIT_FAILURE);
	}

	ret = mnl_socket_recvfrom(nl, buf, sizeof(buf));
	while (ret > 0) {
		ret = mnl_cb_run(buf, ret, seq, portid, data_cb_ip, &opt);
		if (ret <= MNL_CB_STOP)
			break;
		ret = mnl_socket_recvfrom(nl, buf, sizeof(buf));
	}
	if (ret == -1) {
		perror("error");
		exit(EXIT_FAILURE);
	}

	// do "ip l"
	nlh = mnl_nlmsg_put_header(buf);
	nlh->nlmsg_type	= RTM_GETLINK;
	nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
	nlh->nlmsg_seq = seq = time(NULL);
	rt = (struct rtgenmsg *) mnl_nlmsg_put_extra_header(nlh, sizeof(struct rtgenmsg));
	rt->rtgen_family = AF_PACKET;

	if (mnl_socket_sendto(nl, nlh, nlh->nlmsg_len) < 0) {
		perror("mnl_socket_sendto");
		exit(EXIT_FAILURE);
	}

	ret = mnl_socket_recvfrom(nl, buf, sizeof(buf));
	while (ret > 0) {
		ret = mnl_cb_run(buf, ret, seq, portid, data_cb_link, &opt);
		if (ret <= MNL_CB_STOP)
			break;
		ret = mnl_socket_recvfrom(nl, buf, sizeof(buf));
	}
	if (ret == -1) {
		perror("error");
		exit(EXIT_FAILURE);
	}

	mnl_socket_close(nl);
}



