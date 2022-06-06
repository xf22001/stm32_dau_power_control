

/*================================================================
 *
 *
 *   文件名称：power_manager_group_policy_handler.c
 *   创 建 者：肖飞
 *   创建日期：2021年11月30日 星期二 15时07分16秒
 *   修改日期：2022年06月06日 星期一 22时59分48秒
 *   描    述：
 *
 *================================================================*/
#include "power_manager.h"

#include "relay_boards_comm_proxy_remote.h"
#include "channels_comm_proxy.h"

#include "log.h"

static int init_average(void *_power_manager_info)
{
	int ret = 0;

	return ret;
}

static int deinit_average(void *_power_manager_info)
{
	int ret = 0;

	return ret;
}

static int channel_start_average(void *_power_manager_channel_info)
{
	int ret = 0;
	power_manager_channel_info_t *power_manager_channel_info = (power_manager_channel_info_t *)_power_manager_channel_info;

	power_manager_channel_info->status.reassign_power_module_group_number = 0;
	return ret;
}

static int channel_charging_average(void *_power_manager_channel_info)
{
	int ret = 0;
	return ret;
}

static void channel_info_deactive_power_module_group(power_manager_channel_info_t *power_manager_channel_info)
{
	struct list_head *pos;
	struct list_head *n;
	struct list_head *head;
	power_manager_group_info_t *power_manager_group_info = (power_manager_group_info_t *)power_manager_channel_info->power_manager_group_info;

	head = &power_manager_channel_info->power_module_group_list;

	list_for_each_safe(pos, n, head) {
		power_module_group_info_t *power_module_group_info = list_entry(pos, power_module_group_info_t, list);
		power_module_item_info_t *power_module_item_info;
		struct list_head *head1 = &power_module_group_info->power_module_item_list;
		list_for_each_entry(power_module_item_info, head1, power_module_item_info_t, list) {
			power_module_item_info->status.state = POWER_MODULE_ITEM_STATE_PREPARE_DEACTIVE;
		}
		list_move_tail(&power_module_group_info->list, &power_manager_group_info->power_module_group_deactive_list);
	}
}

static void free_power_module_group_for_stop_channel(power_manager_group_info_t *power_manager_group_info)
{
	power_manager_channel_info_t *power_manager_channel_info;
	struct list_head *head;

	head = &power_manager_group_info->channel_deactive_list;

	list_for_each_entry(power_manager_channel_info, head, power_manager_channel_info_t, list) {
		channel_info_deactive_power_module_group(power_manager_channel_info);
	}
}

//要启动的通道数
static uint8_t get_start_channel_count(power_manager_group_info_t *power_manager_group_info)
{
	uint8_t count = 0;
	power_manager_channel_info_t *power_manager_channel_info;
	struct list_head *head;

	head = &power_manager_group_info->channel_active_list;

	list_for_each_entry(power_manager_channel_info, head, power_manager_channel_info_t, list) {
		if(power_manager_channel_info->status.state == POWER_MANAGER_CHANNEL_STATE_PREPARE_START) {
			count++;
		}
	}
	return count;
}

static uint8_t get_disable_power_group_count(power_manager_group_info_t *power_manager_group_info)
{
	uint8_t count = 0;

	count = list_size(&power_manager_group_info->power_module_group_disable_list);

	return count;
}

static void cancel_last_start_channel(power_manager_group_info_t *power_manager_group_info)
{
	power_manager_channel_info_t *power_manager_channel_info;
	struct list_head *head;

	head = &power_manager_group_info->channel_active_list;

	power_manager_channel_info = list_last_entry(head, power_manager_channel_info_t, list);

	list_move_tail(&power_manager_channel_info->list, &power_manager_group_info->channel_idle_list);
}

static uint8_t get_available_power_group_count(power_manager_group_info_t *power_manager_group_info)
{
	uint8_t count = 0;

	//计算空闲的模块组数
	count += list_size(&power_manager_group_info->power_module_group_idle_list);

	//计算将要被回收的模块组数
	count += list_size(&power_manager_group_info->power_module_group_deactive_list);

	return count;
}

