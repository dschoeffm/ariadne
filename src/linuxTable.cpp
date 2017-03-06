#include "linuxTable.hpp"

// This implementation is base upon the following example:
// https://git.netfilter.org/libmnl/tree/examples/rtnl/rtnl-route-dump.c

using namespace std;

static int data_ipv4_attr_cb(const struct nlattr *attr, void *data)
{
	const struct nlattr **tb = static_cast<const struct nlattr **>(data);
	int type = mnl_attr_get_type(attr);

	/* skip unsupported attribute in user-space */
	if (mnl_attr_type_valid(attr, RTA_MAX) < 0)
		return MNL_CB_OK;

	switch(type) {
	case RTA_TABLE:
	case RTA_DST:
	case RTA_SRC:
	case RTA_OIF:
	case RTA_FLOW:
	case RTA_PREFSRC:
	case RTA_GATEWAY:
	case RTA_PRIORITY:
		if (mnl_attr_validate(attr, MNL_TYPE_U32) < 0) {
			logErr("mnl_attr_validate");
			return MNL_CB_ERROR;
		}
		break;
	case RTA_METRICS:
		if (mnl_attr_validate(attr, MNL_TYPE_NESTED) < 0) {
			logErr("mnl_attr_validate");
			return MNL_CB_ERROR;
		}
		break;
	}
	tb[type] = attr;
	return MNL_CB_OK;
}


static int data_cb_new(const struct nlmsghdr *nlh, void *data)
{
	struct nlattr *tb[RTA_MAX+1] = {};
	struct rtmsg *rm = static_cast<struct rtmsg*>(mnl_nlmsg_get_payload(nlh));
	mnl_attr_parse(nlh, sizeof(*rm), data_ipv4_attr_cb, tb);

	vector<vector<RoutingTable::route>>* routes =
		static_cast<vector<vector<RoutingTable::route>>*>(data);

	if(rm->rtm_type != 1){
		return MNL_CB_OK;
	}

	RoutingTable::route new_route;
	new_route.prefix_length = rm->rtm_dst_len;

	if (tb[RTA_DST]) {
		uint32_t *addr = static_cast<uint32_t*>(mnl_attr_get_payload(tb[RTA_DST]));
		new_route.base = ntohl(*addr);
	} else {
		new_route.base = 0;
	}

	/*
	if (tb[RTA_OIF]) {
		new_route.interface->netlinkIndex = mnl_attr_get_u32(tb[RTA_OIF]);
	}
	*/

	if (!tb[RTA_OIF]){
		fatal("route has no netlink index");
	}
	new_route.interface->netlinkIndex = mnl_attr_get_u32(tb[RTA_OIF]);


	if (tb[RTA_GATEWAY]) {
		uint32_t* next_hop = static_cast<uint32_t*>(mnl_attr_get_payload(tb[RTA_GATEWAY]));
		new_route.next_hop = ntohl(*next_hop);
	} else {
		new_route.next_hop = 0;
	}

	stringstream stream;
	stream << "new route " << ip_to_str(new_route.base) << "/" << new_route.prefix_length
		<< " via " << ip_to_str(new_route.next_hop)
		<< " iface " << new_route.interface;
	logInfo(stream.str());

	(*routes)[new_route.prefix_length].push_back(new_route);

	return MNL_CB_OK;
}

LinuxTable::LinuxTable(std::vector<std::shared_ptr<Interface>> ifaces){
	setInterfaces(ifaces);
	update();
};


void LinuxTable::updateInfo(){
	vector<vector<RoutingTable::route>> new_entries(33);

	struct mnl_socket *nl;
	char buf[MNL_SOCKET_BUFFER_SIZE];
	struct nlmsghdr *nlh;
	struct rtmsg *rtm;
	int ret;
	unsigned int seq, portid;

	nlh = mnl_nlmsg_put_header(buf);
	nlh->nlmsg_type = RTM_GETROUTE;
	nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
	nlh->nlmsg_seq = seq = time(NULL);
	rtm = static_cast<struct rtmsg*>(mnl_nlmsg_put_extra_header(nlh, sizeof(struct rtmsg)));

	rtm->rtm_family = AF_INET;

	nl = mnl_socket_open(NETLINK_ROUTE);
	if (nl == NULL) {
		fatal("mnl_socket_open() failed");
	}

	if (mnl_socket_bind(nl, 0, MNL_SOCKET_AUTOPID) < 0) {
		fatal("mnl_socket_bind() failed");
	}
	portid = mnl_socket_get_portid(nl);

	if (mnl_socket_sendto(nl, nlh, nlh->nlmsg_len) < 0) {
		fatal("mnl_socket_sendto() failed");
	}

	ret = mnl_socket_recvfrom(nl, buf, sizeof(buf));
	while (ret > 0) {
		ret = mnl_cb_run(buf, ret, seq, portid, data_cb_new, static_cast<void*>(&new_entries));
		if (ret <= MNL_CB_STOP)
			break;
		ret = mnl_socket_recvfrom(nl, buf, sizeof(buf));
	}
	if (ret == -1) {
		fatal("mnl_socket_recvfrom() failed");
	}

	mnl_socket_close(nl);

	// Map to the correct ifaces
	for(auto& a : new_entries){
		for(auto& r : a){
			for(auto iface : interfaces){
				if(r.interface->netlinkIndex == iface->netmapIndex){
					r.interface = iface;
					break;
				}
			}
		}
	};

	swap(new_entries, *entries);
}
