

/*================================================================
 *
 *
 *   文件名称：power_manager_group_policy_handler.c
 *   创 建 者：肖飞
 *   创建日期：2021年11月30日 星期二 15时07分16秒
 *   修改日期：2022年02月11日 星期五 19时55分18秒
 *   描    述：
 *
 *================================================================*/
#include "power_manager.h"

#include "log.h"

static int _init(void *_power_manager_info)
{
	int ret = 0;

	return ret;
}

static int _deinit(void *_power_manager_info)
{
	int ret = 0;

	return ret;
}

static int _channel_start(void *_power_manager_channel_info)
{
	int ret = 0;
	return ret;
}

static int _channel_charging(void *_power_manager_channel_info)
{
	int ret = 0;
	return ret;
}

static int _free(void *_power_manager_group_info)
{
	int ret = 0;

	return ret;
}

static int _assign(void *_power_manager_group_info)
{
	int ret = 0;
	return ret;
}

static int _config(void *_power_manager_group_info)
{
	int ret = 0;
	return ret;
}

static int _sync(void *_power_manager_group_info)
{
	int ret = 0;
	return ret;
}

static power_manager_group_policy_handler_t power_manager_group_policy_handler_0 = {
	.policy = 0,
	.init = _init,
	.deinit = _deinit,
	.channel_start = _channel_start,
	.channel_charging = _channel_charging,
	.free = _free,
	.assign = _assign,
	.config = _config,
	.sync = _sync,
};

static power_manager_group_policy_handler_t *power_manager_group_policy_handler_sz[] = {
	&power_manager_group_policy_handler_0,
};

power_manager_group_policy_handler_t *get_power_manager_group_policy_handler(uint8_t policy)
{
	int i;
	power_manager_group_policy_handler_t *power_manager_group_policy_handler = NULL;

	for(i = 0; i < ARRAY_SIZE(power_manager_group_policy_handler_sz); i++) {
		power_manager_group_policy_handler_t *power_manager_group_policy_handler_item = power_manager_group_policy_handler_sz[i];

		if(power_manager_group_policy_handler_item->policy == policy) {
			power_manager_group_policy_handler = power_manager_group_policy_handler_item;
		}
	}

	return power_manager_group_policy_handler;
}