static void channel_free_power_module_group(power_manager_channel_info_t *power_manager_channel_info, uint8_t channel_free_power_module_group_count)
{
	struct list_head *pos;
	struct list_head *n;
	struct list_head *head;
	power_manager_group_info_t *power_manager_group_info = power_manager_channel_info->power_manager_group_info;

	if(channel_free_power_module_group_count == 0) {
		return;
	}

	head = &power_manager_channel_info->power_module_group_list;

	list_for_each_safe(pos, n, head) {
		power_module_group_info_t *power_module_group_info = list_entry(pos, power_module_group_info_t, list);
		power_module_item_info_t *power_module_item_info;
		struct list_head *head1 = &power_module_group_info->power_module_item_list;
		list_for_each_entry(power_module_item_info, head1, power_module_item_info_t, list) {
			power_module_item_info->status.state = POWER_MODULE_ITEM_STATE_PREPARE_DEACTIVE;
		}
		list_move_tail(&power_module_group_info->list, &power_manager_group_info->power_module_group_deactive_list);
		channel_free_power_module_group_count--;

		if(channel_free_power_module_group_count == 0) {
			return;
		}
	}
}

static void active_channel_list_remove_power_module_group(power_manager_group_info_t *power_manager_group_info, uint8_t more_power_module_group_count_need, uint8_t keep_count)
{
	power_manager_channel_info_t *power_manager_channel_info;
	struct list_head *head;

	head = &power_manager_group_info->channel_active_list;

	if(more_power_module_group_count_need == 0) {
		return;
	}

	list_for_each_entry_reverse(power_manager_channel_info, head, power_manager_channel_info_t, list) {
		uint8_t power_module_group_count;
		uint8_t channel_free_power_module_group_count;

		power_module_group_count = list_size(&power_manager_channel_info->power_module_group_list);

		if(power_module_group_count <= keep_count) {//没有可以剔除的模块组
			continue;
		}

		channel_free_power_module_group_count = power_module_group_count - keep_count;

		if(channel_free_power_module_group_count > more_power_module_group_count_need) {
			channel_free_power_module_group_count = more_power_module_group_count_need;
		}

		if(channel_free_power_module_group_count > 0) {
			channel_free_power_module_group(power_manager_channel_info, channel_free_power_module_group_count);
			more_power_module_group_count_need -= channel_free_power_module_group_count;
		}

		if(more_power_module_group_count_need == 0) {
			break;
		}
	}

	if(more_power_module_group_count_need != 0) {
		debug("bug!!! more_power_module_group_count_need:%d", more_power_module_group_count_need);
	}
}


static int free_average(void *_power_manager_group_info)
{
	int ret = -1;
	power_manager_group_info_t *power_manager_group_info = (power_manager_group_info_t *)_power_manager_group_info;

	//充电中的枪数
	uint8_t active_channel_count;
	//要启动的枪数
	uint8_t start_channel_count;
	//所有正常模块组数
	uint8_t all_enable_power_module_group_count;
	//平均每个通道可以用的模块组数
	uint8_t average_power_module_group_per_channel;
	//可用的模块组数
	uint8_t available_power_module_group_count;
	//需要从现在充电的枪中剔除多少个模块组
	uint8_t more_power_module_group_count_need;

	//释放所有即将停止充电的枪关联的模块组
	free_power_module_group_for_stop_channel(power_manager_group_info);

	//获取需要充电的枪数
	active_channel_count = list_size(&power_manager_group_info->channel_active_list);
	debug("active_channel_count:%d", active_channel_count);

	if(active_channel_count == 0) {//如果没有枪需要充电，到这里模块组释放完毕
		ret = 0;
		return ret;
	}

	//获取启动充电的枪数
	start_channel_count = get_start_channel_count(power_manager_group_info);
	debug("start_channel_count:%d", start_channel_count);

	if(start_channel_count == 0) {//如果没有要启动的枪，到这里模块组释放完毕
		ret = 0;
		return ret;
	}

	//获取所有可用模块组数
	all_enable_power_module_group_count = power_manager_group_info->power_module_group_number - get_disable_power_group_count(power_manager_group_info);
	debug("all_enable_power_module_group_count:%d", all_enable_power_module_group_count);

	if(all_enable_power_module_group_count == 0) {
		debug("bug!!!");
		return ret;
	}

	//计算平均每个需要充电的枪使用的模块组数
	average_power_module_group_per_channel = all_enable_power_module_group_count / active_channel_count;
	debug("average_power_module_group_per_channel:%d", average_power_module_group_per_channel);

	if(average_power_module_group_per_channel == 0) {//模块组不够分了，停掉最后一个要启动的通道
		cancel_last_start_channel(power_manager_group_info);
		return ret;
	}

	//计算剩余模块组数
	available_power_module_group_count = get_available_power_group_count(power_manager_group_info);
	debug("available_power_module_group_count:%d", available_power_module_group_count);

	//计算至少需要从正在充电枪中释放多少个模块组
	more_power_module_group_count_need = 0;

	if(start_channel_count * average_power_module_group_per_channel > available_power_module_group_count) {
		more_power_module_group_count_need = start_channel_count * average_power_module_group_per_channel - available_power_module_group_count;
	} else {//不需要再释放模块组，到这里模块组释放完毕
		ret = 0;
		return ret;
	}

	debug("more_power_module_group_count_need:%d", more_power_module_group_count_need);

	//从正在充电的枪中释放模块组
	active_channel_list_remove_power_module_group(power_manager_group_info, more_power_module_group_count_need, average_power_module_group_per_channel);

	ret = 0;
	return ret;
}

