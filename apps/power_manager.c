

/*================================================================
 *
 *
 *   文件名称：power_manager.c
 *   创 建 者：肖飞
 *   创建日期：2021年11月23日 星期二 14时08分52秒
 *   修改日期：2022年02月16日 星期三 14时35分37秒
 *   描    述：
 *
 *================================================================*/
#include "power_manager.h"
#include "power_manager_handler_native.h"

#include "log.h"

static power_manager_handler_t *power_manager_handler_sz[] = {
	&power_manager_handler_native,
};

static power_manager_handler_t *get_power_manager_handler(power_manager_type_t type)
{
	int i;
	power_manager_handler_t *power_manager_handler = NULL;

	for(i = 0; i < ARRAY_SIZE(power_manager_handler_sz); i++) {
		power_manager_handler_t *power_manager_handler_item = power_manager_handler_sz[i];

		if(power_manager_handler_item->type == type) {
			power_manager_handler = power_manager_handler_item;
		}
	}

	return power_manager_handler;
}

char *get_power_manager_channel_state_des(power_manager_channel_state_t state)
{
	char *des = "unknow";

	switch(state) {
			add_des_case(POWER_MANAGER_CHANNEL_STATE_IDLE);
			add_des_case(POWER_MANAGER_CHANNEL_STATE_PREPARE_START);
			add_des_case(POWER_MANAGER_CHANNEL_STATE_START);
			add_des_case(POWER_MANAGER_CHANNEL_STATE_RUNNING);
			add_des_case(POWER_MANAGER_CHANNEL_STATE_PREPARE_STOP);
			add_des_case(POWER_MANAGER_CHANNEL_STATE_STOP);

		default: {
		}
		break;
	}

	return des;
}

char *get_power_manager_group_change_state_des(power_manager_change_state_t state)
{
	char *des = "unknow";

	switch(state) {
			add_des_case(POWER_MANAGER_CHANGE_STATE_IDLE);
			add_des_case(POWER_MANAGER_CHANGE_STATE_MODULE_PREPARE_FREE);
			add_des_case(POWER_MANAGER_CHANGE_STATE_MODULE_FREE);
			add_des_case(POWER_MANAGER_CHANGE_STATE_MODULE_FREE_CONFIG);
			add_des_case(POWER_MANAGER_CHANGE_STATE_MODULE_FREE_CONFIG_SYNC);
			add_des_case(POWER_MANAGER_CHANGE_STATE_MODULE_ASSIGN);
			add_des_case(POWER_MANAGER_CHANGE_STATE_MODULE_READY_SYNC);
			add_des_case(POWER_MANAGER_CHANGE_STATE_MODULE_ASSIGN_CONFIG);
			add_des_case(POWER_MANAGER_CHANGE_STATE_MODULE_ASSIGN_CONFIG_SYNC);

		default: {
		}
		break;
	}

	return des;
}

char *get_power_module_item_state_des(power_module_item_state_t state)
{
	char *des = "unknow";

	switch(state) {
			add_des_case(POWER_MODULE_ITEM_STATE_IDLE);
			add_des_case(POWER_MODULE_ITEM_STATE_PREPARE_ACTIVE);
			add_des_case(POWER_MODULE_ITEM_STATE_READY);
			add_des_case(POWER_MODULE_ITEM_STATE_ACTIVE);
			add_des_case(POWER_MODULE_ITEM_STATE_PREPARE_DEACTIVE);
			add_des_case(POWER_MODULE_ITEM_STATE_DEACTIVE);
			add_des_case(POWER_MODULE_ITEM_STATE_DISABLE);

		default: {
		}
		break;
	}

	return des;
}

static uint8_t dump_channels_stats = 0;

void start_dump_channels_stats(void)
{
	dump_channels_stats = 1;
}

