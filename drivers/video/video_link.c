// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2020 NXP
 *
 */

#include <common.h>
#include <linux/errno.h>

#include <dm.h>
#include <dm/uclass-internal.h>
#include <dm/device-internal.h>
#include <dm/ofnode.h>
#include <dm/read.h>
#include <video.h>
#include <panel.h>

struct of_endpoint {
	unsigned int port;
	unsigned int id;
	ofnode local_node;
};

#define MAX_LINKS 3
#define MAX_LINK_DEVICES 5

struct video_link {
	struct udevice *link_devs[MAX_LINK_DEVICES];
	int dev_num;
};

struct video_link video_links[MAX_LINKS];
struct video_link temp_stack;
ulong video_links_num = 0;
ulong curr_video_link = 0;
bool video_off = false;

ofnode ofnode_get_child_by_name(ofnode parent, const char *name)
{
	ofnode child;
	const char *child_name;

	for (child = ofnode_first_subnode(parent);
	     ofnode_valid(child);
	     child = ofnode_next_subnode(child)) {

		child_name = ofnode_get_name(child);

		if (!strncmp(child_name, name, strlen(name))) {
			break;
		}

	}
	return child;
}

ofnode ofnode_graph_get_next_endpoint(ofnode parent,
					ofnode prev)
{
	ofnode endpoint;
	ofnode port;
	const char *name;


	if (!ofnode_valid(prev)) {
		ofnode node;

		node = ofnode_find_subnode(parent, "ports");
		if (ofnode_valid(node))
			parent = node;

		port = ofnode_get_child_by_name(parent, "port");
		if (!ofnode_valid(port)) {
			debug("no port node found in 0x%lx\n", parent.of_offset);
			return ofnode_null();
		}

		endpoint = ofnode_first_subnode(port);
		if (ofnode_valid(endpoint)) {
			debug("get next endpoint %s\n", ofnode_get_name(endpoint));
			return endpoint;
		}
	} else {
		port = ofnode_get_parent(prev);
		endpoint = ofnode_next_subnode(prev);
		if (ofnode_valid(endpoint)) {
			debug("get next endpoint %s\n", ofnode_get_name(endpoint));
			return endpoint;
		}
	}

	debug("port %s\n", ofnode_get_name(port));

	while (1) {
		do {
			port = ofnode_next_subnode(port);
			if (!ofnode_valid(port))
				return ofnode_null();

			name = ofnode_get_name(port);
		} while (strncmp(name, "port", 4));

		/*
		 * Now that we have a port node, get the next endpoint by
		 * getting the next child. If the previous endpoint is NULL this
		 * will return the first child.
		 */
		endpoint = ofnode_first_subnode(port);
		if (ofnode_valid(endpoint)) {
			debug("get next endpoint %s\n", ofnode_get_name(endpoint));
			return endpoint;
		}
	}

	return ofnode_null();
}

#define for_each_endpoint_of_node(parent, child) \
	for (child = ofnode_graph_get_next_endpoint(parent, ofnode_null()); ofnode_valid(child); \
	     child = ofnode_graph_get_next_endpoint(parent, child))


int ofnode_graph_get_endpoint_count(ofnode node)
{
	ofnode endpoint;
	int num = 0;

	for_each_endpoint_of_node(node, endpoint)
		num++;

	return num;
}

int ofnode_graph_parse_endpoint(ofnode node,
			    struct of_endpoint *endpoint)
{
	ofnode port_node = ofnode_get_parent(node);

	memset(endpoint, 0, sizeof(*endpoint));

	endpoint->local_node = node;
	/*
	 * It doesn't matter whether the two calls below succeed.
	 * If they don't then the default value 0 is used.
	 */
	ofnode_read_u32(port_node, "reg", &endpoint->port);
	ofnode_read_u32(node, "reg", &endpoint->id);

	return 0;
}

ofnode ofnode_graph_get_endpoint_by_regs(
	const ofnode parent, int port_reg, int reg)
{
	struct of_endpoint endpoint;
	ofnode node;

	for_each_endpoint_of_node(parent, node) {
		ofnode_graph_parse_endpoint(node, &endpoint);
		if (((port_reg == -1) || (endpoint.port == port_reg)) &&
			((reg == -1) || (endpoint.id == reg))) {
			debug("get node %s\n", ofnode_get_name(node));

			return node;
		}
	}

	return ofnode_null();
}