static void channel_info_assign_power_module_group(power_manager_channel_info_t *power_manager_channel_info, uint8_t count)
{
	power_manager_group_info_t *power_manager_group_info = power_manager_channel_info->power_manager_group_info;
	struct list_head *pos;
	struct list_head *n;
	struct list_head *head;
	struct list_head *head1;
	//uint32_t ticks = osKernelSysTick();

	if(count == 0) {
		return;
	}

	head = &power_manager_group_info->power_module_group_idle_list;

	list_for_each_safe(pos, n, head) {
		power_module_group_info_t *power_module_group_info = list_entry(pos, power_module_group_info_t, list);
		power_manager_relay_board_info_t *power_manager_relay_board_info;
		power_module_item_info_t *power_module_item_info;

		head1 = &power_module_group_info->power_module_item_list;
		list_for_each_entry(power_module_item_info, head1, power_module_item_info_t, list) {
			if(power_module_item_info->status.state != POWER_MODULE_ITEM_STATE_IDLE) {
				debug("power module state is not idle:%s!!!", get_power_module_item_state_des(power_module_item_info->status.state));
			}

			power_module_item_info->status.state = POWER_MODULE_ITEM_STATE_PREPARE_ACTIVE;
			//power_module_item_info->test_stamp = ticks;
		}
		power_module_group_info->power_manager_channel_info = power_manager_channel_info;
		list_move_tail(&power_module_group_info->list, &power_manager_channel_info->power_module_group_list);
		debug("assign module group_id %d to channel_id %d", power_module_group_info->id, power_manager_channel_info->id);

		head1 = &power_manager_channel_info->relay_board_list;
		list_for_each_entry(power_manager_relay_board_info, head1, power_manager_relay_board_info_t, list) {
			uint8_t power_module_group = power_module_group_info->id;

			if(power_module_group < power_manager_relay_board_info->offset) {
				continue;
			}

			if(power_module_group >= power_manager_relay_board_info->offset + power_manager_relay_board_info->number) {
				continue;
			}

			//xiaofei clear power_module_group_info->power_manager_relay_board_info
			power_module_group_info->power_manager_relay_board_info = power_manager_relay_board_info;
		}

		count--;

		if(count == 0) {
			break;
		}
	}

	if(count != 0) {
		debug("bug!!!");
	}
}

static void active_power_manager_group_info_power_module_group_assign(power_manager_group_info_t *power_manager_group_info, uint8_t average_count, uint8_t available_count)
{
	power_manager_channel_info_t *power_manager_channel_info;
	struct list_head *head;

	head = &power_manager_group_info->channel_active_list;

	list_for_each_entry(power_manager_channel_info, head, power_manager_channel_info_t, list) {
		uint8_t power_module_group_count = list_size(&power_manager_channel_info->power_module_group_list);

		if(power_module_group_count >= average_count) {
			continue;
		}

		power_module_group_count = average_count - power_module_group_count;

		if(power_module_group_count > available_count) {
			power_module_group_count = available_count;
		}

		channel_info_assign_power_module_group(power_manager_channel_info, power_module_group_count);
		available_count -= power_module_group_count;

		if(available_count == 0) {
			break;
		}
	}

	if(available_count == 0) {
		return;
	}

	list_for_each_entry(power_manager_channel_info, head, power_manager_channel_info_t, list) {
		channel_info_assign_power_module_group(power_manager_channel_info, 1);
		available_count -= 1;

		if(available_count == 0) {
			break;
		}
	}
}