static void print_power_module_item_info(const uint8_t pad, power_module_item_info_t *power_module_item_info)
{
	//u_power_module_status_t u_power_module_status;
	char *_pad = (char *)os_calloc(1, pad + 1);

	OS_ASSERT(_pad != NULL);
	memset(_pad, '\t', pad);

	_printf("%spower module:%d\n", _pad, power_module_item_info->id);
	_printf("%s\tstate:%s\n", _pad, get_power_module_item_state_des(power_module_item_info->status.state));
	_printf("%s\trequire_output_voltage:%d\n", _pad, power_module_item_info->status.require_output_voltage);
	_printf("%s\trequire_output_current:%d\n", _pad, power_module_item_info->status.require_output_current);
	_printf("%s\tsetting_output_voltage:%d\n", _pad, power_module_item_info->status.setting_output_voltage);
	_printf("%s\tsetting_output_current:%d\n", _pad, power_module_item_info->status.setting_output_current);
	_printf("%s\tsmooth_setting_output_current:%d\n", _pad, power_module_item_info->status.smooth_setting_output_current);
	_printf("%s\tmodule_output_voltage:%d\n", _pad, power_module_item_info->status.module_output_voltage);
	_printf("%s\tmodule_output_current:%d\n", _pad, power_module_item_info->status.module_output_current);
	//_printf("%s\tconnect_state:%d\n", _pad, power_module_item_info->status.connect_state);
	//u_power_module_status.v = power_module_item_info->status.module_status;
	//_printf("%s\tpoweroff:%d\n", _pad, u_power_module_status.s.poweroff);
	//_printf("%s\tfault:%d\n", _pad, u_power_module_status.s.fault);
	//_printf("%s\toutput_state:%d\n", _pad, u_power_module_status.s.output_state);
	//_printf("%s\tfan_state:%d\n", _pad, u_power_module_status.s.fan_state);
	//_printf("%s\tinput_overvoltage:%d\n", _pad, u_power_module_status.s.input_overvoltage);
	//_printf("%s\tinput_lowvoltage:%d\n", _pad, u_power_module_status.s.input_lowvoltage);
	//_printf("%s\toutput_overvoltage:%d\n", _pad, u_power_module_status.s.output_overvoltage);
	//_printf("%s\toutput_lowvoltage:%d\n", _pad, u_power_module_status.s.output_lowvoltage);
	//_printf("%s\tprotect_overcurrent:%d\n", _pad, u_power_module_status.s.protect_overcurrent);
	//_printf("%s\tprotect_overtemperature:%d\n", _pad, u_power_module_status.s.protect_overtemperature);
	//_printf("%s\tsetting_poweroff:%d\n", _pad, u_power_module_status.s.setting_poweroff);

	os_free(_pad);
}

static void print_power_module_group_info(const uint8_t pad, power_module_group_info_t *power_module_group_info)
{
	char *_pad = (char *)os_calloc(1, pad + 1);
	struct list_head *head = &power_module_group_info->power_module_item_list;
	power_module_item_info_t *power_module_item_info;

	OS_ASSERT(_pad != NULL);
	memset(_pad, '\t', pad);

	_printf("%spower mdoule group:%d\n", _pad, power_module_group_info->id);
	list_for_each_entry(power_module_item_info, head, power_module_item_info_t, list) {
		print_power_module_item_info(pad + 1, power_module_item_info);
	}

	os_free(_pad);
}

static void print_power_manager_channel_info(const uint8_t pad, power_manager_channel_info_t *power_manager_channel_info)
{
	char *_pad = (char *)os_calloc(1, pad + 1);

	OS_ASSERT(_pad != NULL);
	memset(_pad, '\t', pad);

	_printf("%spower manager channel:%d\n", _pad, power_manager_channel_info->id);
	_printf("%s\tstate:%s\n", _pad, get_power_manager_channel_state_des(power_manager_channel_info->status.state));
	_printf("%s\trequire_output_voltage:%d\n", _pad, power_manager_channel_info->status.require_output_voltage);
	_printf("%s\trequire_output_current:%d\n", _pad, power_manager_channel_info->status.require_output_current);
	_printf("%s\treassign:%d\n", _pad, power_manager_channel_info->status.reassign);
	_printf("%s\treassign_module_number:%d\n", _pad, power_manager_channel_info->status.reassign_module_group_number);
	_printf("%s\tcharge_output_voltage:%d\n", _pad, power_manager_channel_info->status.charge_output_voltage);
	_printf("%s\tcharge_output_current:%d\n", _pad, power_manager_channel_info->status.charge_output_current);

	os_free(_pad);
}