ofnode ofnode_graph_get_remote_endpoint(ofnode node)
{
	ofnode remote;
	u32 phandle;
	int ret;

	ret = ofnode_read_u32(node, "remote-endpoint", &phandle);
	if (ret) {
		printf("required remote-endpoint property isn't provided\n");
		return ofnode_null();
	}

	remote = ofnode_get_by_phandle(phandle);
	if (!ofnode_valid(remote)) {
		printf("failed to find remote-endpoint\n");
		return ofnode_null();
	}

	return remote;
}

ofnode ofnode_graph_get_port_parent(ofnode node)
{
	unsigned int depth;

	if (!ofnode_valid(node))
		return ofnode_null();

	/*
	 * Preserve usecount for passed in node as of_get_next_parent()
	 * will do of_node_put() on it.
	 */

	/* Walk 3 levels up only if there is 'ports' node. */
	for (depth = 3; depth && ofnode_valid(node); depth--) {
		node = ofnode_get_parent(node);
		const char *name = ofnode_get_name(node);
		if (depth == 2 && strcmp(name, "ports"))
			break;
	}
	return node;
}

ofnode ofnode_graph_get_remote_port_parent(ofnode node)
{
	ofnode np, pp;

	/* Get remote endpoint node. */
	np = ofnode_graph_get_remote_endpoint(node);

	pp = ofnode_graph_get_port_parent(np);

	return pp;
}

int find_device_by_ofnode(ofnode node, struct udevice **pdev)
{
	int ret;

	if (!ofnode_is_available(node))
		return -2;

	ret = uclass_find_device_by_ofnode(UCLASS_DISPLAY, node, pdev);
	if (!ret)
		return 0;

	ret = uclass_find_device_by_ofnode(UCLASS_DSI_HOST, node, pdev);
	if (!ret)
		return 0;

	ret = uclass_find_device_by_ofnode(UCLASS_VIDEO_BRIDGE, node, pdev);
	if (!ret)
		return 0;

	ret = uclass_find_device_by_ofnode(UCLASS_PANEL, node, pdev);
	if (!ret)
		return 0;

	return -1;
}

static void video_link_stack_push(struct udevice *dev)
{
	if (temp_stack.dev_num < MAX_LINK_DEVICES) {
		temp_stack.link_devs[temp_stack.dev_num] = dev;
		temp_stack.dev_num++;
	}
}

static void video_link_stack_pop(void)
{
	if (temp_stack.dev_num > 0) {
		temp_stack.link_devs[temp_stack.dev_num] = NULL;
		temp_stack.dev_num--;
	}
}

static int duplicate_video_link(void)
{
	if (video_links_num < MAX_LINKS) {
		video_links[video_links_num] = temp_stack;
		video_links_num++;

		debug("duplicate links num %lu,  temp_stack num %d\n",
			video_links_num, temp_stack.dev_num);
		return 0;
	}

	return -ENODEV;
}

static void video_link_add_node(struct udevice *peer_dev, struct udevice *dev, ofnode dev_node)
{
	int ret = 0;
	ofnode remote, endpoint_node;
	struct udevice *remote_dev;
	bool find = false;

	debug("endpoint cnt %d\n", ofnode_graph_get_endpoint_count(dev_node));

	video_link_stack_push(dev);

	for_each_endpoint_of_node(dev_node, endpoint_node) {
		remote = ofnode_graph_get_remote_port_parent(endpoint_node);
		if (!ofnode_valid(remote))
			continue;

		debug("remote %s\n", ofnode_get_name(remote));
		ret = find_device_by_ofnode(remote, &remote_dev);
		if (!ret) {
			debug("remote dev %s\n", remote_dev->name);

			if (peer_dev && peer_dev == remote_dev)
				continue;

			/* it is possible that ofnode of remote_dev is not equal to remote */
			video_link_add_node(dev, remote_dev, remote);

			find = true;
		}
	}

	/* leaf node or no valid new endpoint, now copy the entire stack to a new video link */
	if (!find) {
		ret = duplicate_video_link();
		if (ret)
			printf("video link is full\n");
	}

	video_link_stack_pop();
}

struct udevice *video_link_get_next_device(struct udevice *curr_dev)
{
	int i, ret;

	if (video_off)
		return NULL;

	if (curr_video_link >= video_links_num) {
		printf("current video link is not correct\n");
		return NULL;
	}

	for (i = 0; i < video_links[curr_video_link].dev_num; i++) {
		if (video_links[curr_video_link].link_devs[i] == curr_dev) {
			if ((i + 1) < video_links[curr_video_link].dev_num) {
				ret = device_probe(video_links[curr_video_link].link_devs[i + 1]);
				if (ret) {
					printf("probe device is failed, ret %d\n", ret);
					return NULL;
				}

				return video_links[curr_video_link].link_devs[i + 1];
			} else {
				debug("fail to find next device, already last one\n");
				return NULL;
			}
		}
	}