static int assign_average(void *_power_manager_group_info)
{
	int ret = 0;
	power_manager_group_info_t *power_manager_group_info = (power_manager_group_info_t *)_power_manager_group_info;
	//充电中的枪数
	uint8_t active_channel_count;
	//所有正常模块组数
	uint8_t all_enable_power_module_group_count;
	//平均每个通道可以用的模块组数
	uint8_t average_power_module_group_per_channel;
	//可用的模块组数
	uint8_t available_power_module_group_count;

	//获取需要充电的枪数
	active_channel_count = list_size(&power_manager_group_info->channel_active_list);
	debug("active_channel_count:%d", active_channel_count);

	if(active_channel_count == 0) {//如果没有枪需要充电,不分配
		return ret;
	}

	//获取所有可用模块组数
	all_enable_power_module_group_count = power_manager_group_info->power_module_group_number - get_disable_power_group_count(power_manager_group_info);
	debug("all_enable_power_module_group_count:%d", all_enable_power_module_group_count);

	if(all_enable_power_module_group_count == 0) {
		debug("bug!!!");
		return ret;
	}

	//计算平均每个需要充电的枪使用的模块组数
	average_power_module_group_per_channel = all_enable_power_module_group_count / active_channel_count;
	debug("average_power_module_group_per_channel:%d", average_power_module_group_per_channel);

	//计算剩余模块组数
	available_power_module_group_count = get_available_power_group_count(power_manager_group_info);
	debug("available_power_module_group_count:%d", available_power_module_group_count);

	active_power_manager_group_info_power_module_group_assign(power_manager_group_info, average_power_module_group_per_channel, available_power_module_group_count);

	return ret;
}

static void print_relay_board_info(const uint8_t pad, power_manager_relay_board_info_t *power_manager_relay_board_info)
{
	//u_power_module_status_t u_power_module_status;
	char *_pad = (char *)os_calloc(1, pad + 1);
	power_manager_channel_info_t *power_manager_channel_info = (power_manager_channel_info_t *)power_manager_relay_board_info->power_manager_channel_info;

	OS_ASSERT(_pad != NULL);
	memset(_pad, '\t', pad);

	_printf("%srelay board:%d\n", _pad, power_manager_relay_board_info->id);
	_printf("%s\tchannel id:%d\n", _pad, power_manager_channel_info->id);
	_printf("%s\toffset:%d\n", _pad, power_manager_relay_board_info->offset);
	_printf("%s\tnumber:%d\n", _pad, power_manager_relay_board_info->number);
	_printf("%s\tconfig:0b%08x\n", _pad, u8_bin(power_manager_relay_board_info->config));
	_printf("%s\tremote_config:0b%08x\n", _pad, u8_bin(power_manager_relay_board_info->remote_config));
	_printf("%s\ttemperature1:%d\n", _pad, power_manager_relay_board_info->temperature1);
	_printf("%s\ttemperature2:%d\n", _pad, power_manager_relay_board_info->temperature2);

	os_free(_pad);
}