static void power_manager_debug(power_manager_info_t *power_manager_info)
{
	int i;
	int j;
	channels_info_t *channels_info = power_manager_info->channels_info;
	struct list_head *head;
	struct list_head *head1;

	_printf("all module items:\n");

	for(i = 0; i < power_manager_info->power_modules_info->power_module_number; i++) {
		power_module_item_info_t *power_module_item_info = power_manager_info->power_module_item_info + i;

		print_power_module_item_info(1, power_module_item_info);
	}

	_printf("\n");

	_printf("all channels:\n");

	for(i = 0; i < channels_info->channel_number; i++) {
		power_manager_channel_info_t *power_manager_channel_info = power_manager_info->power_manager_channel_info + i;
		power_module_group_info_t *power_module_group_info;

		print_power_manager_channel_info(1, power_manager_channel_info);

		head = &power_manager_channel_info->power_module_group_list;
		list_for_each_entry(power_module_group_info, head, power_module_group_info_t, list) {
			print_power_module_group_info(2, power_module_group_info);
		}
	}

	_printf("\n");

	for(i = 0; i < power_manager_info->power_manager_group_number; i++) {
		power_manager_group_info_t *power_manager_group_info = power_manager_info->power_manager_group_info + i;
		power_module_group_info_t *power_module_group_info;
		power_manager_channel_info_t *power_manager_channel_info;

		_printf("power manager group:%d\n", power_manager_group_info->id);

		_printf("\tpower manager group %d power module groups:\n", power_manager_group_info->id);

		for(j = 0; j < power_manager_group_info->power_module_group_number; j++) {
			power_module_group_info_t *power_module_group_info = power_manager_group_info->power_module_group_info + j;
			print_power_module_group_info(2, power_module_group_info);
		}

		_printf("\n");

		_printf("\tall idle module groups:\n");
		head = &power_manager_group_info->power_module_group_idle_list;
		list_for_each_entry(power_module_group_info, head, power_module_group_info_t, list) {
			print_power_module_group_info(2, power_module_group_info);
		}
		_printf("\n");

		_printf("\tall deactive module groups:\n");
		head = &power_manager_group_info->power_module_group_deactive_list;
		list_for_each_entry(power_module_group_info, head, power_module_group_info_t, list) {
			print_power_module_group_info(2, power_module_group_info);
		}
		_printf("\n");

		_printf("\tall disable module groups:\n");
		head = &power_manager_group_info->power_module_group_disable_list;
		list_for_each_entry(power_module_group_info, head, power_module_group_info_t, list) {
			print_power_module_group_info(2, power_module_group_info);
		}
		_printf("\n");

		_printf("\tpower manager group %d power manager channels:\n", power_manager_group_info->id);

		_printf("\tall idle channels:\n");
		head = &power_manager_group_info->channel_idle_list;
		list_for_each_entry(power_manager_channel_info, head, power_manager_channel_info_t, list) {
			print_power_manager_channel_info(2, power_manager_channel_info);

			head1 = &power_manager_channel_info->power_module_group_list;
			list_for_each_entry(power_module_group_info, head1, power_module_group_info_t, list) {
				print_power_module_group_info(2, power_module_group_info);
			}
		}
		_printf("\n");

		_printf("\tall active channels:\n");
		head = &power_manager_group_info->channel_active_list;
		list_for_each_entry(power_manager_channel_info, head, power_manager_channel_info_t, list) {
			print_power_manager_channel_info(2, power_manager_channel_info);

			head1 = &power_manager_channel_info->power_module_group_list;
			list_for_each_entry(power_module_group_info, head1, power_module_group_info_t, list) {
				print_power_module_group_info(3, power_module_group_info);
			}
		}
		_printf("\n");

		_printf("\tall deactive channels:\n");
		head = &power_manager_group_info->channel_deactive_list;
		list_for_each_entry(power_manager_channel_info, head, power_manager_channel_info_t, list) {
			print_power_manager_channel_info(2, power_manager_channel_info);

			head1 = &power_manager_channel_info->power_module_group_list;
			list_for_each_entry(power_module_group_info, head1, power_module_group_info_t, list) {
				print_power_module_group_info(3, power_module_group_info);
			}
		}
		_printf("\n");

		_printf("\tall disable channels:\n");
		head = &power_manager_group_info->channel_disable_list;
		list_for_each_entry(power_manager_channel_info, head, power_manager_channel_info_t, list) {
			print_power_manager_channel_info(2, power_manager_channel_info);

			head1 = &power_manager_channel_info->power_module_group_list;
			list_for_each_entry(power_module_group_info, head1, power_module_group_info_t, list) {
				print_power_module_group_info(3, power_module_group_info);
			}
		}
		_printf("\n");
	}
}

