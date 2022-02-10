

/*================================================================
 *
 *
 *   文件名称：uart_debug_handler.c
 *   创 建 者：肖飞
 *   创建日期：2020年05月13日 星期三 13时18分00秒
 *   修改日期：2021年07月28日 星期三 15时43分21秒
 *   描    述：
 *
 *================================================================*/
#include "uart_debug_handler.h"
#include "channels.h"
#include <stdio.h>
#include <string.h>

#include "lwip/netdb.h"
#include "lwip/inet.h"
#include "lwip/netif.h"

#define LOG_UART
#include "log.h"

extern struct netif gnetif;

static void fn1(char *arguments)
{
	_printf("%s:%s:%d arguments:\'%s\'\n", __FILE__, __func__, __LINE__, arguments);
}

static void fn2(char *arguments)
{
	int ret;
	char *content = (char *)arguments;
	channels_config_t *channels_config = get_channels_config(0);
	channels_info_t *channels_info;
	int channel;
	int type;
	int catched;

	if(channels_config == NULL) {
		debug("");
		return;
	}

	channels_info = get_or_alloc_channels_info(channels_config);

	if(channels_info == NULL) {
		debug("");
		return;
	}

	ret = sscanf(content, "%d %d%n", &channel, &type, &catched);

	if(ret == 2) {
		debug("channel:%d, type:%d!", channel, type);
	} else {
		debug("\'%s\'", content);
		return;
	}

	start_stop_channel(channels_info, channel, type);
}

static void fn3(char *arguments)
{
	start_dump_channels_stats();
}

static int p_host(struct hostent *ent)
{
	int ret = 0;
	char **cp;

	if(ent == NULL) {
		ret = -1;
		return ret;
	}

	_printf("\n");

	_printf("h_name:%s\n", ent->h_name);
	_printf("h_aliases:\n");
	cp = ent->h_aliases;

	while(*cp != NULL) {
		_printf("%s\n", *cp);
		cp += 1;

		if(*cp != NULL) {
			//_printf(", ");
		}
	}

	_printf("h_addrtype:%d\n", ent->h_addrtype);

	_printf("h_length:%d\n", ent->h_length);

	_printf("h_addr_list:\n");
	cp = ent->h_addr_list;

	while(*cp != NULL) {
		_printf("%s\n", inet_ntoa(**cp));
		cp += 1;

		if(*cp != NULL) {
			//_printf(", ");
		}
	}

	return ret;
}

#define RECV_BUFFER_SIZE 128

static void get_host_by_name(char *content, uint32_t size)
{
	struct hostent *ent;
	char *hostname = (char *)os_alloc(RECV_BUFFER_SIZE);
	int ret;
	int catched;

	_hexdump("content", (const char *)content, size);

	if(hostname == NULL) {
		return;
	}

	hostname[0] = 0;

	ret = sscanf(content, "%s%n", hostname, &catched);

	if(ret == 1) {
		_printf("hostname:%s!\n", hostname);
		ent = gethostbyname(hostname);
		p_host(ent);
	} else {
		_printf("no hostname!\n");
	}

	os_free(hostname);
}

static void fn4(char *arguments)
{
	_printf("local host ip:%s\n", inet_ntoa(gnetif.ip_addr));

	get_host_by_name(arguments, strlen(arguments));
}


static void fn5(char *arguments)
{
	int size = xPortGetFreeHeapSize();
	uint8_t *os_thread_info;
	uint8_t is_app = 0;
	uint32_t ticks = osKernelSysTick();

#if defined(USER_APP)
	is_app = 1;
#endif

	_printf("free heap size:%d\n", size);
	_printf("current ticks:%lu\n", ticks);
	_printf("%lu day %lu hour %lu min %lu sec\n",
	        ticks / (1000 * 60 * 60 * 24),//day
	        (ticks % (1000 * 60 * 60 * 24)) / (1000 * 60 * 60),//hour
	        (ticks % (1000 * 60 * 60)) / (1000 * 60),//min
	        (ticks % (1000 * 60)) / (1000)//sec
	       );

	if(size < 4 * 1024) {
		return;
	}

	size = 1024;

	os_thread_info = (uint8_t *)os_alloc(size);

	if(os_thread_info == NULL) {
		return;
	}

	osThreadList(os_thread_info);

	_puts((const char *)os_thread_info);

	os_free(os_thread_info);

	if(is_app) {
		_printf("in app!\n");
	} else {
		_printf("in bootloader!\n");
	}
}

static uart_fn_item_t uart_fn_map[] = {
	{1, fn1},
	{2, fn2},
	{3, fn3},
	{4, fn4},
	{5, fn5},
};

uart_fn_map_info_t uart_fn_map_info = {
	.uart_fn_map = uart_fn_map,
	.uart_fn_map_size = sizeof(uart_fn_map) / sizeof(uart_fn_item_t),
};