static int _config(void *_power_manager_group_info)
{
	int ret = 0;
	power_manager_group_info_t *power_manager_group_info = (power_manager_group_info_t *)_power_manager_group_info;
	power_manager_info_t *power_manager_info = (power_manager_info_t *)power_manager_group_info->power_manager_info;
	struct list_head *head;
	struct list_head *head1;
	int i;
	channels_info_t *channels_info = (channels_info_t *)power_manager_info->channels_info;

	for(i = 0; i < channels_info->channel_number; i++) {
		power_manager_channel_info_t *power_manager_channel_info = power_manager_info->power_manager_channel_info + i;
		power_manager_relay_board_info_t *power_manager_relay_board_info;
		power_module_group_info_t *power_module_group_info;

		if(power_manager_channel_info->power_manager_group_info != power_manager_group_info) {
			power_manager_group_info_t *power_manager_group_info_channel = (power_manager_group_info_t *)power_manager_channel_info->power_manager_group_info;
			debug("skip channel_id %d in power_manager_group_id %d", power_manager_channel_info->id, power_manager_group_info_channel->id);
			continue;
		}

		debug("channel_id:%d, state:%s",
		      power_manager_channel_info->id,
		      get_power_manager_channel_state_des(power_manager_channel_info->status.state));

		head = &power_manager_channel_info->relay_board_list;
		list_for_each_entry(power_manager_relay_board_info, head, power_manager_relay_board_info_t, list) {
			power_manager_relay_board_info->config = 0;
		}

		head = &power_manager_channel_info->power_module_group_list;
		list_for_each_entry(power_module_group_info, head, power_module_group_info_t, list) {
			head1 = &power_manager_channel_info->relay_board_list;
			list_for_each_entry(power_manager_relay_board_info, head1, power_manager_relay_board_info_t, list) {
				if(power_module_group_info->id < power_manager_relay_board_info->offset) {
					continue;
				}

				if(power_module_group_info->id >= power_manager_relay_board_info->offset + power_manager_relay_board_info->number) {
					continue;
				}

				power_manager_relay_board_info->config = set_u8_bits(power_manager_relay_board_info->config,
				        power_module_group_info->id - power_manager_relay_board_info->offset,
				        1);
				print_relay_board_info(2, power_manager_relay_board_info);
			}
		}

		head = &power_manager_channel_info->relay_board_list;
		list_for_each_entry(power_manager_relay_board_info, head, power_manager_relay_board_info_t, list) {
			print_relay_board_info(1, power_manager_relay_board_info);

			relay_boards_comm_update_config(channels_info, power_manager_relay_board_info->id, power_manager_relay_board_info->config);
		}
	}

	return ret;
}

static int _sync(void *_power_manager_group_info)
{
	int ret = 0;
	power_manager_group_info_t *power_manager_group_info = (power_manager_group_info_t *)_power_manager_group_info;
	power_manager_info_t *power_manager_info = (power_manager_info_t *)power_manager_group_info->power_manager_info;
	struct list_head *head;
	int i;
	channels_info_t *channels_info = (channels_info_t *)power_manager_info->channels_info;

	for(i = 0; i < channels_info->channel_number; i++) {
		power_manager_channel_info_t *power_manager_channel_info = power_manager_info->power_manager_channel_info + i;
		power_manager_relay_board_info_t *power_manager_relay_board_info;

		if(power_manager_channel_info->power_manager_group_info != power_manager_group_info) {
			power_manager_channel_info_t *power_manager_channel_info_channel = (power_manager_channel_info_t *)power_manager_channel_info->power_manager_group_info;
			debug("skip channel_id %d in power_manager_group_id %d", power_manager_channel_info->id, power_manager_channel_info_channel->id);
			continue;
		}

		head = &power_manager_channel_info->relay_board_list;
		list_for_each_entry(power_manager_relay_board_info, head, power_manager_relay_board_info_t, list) {
			if(power_manager_relay_board_info->config != power_manager_relay_board_info->remote_config) {
				debug("channel_id %d board_id %d config 0b%08x, remote config 0b%08x",
				      power_manager_channel_info->id,
				      power_manager_relay_board_info->id,
				      u8_bin(power_manager_relay_board_info->config),
				      u8_bin(power_manager_relay_board_info->remote_config));
				ret = -1;
				break;
			}
		}

		if(ret != 0) {
			break;
		}
	}

	return ret;
}

static power_manager_group_policy_handler_t power_manager_group_policy_handler_average = {
	.policy = POWER_MANAGER_GROUP_POLICY_AVERAGE,
	.init = init_average,
	.deinit = deinit_average,
	.channel_start = channel_start_average,
	.channel_charging = channel_charging_average,
	.free = free_average,
	.assign = assign_average,
	.config = _config,
	.sync = _sync,
};

static int init_priority(void *_power_manager_info)
{
	int ret = 0;

	return ret;
}

static int deinit_priority(void *_power_manager_info)
{
	int ret = 0;

	return ret;
}

static int channel_start_priority(void *_power_manager_channel_info)
{
	int ret = 0;
	power_manager_channel_info_t *power_manager_channel_info = (power_manager_channel_info_t *)_power_manager_channel_info;

	power_manager_channel_info->status.reassign_power_module_group_number = 1;
	return ret;
}

static int channel_charging_priority(void *_power_manager_channel_info)
{
	int ret = 0;
	return ret;
}