static void handle_power_manager_group_faults(power_manager_info_t *power_manager_info)
{
	int i;
	int j;
	channels_info_t *channels_info = power_manager_info->channels_info;

	for(i = 0; i < channels_info->channel_number; i++) {
		uint8_t relay_board_over_temperature = 0;

		power_manager_channel_info_t *power_manager_channel_info = power_manager_info->power_manager_channel_info + i;
		channel_info_t *channel_info = channels_info->channel_info + i;

		for(j = 0; j < channels_info->relay_board_number; j++) {
			power_manager_relay_board_info_t *power_manager_relay_board_info = power_manager_info->power_manager_relay_board_info + j;

			if(power_manager_relay_board_info->power_manager_channel_info != power_manager_channel_info) {
				continue;
			}

			if(get_fault(power_manager_relay_board_info->faults, POWER_MANAGER_RELAY_BOARD_FAULT_OVER_TEMPERATURE) == 1) {
				relay_board_over_temperature = 1;
				break;
			}
		}

		set_fault(channel_info->faults, CHANNEL_FAULT_POWER_MANAGER_RELAY_BOARD_OVER_TEMPERATURE, relay_board_over_temperature);
	}

	for(i = 0; i < power_manager_info->power_manager_group_number; i++) {
		uint8_t relay_board_timeout = 0;
		uint8_t relay_board_fault = 0;

		power_manager_group_info_t *power_manager_group_info = power_manager_info->power_manager_group_info + i;

		//relay board
		for(j = 0; j < channels_info->relay_board_number; j++) {
			power_manager_relay_board_info_t *power_manager_relay_board_info = power_manager_info->power_manager_relay_board_info + j;
			power_manager_channel_info_t *power_manager_channel_info = (power_manager_channel_info_t *)power_manager_relay_board_info->power_manager_channel_info;

			if(power_manager_channel_info->power_manager_group_info != power_manager_group_info) {
				continue;
			}

			if(get_fault(power_manager_relay_board_info->faults, POWER_MANAGER_RELAY_BOARD_FAULT_FAULT) == 1) {
				relay_board_fault = 1;
				break;
			}

			if(get_fault(power_manager_relay_board_info->faults, POWER_MANAGER_RELAY_BOARD_FAULT_CONNECT) == 1) {
				relay_board_timeout = 1;
				break;
			}
		}

		if(get_fault(power_manager_group_info->faults, POWER_MANAGER_GROUP_FAULT_RELAY_BOARD_FAULT) != relay_board_fault) {
			set_fault(power_manager_group_info->faults, POWER_MANAGER_GROUP_FAULT_RELAY_BOARD_FAULT, relay_board_fault);

			for(j = 0; j < channels_info->channel_number; j++) {
				power_manager_channel_info_t *power_manager_channel_info = power_manager_info->power_manager_channel_info + j;
				channel_info_t *channel_info = channels_info->channel_info + j;

				if(power_manager_channel_info->power_manager_group_info != power_manager_group_info) {
					continue;
				}

				set_fault(channel_info->faults, CHANNEL_FAULT_POWER_MANAGER_RELAY_BOARD_FAULT, relay_board_fault);
			}
		}

		if(get_fault(power_manager_group_info->faults, POWER_MANAGER_GROUP_FAULT_RELAY_BOARD_CONNECT_TIMEOUT) != relay_board_timeout) {
			set_fault(power_manager_group_info->faults, POWER_MANAGER_GROUP_FAULT_RELAY_BOARD_CONNECT_TIMEOUT, relay_board_timeout);

			for(j = 0; j < channels_info->channel_number; j++) {
				power_manager_channel_info_t *power_manager_channel_info = power_manager_info->power_manager_channel_info + j;
				channel_info_t *channel_info = channels_info->channel_info + j;

				if(power_manager_channel_info->power_manager_group_info != power_manager_group_info) {
					continue;
				}

				set_fault(channel_info->faults, CHANNEL_FAULT_POWER_MANAGER_RELAY_BOARD_CONNECT_TIMEOUT, relay_board_timeout);
			}
		}
	}
}

static void do_dump_power_manager_stats(power_manager_info_t *power_manager_info)
{
	if(dump_channels_stats == 0) {
		return;
	}

	dump_channels_stats = 0;

	power_manager_debug(power_manager_info);
}

