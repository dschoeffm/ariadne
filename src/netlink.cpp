#include "netlink.hpp"

#define mnl_attr_for_each_cpp(attr, nlh, offset) \
	for ((attr) = (struct nlattr*) mnl_nlmsg_get_payload_offset((nlh), (offset)); \
		mnl_attr_ok((attr), (char *)mnl_nlmsg_get_payload_tail(nlh) - (char *)(attr)); \
		(attr) = mnl_attr_next(attr))

using namespace std;

// Adapted from:
// https://git.netfilter.org/libmnl/plain/examples/rtnl/rtnl-link-dump3.c
static int data_cb_ip(const struct nlmsghdr *nlh, void *data)
{
	struct ifaddrmsg *ifm = (struct ifaddrmsg *) mnl_nlmsg_get_payload(nlh);
	struct nlattr *attr;
	vector<interface>* interfaces = static_cast<vector<interface>*>(data);
	uint32_t index = ifm->ifa_index;

	auto it = find_if(interfaces->begin(), interfaces->end(),
			[index](interface& i){
				return i == index;
			});

	interface& interface =
		(it == interfaces->end())
		? *interfaces->emplace(interfaces->end())
		: *it;
	interface.netlinkIndex = index;

	mnl_attr_for_each_cpp(attr, nlh, sizeof(*ifm)) {
		int type = mnl_attr_get_type(attr);

		/* skip unsupported attribute in user-space */
		if (mnl_attr_type_valid(attr, IFLA_MAX) < 0)
			continue;

		switch(type) {
		case IFA_ADDRESS:
			if (mnl_attr_validate(attr, MNL_TYPE_U32) < 0) {
				logErr("mnl_attr_validate");
				return MNL_CB_ERROR;
			}
			interface.IPs.push_back(htonl(mnl_attr_get_u32(attr)));
			break;
		case IFA_LABEL:
			if (mnl_attr_validate(attr, MNL_TYPE_STRING) < 0) {
				logErr("mnl_attr_validate");
				return MNL_CB_ERROR;
			}
			interface.name = mnl_attr_get_str(attr);
			break;
		}
	}

	return MNL_CB_OK;
}

// Adapted from:
// https://git.netfilter.org/libmnl/plain/examples/rtnl/rtnl-link-dump3.c
static int data_cb_link(const struct nlmsghdr *nlh, void *data) {
	struct ifinfomsg *ifm = (struct ifinfomsg*) mnl_nlmsg_get_payload(nlh);
	struct nlattr *attr;

	vector<interface>* interfaces = static_cast<vector<interface>*>(data);
	uint32_t index = ifm->ifi_index;

	auto it = find_if(interfaces->begin(), interfaces->end(),
			[index](interface& i){
				return i == index;
			});

	interface& interface =
		(it == interfaces->end())
		? *interfaces->emplace(interfaces->end())
		: *it;
	interface.netlinkIndex = index;

	mnl_attr_for_each_cpp(attr, nlh, sizeof(*ifm)) {
		int type = mnl_attr_get_type(attr);

		/* skip unsupported attribute in user-space */
		if (mnl_attr_type_valid(attr, IFLA_MAX) < 0)
			continue;

		switch(type) {
		case IFLA_ADDRESS:
			memcpy(&interface.mac, RTA_DATA(attr), 6);
			break;
		}
	}

	return MNL_CB_OK;
}

// Adapted from:
// https://git.netfilter.org/libmnl/plain/examples/rtnl/rtnl-link-dump3.c
shared_ptr<vector<interface>> Netlink::getAllInterfaces() {
	struct mnl_socket *nl;
	char buf[MNL_SOCKET_BUFFER_SIZE];
	struct nlmsghdr *nlh;
	struct rtgenmsg *rt;
	int ret;
	unsigned int seq, portid;

	shared_ptr<vector<interface>> interfaces = make_shared<vector<interface>>();

	// Open NL socket
	nl = mnl_socket_open(NETLINK_ROUTE);
	if (nl == NULL) {
		fatal("mnl_socket_open() failed");
	}

	if (mnl_socket_bind(nl, 0, MNL_SOCKET_AUTOPID) < 0) {
		fatal("mnl_socket_bind() failed");
	}
	portid = mnl_socket_get_portid(nl);

	// do "ip a"
	nlh = mnl_nlmsg_put_header(buf);
	nlh->nlmsg_type	= RTM_GETADDR;
	nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
	nlh->nlmsg_seq = seq = time(NULL);
	rt = (struct rtgenmsg *) mnl_nlmsg_put_extra_header(nlh, sizeof(struct rtgenmsg));
	rt->rtgen_family = AF_PACKET;

	if (mnl_socket_sendto(nl, nlh, nlh->nlmsg_len) < 0) {
		fatal("mnl_socket_sendto() failed");
	}

	ret = mnl_socket_recvfrom(nl, buf, sizeof(buf));
	while (ret > 0) {
		ret = mnl_cb_run(buf, ret, seq, portid, data_cb_ip, interfaces.get());
		if (ret <= MNL_CB_STOP)
			break;
		ret = mnl_socket_recvfrom(nl, buf, sizeof(buf));
	}
	if (ret == -1) {
		fatal("mnl_socket_recvfrom() failed");
	}

	// do "ip l"
	nlh = mnl_nlmsg_put_header(buf);
	nlh->nlmsg_type	= RTM_GETLINK;
	nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
	nlh->nlmsg_seq = seq = time(NULL);
	rt = (struct rtgenmsg *) mnl_nlmsg_put_extra_header(nlh, sizeof(struct rtgenmsg));
	rt->rtgen_family = AF_PACKET;

	if (mnl_socket_sendto(nl, nlh, nlh->nlmsg_len) < 0) {
		fatal("mnl_socket_sendto");
	}

	ret = mnl_socket_recvfrom(nl, buf, sizeof(buf));
	while (ret > 0) {
		ret = mnl_cb_run(buf, ret, seq, portid, data_cb_link, interfaces.get());
		if (ret <= MNL_CB_STOP)
			break;
		ret = mnl_socket_recvfrom(nl, buf, sizeof(buf));
	}
	if (ret == -1) {
		fatal("mnl_socket_recvfrom() failed");
	}

	mnl_socket_close(nl);

	return interfaces;
}