	return NULL;
}

struct udevice *video_link_get_video_device(void)
{
	int ret;
	if (video_off)
		return NULL;

	if (curr_video_link >= video_links_num)
		return NULL;

	if (video_links[curr_video_link].dev_num == 0)
		return NULL;

	ret = device_probe(video_links[curr_video_link].link_devs[0]);
	if (ret) {
		printf("probe video device failed, ret %d\n", ret);
		return NULL;
	}

	return video_links[curr_video_link].link_devs[0];
}

int video_link_get_display_timings(struct display_timing *timings)
{
	int i = 0;
	int ret;
	struct udevice *dev;

	if (video_off)
		return -EPERM;

	if (curr_video_link >= video_links_num)
		return -ENODEV;

	if (video_links[curr_video_link].dev_num == 0)
		return -ENODEV;

	for (i = video_links[curr_video_link].dev_num - 1; i >= 0 ; i--) {
		dev = video_links[curr_video_link].link_devs[i];
		if (device_get_uclass_id(dev) == UCLASS_PANEL) {
			ret = device_probe(video_links[curr_video_link].link_devs[i]);
			if (ret) {
				printf("fail to probe panel device %s\n", dev->name);
				return ret;
			}

			ret = panel_get_display_timing(dev, timings);
			if (ret) {
				ret = ofnode_decode_display_timing(dev_ofnode(dev), 0, timings);
				if (ret) {
					printf("fail to get panel timing %s\n", dev->name);
					return ret;
				}
			}

			return 0;
		} else if (device_get_uclass_id(dev) == UCLASS_DISPLAY ||
			device_get_uclass_id(dev) == UCLASS_VIDEO) {

			ret = ofnode_decode_display_timing(dev_ofnode(dev), 0, timings);
			if (!ret)
				return 0;
		}
	}

	return -EINVAL;
}

static void list_videolink(bool current_only)
{
	ulong index = 0;
	int j;
	bool match;

	/* dump the link */
	debug("video link number: %lu\n", video_links_num);

	for (index = 0; index < video_links_num; index ++) {
		match = false;
		if (curr_video_link == index)
			match = true;
		else if (current_only)
			continue;

		printf("[%c]-Video Link %lu", (match)? '*':' ', index);

		if (match) {
			struct udevice *video_dev = video_link_get_video_device();
			if (video_dev) {
				printf(" (%u x %u)", video_get_xsize(video_dev),
					video_get_ysize(video_dev));
			}
		}

		printf("\n");

		for (j = 0; j < video_links[index].dev_num; j++) {
			printf("\t[%d] %s, %s\n", j, video_links[index].link_devs[j]->name,
				dev_get_uclass_name(video_links[index].link_devs[j]));
		}
	}
}

static int do_videolink(cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[])
{
	char cmd = 'l';
	int ret = 0;

	if (argc > 1)
		cmd = argv[1][0];

	switch (cmd) {
	case 'l':		/* list */
		list_videolink(false);
		break;
	default:
		ret = CMD_RET_USAGE;
		break;
	}

	return ret;
}

int video_link_init(void)
{
	struct udevice *dev;
	ulong env_id;
	int off;
	memset(&video_links, 0, sizeof(video_links));
	memset(&temp_stack, 0, sizeof(temp_stack));

	for (uclass_find_first_device(UCLASS_VIDEO, &dev);
	     dev;
	     uclass_find_next_device(&dev)) {

		video_link_add_node(NULL, dev, dev_ofnode(dev));
	}

	if (video_links_num == 0) {
		printf("Fail to setup video link\n");
		return -ENODEV;
	}

	/* Read the env variable for default video link */
	off = env_get_yesno("video_off");
	if (off == 1) {
		video_off = true;
		return 0;
	}

	env_id = env_get_ulong("video_link", 10, 0);
	if (env_id < video_links_num)
		curr_video_link = env_id;

	list_videolink(true);

	return 0;
}

int video_link_shut_down(void)
{
	struct udevice *video_dev = video_link_get_video_device();

	if (video_dev)
		device_remove(video_dev, DM_REMOVE_NORMAL);

	return 0;
}

#ifdef CONFIG_SYS_LONGHELP
static char video_link_help_text[] =
	"list \n"
	"    -  show video link info, set video_link variable to select link";
#endif

U_BOOT_CMD(
	videolink,	5,	1,	do_videolink,
	"list and select video link", video_link_help_text
);