static void power_manager_periodic(void *fn_ctx, void *chain_ctx)
{
	power_manager_info_t *power_manager_info = (power_manager_info_t *)fn_ctx;

	handle_power_manager_group_faults(power_manager_info);
	do_dump_power_manager_stats(power_manager_info);
}

static void init_power_manager_group_info(power_manager_info_t *power_manager_info)
{
	channels_info_t *channels_info = power_manager_info->channels_info;
	power_manager_settings_t *power_manager_settings = &channels_info->channels_settings.power_manager_settings;
	int i;
	int j;
	int k;
	uint8_t power_module_item_offset = 0;
	uint8_t power_manager_channel_offset = 0;
	uint8_t power_manager_relay_board_offset = 0;
	uint8_t power_manager_relay_board_slot_offset_base = 0;

	power_manager_info->power_manager_channel_module_assign_ready_chain = alloc_callback_chain();
	OS_ASSERT(power_manager_info->power_manager_channel_module_assign_ready_chain != NULL);

	debug("power manager group number:%d", power_manager_settings->power_manager_group_number);
	power_manager_info->power_manager_group_number = power_manager_settings->power_manager_group_number;

	if(power_manager_info->power_manager_group_number == 0) {
		return;
	}

	debug("power module number:%d", channels_info->power_module_number);

	power_manager_info->power_module_item_info = (power_module_item_info_t *)os_calloc(
	            power_manager_info->power_modules_info->power_module_number,
	            sizeof(power_module_item_info_t));
	OS_ASSERT(power_manager_info->power_module_item_info != NULL);

	for(i = 0; i < channels_info->power_module_number; i++) {
		power_module_item_info_t *power_module_item_info = power_manager_info->power_module_item_info + i;

		INIT_LIST_HEAD(&power_module_item_info->list);
		power_module_item_info->id = i;

		power_module_item_info->power_modules_info = power_manager_info->power_modules_info;

		power_module_item_info->faults = alloc_bitmap(POWER_MODULE_ITEM_FAULT_SIZE);
		OS_ASSERT(power_module_item_info->faults != NULL);
	}

	debug("channel number:%d", channels_info->channel_number);

	power_manager_info->power_manager_channel_info = (power_manager_channel_info_t *)os_calloc(
	            channels_info->channel_number,
	            sizeof(power_manager_channel_info_t));
	OS_ASSERT(power_manager_info->power_manager_channel_info != NULL);

	for(i = 0; i < channels_info->channel_number; i++) {
		power_manager_channel_info_t *power_manager_channel_info = power_manager_info->power_manager_channel_info + i;

		INIT_LIST_HEAD(&power_manager_channel_info->power_module_group_list);
		INIT_LIST_HEAD(&power_manager_channel_info->relay_board_list);
		power_manager_channel_info->id = i;
	}

	power_manager_info->power_manager_group_info = (power_manager_group_info_t *)os_calloc(
	            power_manager_info->power_manager_group_number,
	            sizeof(power_manager_group_info_t));
	OS_ASSERT(power_manager_info->power_manager_group_info != NULL);

	debug("relay board number:%d", channels_info->relay_board_number);

	if(channels_info->relay_board_number != 0) {
		power_manager_info->power_manager_relay_board_info = (power_manager_relay_board_info_t *)os_calloc(
		            channels_info->relay_board_number,
		            sizeof(power_manager_relay_board_info_t));
		OS_ASSERT(power_manager_info->power_manager_relay_board_info != NULL);
	}

	for(i = 0; i < power_manager_info->power_manager_group_number; i++) {
		power_manager_group_settings_t *power_manager_group_settings = &power_manager_settings->power_manager_group_settings[i];
		power_manager_group_info_t *power_manager_group_info = power_manager_info->power_manager_group_info + i;
		power_manager_group_info->faults = alloc_bitmap(POWER_MANAGER_GROUP_FAULT_RELAY_SIZE);
		OS_ASSERT(power_manager_group_info->faults != NULL);

		power_manager_group_info->power_manager_info = power_manager_info;
		power_manager_group_info->id = i;
		power_manager_group_info->change_state = POWER_MANAGER_CHANGE_STATE_IDLE;

		INIT_LIST_HEAD(&power_manager_group_info->channel_idle_list);
		INIT_LIST_HEAD(&power_manager_group_info->channel_active_list);
		INIT_LIST_HEAD(&power_manager_group_info->channel_deactive_list);
		INIT_LIST_HEAD(&power_manager_group_info->channel_disable_list);
		INIT_LIST_HEAD(&power_manager_group_info->power_module_group_idle_list);
		INIT_LIST_HEAD(&power_manager_group_info->power_module_group_deactive_list);
		INIT_LIST_HEAD(&power_manager_group_info->power_module_group_disable_list);

		debug("power manager group %d power module group number:%d", i, power_manager_group_settings->power_module_group_number);
		debug("power manager group %d channel number:%d", i, power_manager_group_settings->channel_number);

		power_manager_group_info->power_module_group_number = power_manager_group_settings->power_module_group_number;
		power_manager_group_info->channel_number = power_manager_group_settings->channel_number;

		power_manager_group_info->power_module_group_info = (power_module_group_info_t *)os_calloc(
		            power_manager_group_settings->power_module_group_number,
		            sizeof(power_module_group_info_t));
		OS_ASSERT(power_manager_group_info->power_module_group_info != NULL);

		for(j = 0; j < power_manager_group_settings->power_module_group_number; j++) {
			power_module_group_info_t *power_module_group_info = power_manager_group_info->power_module_group_info + j;
			power_module_group_settings_t *power_module_group_settings = &power_manager_group_settings->power_module_group_settings[j];

			INIT_LIST_HEAD(&power_module_group_info->list);
			INIT_LIST_HEAD(&power_module_group_info->power_module_item_list);
			power_module_group_info->id = j;
			power_module_group_info->power_manager_group_info = power_manager_group_info;
			power_module_group_info->power_manager_channel_info = NULL;

			for(k = 0; k < power_module_group_settings->power_module_number; k++) {
				power_module_item_info_t *power_module_item_info = power_manager_info->power_module_item_info + power_module_item_offset;
				debug("add power module %d to power module group %d in power manager group %d",
				      power_module_item_info->id,
				      power_module_group_info->id,
				      power_manager_group_info->id);
				power_module_item_info->power_module_group_info = power_module_group_info;
				list_add_tail(&power_module_item_info->list, &power_module_group_info->power_module_item_list);
				power_module_item_offset++;
				OS_ASSERT(power_module_item_offset <= power_manager_info->power_modules_info->power_module_number);
			}

			debug("add power module group %d to power manager group %d",
			      power_module_group_info->id,
			      power_manager_group_info->id);
			list_add_tail(&power_module_group_info->list, &power_manager_group_info->power_module_group_idle_list);
		}

		for(j = 0; j < power_manager_group_settings->channel_number; j++) {
			power_manager_channel_info_t *power_manager_channel_info = power_manager_info->power_manager_channel_info + power_manager_channel_offset;
			uint8_t power_manager_relay_board_slot_offset = power_manager_relay_board_slot_offset_base;

			if(power_manager_group_settings->relay_board_number_per_channel != 0) {
				for(k = 0; k < power_manager_group_settings->relay_board_number_per_channel; k++) {
					uint8_t slot_per_relay_board = power_manager_group_settings->slot_per_relay_board[k];
					power_manager_relay_board_info_t *power_manager_relay_board_info = power_manager_info->power_manager_relay_board_info + power_manager_relay_board_offset;

					power_manager_relay_board_info->faults = alloc_bitmap(POWER_MANAGER_RELAY_BOARD_FAULT_SIZE);
					OS_ASSERT(power_manager_relay_board_info->faults != NULL);

					power_manager_relay_board_info->id = power_manager_relay_board_offset;
					power_manager_relay_board_info->power_manager_channel_info = power_manager_channel_info;
					power_manager_relay_board_info->offset = power_manager_relay_board_slot_offset;
					power_manager_relay_board_info->number = slot_per_relay_board;

					power_manager_relay_board_offset++;
					power_manager_relay_board_slot_offset += slot_per_relay_board;

					debug("add relay board %d offset %d, number:%d, to channel %d",
					      power_manager_relay_board_info->id,
					      power_manager_relay_board_info->offset,
					      power_manager_relay_board_info->number,
					      power_manager_channel_info->id);
					list_add_tail(&power_manager_relay_board_info->list, &power_manager_channel_info->relay_board_list);
				}
			}

			power_manager_channel_info->power_manager_group_info = power_manager_group_info;
			debug("add channel %d to power manager group %d",
			      power_manager_channel_info->id,
			      power_manager_group_info->id);
			list_add_tail(&power_manager_channel_info->list, &power_manager_group_info->channel_idle_list);
			power_manager_channel_offset++;
			OS_ASSERT(power_manager_channel_offset <= channels_info->channel_number);
		}

		if(power_manager_group_settings->relay_board_number_per_channel != 0) {
			for(k = 0; k < power_manager_group_settings->relay_board_number_per_channel; k++) {
				uint8_t slot_per_relay_board = power_manager_group_settings->slot_per_relay_board[k];

				power_manager_relay_board_slot_offset_base += slot_per_relay_board;
			}
		}
	}

	power_manager_info->power_manager_periodic_callback_item.fn = power_manager_periodic;
	power_manager_info->power_manager_periodic_callback_item.fn_ctx = power_manager_info;
	OS_ASSERT(register_callback(channels_info->common_periodic_chain, &power_manager_info->power_manager_periodic_callback_item) == 0);
}

