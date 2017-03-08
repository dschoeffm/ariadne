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
	struct ifaddrmsg *ifa = (struct ifaddrmsg *) mnl_nlmsg_get_payload(nlh);
	struct nlattr *attr;
	vector<shared_ptr<Interface>>* interfaces = static_cast<vector<shared_ptr<Interface>>*>(data);
	uint32_t index = ifa->ifa_index;

	/*
	auto it = find_if(interfaces->begin(), interfaces->end(),
			[index](Interface& i){
				return i.netlinkIndex == index;
			});

	shared_ptr<Interface>& interface =
		(it == interfaces->end())
		? *interfaces->emplace(interfaces->end())
		: *it;
	interface->netlinkIndex = index;
	*/

	shared_ptr<Interface> iface_ptr;
	for(auto i : *interfaces){
		if(i->netlinkIndex == index){
			iface_ptr = i;
		}
	}
	if(iface_ptr == nullptr){
		interfaces->push_back(make_shared<Interface>());
		iface_ptr = interfaces->back();
		iface_ptr->netlinkIndex = index;
	}

	mnl_attr_for_each_cpp(attr, nlh, sizeof(*ifa)) {
		int type = mnl_attr_get_type(attr);

		/* skip unsupported attribute in user-space */
		if (mnl_attr_type_valid(attr, IFLA_MAX) < 0)
			continue;

		switch(type) {
		case IFA_ADDRESS:
			if (mnl_attr_validate(attr, MNL_TYPE_BINARY) < 0) {
				logErr("mnl_attr_validate() failed");
				return MNL_CB_ERROR;
			}
			iface_ptr->IPs.push_back(htonl(mnl_attr_get_u32(attr)));
			break;
		case IFA_LABEL:
			if (mnl_attr_validate(attr, MNL_TYPE_STRING) < 0) {
				logErr("mnl_attr_validate() failed");
				return MNL_CB_ERROR;
			}
			iface_ptr->name = mnl_attr_get_str(attr);
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

	vector<shared_ptr<Interface>>* interfaces = static_cast<vector<shared_ptr<Interface>>*>(data);
	uint32_t index = ifm->ifi_index;

	/*
	auto it = find_if(interfaces->begin(), interfaces->end(),
			[index](Interface& i){
				return i.netlinkIndex == index;
			});

	shared_ptr<Interface>& interface =
		(it == interfaces->end())
		? *interfaces->emplace(interfaces->end())
		: *it;
	interface->netlinkIndex = index;
	interface->name = "noname";
	interface->mac = {{0}};
	//interface.IPs.resize(0);
	*/

	shared_ptr<Interface> iface_ptr;
	for(auto i : *interfaces){
		if(i->netlinkIndex == index){
			iface_ptr = i;
		}
	}
	if(iface_ptr == nullptr){
		interfaces->push_back(make_shared<Interface>());
		iface_ptr = interfaces->back();
		iface_ptr->netlinkIndex = index;
		iface_ptr->name = "noname";
		iface_ptr->mac = {{0}};
	}

	mnl_attr_for_each_cpp(attr, nlh, sizeof(*ifm)) {
		int type = mnl_attr_get_type(attr);

		/* skip unsupported attribute in user-space */
		if (mnl_attr_type_valid(attr, IFLA_MAX) < 0)
			continue;

		switch(type) {
		case IFLA_ADDRESS:
			memcpy(&iface_ptr->mac, RTA_DATA(attr), 6);
			break;
		case IFA_LABEL:
			if (mnl_attr_validate(attr, MNL_TYPE_STRING) < 0) {
				logErr("mnl_attr_validate() failed");
				return MNL_CB_ERROR;
			}
			iface_ptr->name = mnl_attr_get_str(attr);
			break;

		}
	}

	return MNL_CB_OK;
}

// Adapted from:
// https://git.netfilter.org/libmnl/plain/examples/rtnl/rtnl-link-dump3.c
vector<shared_ptr<Interface>> Netlink::getAllInterfaces() {
	struct mnl_socket *nl;
	char buf[MNL_SOCKET_BUFFER_SIZE];
	struct nlmsghdr *nlh;
	struct rtgenmsg *rt;
	int ret;
	unsigned int seq, portid;

	vector<shared_ptr<Interface>> interfaces;

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
	rt->rtgen_family = AF_INET;

	if (mnl_socket_sendto(nl, nlh, nlh->nlmsg_len) < 0) {
		fatal("mnl_socket_sendto() failed");
	}

	ret = mnl_socket_recvfrom(nl, buf, sizeof(buf));
	while (ret > 0) {
		ret = mnl_cb_run(buf, ret, seq, portid, data_cb_ip, &interfaces);
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
		ret = mnl_cb_run(buf, ret, seq, portid, data_cb_link, &interfaces);
		if (ret <= MNL_CB_STOP)
			break;
		ret = mnl_socket_recvfrom(nl, buf, sizeof(buf));
	}
	if (ret == -1) {
		fatal("mnl_socket_recvfrom() failed");
	}

	mnl_socket_close(nl);

	stringstream sstream;

	sstream << "Interfaces:" << endl;
	for(auto i : interfaces){
		sstream << "\tName: " << i->name << endl;
		sstream << "\tMAC address: " << mac_to_str(i->mac) << endl;
		sstream << "\tNetlink Index: " << i->netlinkIndex << endl;
		sstream << "\tIP addresses: ";
		for(auto ip : i->IPs) {
			sstream << ip_to_str(ip) << endl << "\t              ";
		}
		sstream << endl;
	}

	logInfo(sstream.str());

	return interfaces;
}