static void channel_info_deactive_unneeded_power_module_group_priority(power_manager_channel_info_t *power_manager_channel_info)
{
	struct list_head *pos;
	struct list_head *n;
	struct list_head *head;
	power_manager_group_info_t *power_manager_group_info = power_manager_channel_info->power_manager_group_info;
	uint8_t channel_power_module_group_size;
	uint8_t channel_power_module_group_to_remove_count = 0;

	head = &power_manager_channel_info->power_module_group_list;
	channel_power_module_group_size = list_size(head);

	debug("channel_id %d reassign_power_module_group_number:%d", power_manager_channel_info->id, power_manager_channel_info->status.reassign_power_module_group_number);

	if(channel_power_module_group_size <= power_manager_channel_info->status.reassign_power_module_group_number) {
		return;
	}

	channel_power_module_group_to_remove_count = channel_power_module_group_size - power_manager_channel_info->status.reassign_power_module_group_number;

	list_for_each_safe(pos, n, head) {
		power_module_group_info_t *power_module_group_info = list_entry(pos, power_module_group_info_t, list);
		power_module_item_info_t *power_module_item_info;
		struct list_head *head1 = &power_module_group_info->power_module_item_list;
		list_for_each_entry(power_module_item_info, head1, power_module_item_info_t, list) {
			power_module_item_info->status.state = POWER_MODULE_ITEM_STATE_PREPARE_DEACTIVE;
		}
		list_move_tail(&power_module_group_info->list, &power_manager_group_info->power_module_group_deactive_list);
		channel_power_module_group_to_remove_count--;

		if(channel_power_module_group_to_remove_count == 0) {
			break;
		}
	}
}


static void free_power_module_group_for_active_channel_priority(power_manager_group_info_t *power_manager_group_info)
{
	power_manager_channel_info_t *power_manager_channel_info;
	struct list_head *head;

	head = &power_manager_group_info->channel_active_list;

	list_for_each_entry(power_manager_channel_info, head, power_manager_channel_info_t, list) {
		channel_info_deactive_unneeded_power_module_group_priority(power_manager_channel_info);
	}
}

static uint8_t get_more_power_group_count_need(power_manager_group_info_t *power_manager_group_info)
{
	uint8_t count = 0;
	power_manager_channel_info_t *power_manager_channel_info;
	struct list_head *head;

	head = &power_manager_group_info->channel_active_list;

	list_for_each_entry(power_manager_channel_info, head, power_manager_channel_info_t, list) {
		if(list_size(&power_manager_channel_info->power_module_group_list) == 0) {
			count++;
		}
	}
	return count;
}

static int free_priority(void *_power_manager_group_info)
{
	int ret = -1;
	power_manager_group_info_t *power_manager_group_info = (power_manager_group_info_t *)_power_manager_group_info;

	//可用的模块组数
	uint8_t available_power_module_group_count;
	//需要从现在充电的枪中剔除多少个模块组
	uint8_t more_power_module_group_count_need;

	//释放所有即将停止充电的枪关联的模块组
	free_power_module_group_for_stop_channel(power_manager_group_info);

	//释放所有通道多余模块组数
	free_power_module_group_for_active_channel_priority(power_manager_group_info);

	//计算剩余模块组数
	available_power_module_group_count = get_available_power_group_count(power_manager_group_info);
	debug("available_power_module_group_count:%d", available_power_module_group_count);

	//新启动枪至少一个模块组的前提下,至少需要几个模块组
	more_power_module_group_count_need = get_more_power_group_count_need(power_manager_group_info);

	if(more_power_module_group_count_need > available_power_module_group_count) {
		more_power_module_group_count_need -= available_power_module_group_count;
	} else {//不需要再释放模块组，到这里模块组释放完毕
		ret = 0;
		return ret;
	}

	debug("more_power_module_group_count_need:%d", more_power_module_group_count_need);

	//从正在充电的枪中释放模块组
	active_channel_list_remove_power_module_group(power_manager_group_info, more_power_module_group_count_need, 1);

	ret = 0;
	return ret;
}

static power_manager_group_policy_handler_t power_manager_group_policy_handler_priority = {
	.policy = POWER_MANAGER_GROUP_POLICY_PRIORITY,
	.init = init_priority,
	.deinit = deinit_priority,
	.channel_start = channel_start_priority,
	.channel_charging = channel_charging_priority,
	.free = free_priority,
	.assign = assign_average,
	.config = _config,
	.sync = _sync,
};

static power_manager_group_policy_handler_t *power_manager_group_policy_handler_sz[] = {
	&power_manager_group_policy_handler_average,
	&power_manager_group_policy_handler_priority,
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