void alloc_power_manager(channels_info_t *channels_info)
{
	power_manager_settings_t *power_manager_settings = &channels_info->channels_settings.power_manager_settings;
	power_manager_info_t *power_manager_info = (power_manager_info_t *)os_calloc(1, sizeof(power_manager_info_t));

	OS_ASSERT(power_manager_info != NULL);
	power_manager_info->channels_info = channels_info;
	channels_info->power_manager_info = power_manager_info;

	power_manager_info->power_modules_info = alloc_power_modules_info(channels_info);
	OS_ASSERT(power_manager_info->power_modules_info != NULL);

	init_power_manager_group_info(power_manager_info);

	debug("set power_manager %s", get_power_manager_type_des(power_manager_settings->type));
	power_manager_info->power_manager_handler = get_power_manager_handler(power_manager_settings->type);

	if((power_manager_info->power_manager_handler != NULL) && (power_manager_info->power_manager_handler->init != NULL)) {
		power_manager_info->power_manager_handler->init(power_manager_info);
	} else {
		debug("skip power_manager %s", get_power_manager_type_des(power_manager_settings->type));
	}

	start_relay_boards_comm_proxy_remote(channels_info);
}

__weak void power_manager_restore_config(channels_info_t *channels_info)
{
	int i;
	int j;

	channels_settings_t *channels_settings = &channels_info->channels_settings;
	power_manager_settings_t *power_manager_settings = &channels_settings->power_manager_settings;

	power_manager_settings->power_manager_group_number = POWER_MANAGER_GROUP_MAX_SIZE;

	debug("power_manager_group_number:%d", power_manager_settings->power_manager_group_number);

	for(i = 0; i < power_manager_settings->power_manager_group_number; i++) {
		power_manager_group_settings_t *power_manager_group_settings = &power_manager_settings->power_manager_group_settings[i];

		power_manager_group_settings->channel_number = DEFAULT_POWER_MANAGER_GROUP_CHANNEL_NUMBER;
		power_manager_group_settings->relay_board_number_per_channel = RELAY_BOARD_NUMBER_PER_CHANNEL_MAX;

		for(j = 0; j < power_manager_group_settings->relay_board_number_per_channel; j++) {
			power_manager_group_settings->slot_per_relay_board[j] = 6;
		}

		power_manager_group_settings->power_module_group_number = POWER_MODULE_GROUP_MAX_SIZE;

		channels_info->channel_number += power_manager_group_settings->channel_number;

		for(j = 0; j < power_manager_group_settings->power_module_group_number; j++) {
			power_module_group_settings_t *power_module_group_settings = &power_manager_group_settings->power_module_group_settings[j];
			power_module_group_settings->power_module_number = 1;
		}
	}
}

__weak power_manager_group_policy_handler_t *get_power_manager_group_policy_handler(uint8_t policy)
{
	return NULL;
}

int set_power_manager_channel_request_state(power_manager_info_t *power_manager_info, uint8_t channel_id, power_manager_channel_request_state_t state)
{
	int ret = 0;
	channels_info_t *channels_info = power_manager_info->channels_info;
	power_manager_channel_info_t *power_manager_channel_info = power_manager_info->power_manager_channel_info + channel_id;

	OS_ASSERT(channel_id < channels_info->channel_number);

	power_manager_channel_info->request_state = state;

	return ret;
}
