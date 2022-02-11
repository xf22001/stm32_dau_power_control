

/*================================================================
 *
 *
 *   文件名称：channel_handler_proxy_remote.c
 *   创 建 者：肖飞
 *   创建日期：2021年09月27日 星期一 09时22分16秒
 *   修改日期：2022年02月11日 星期五 21时57分08秒
 *   描    述：
 *
 *================================================================*/
#include "channel_handler_proxy_remote.h"

#include "charger.h"
#include "charger_bms.h"

#include "log.h"

static void handle_channel_faults_stop(channel_info_t *channel_info)
{
	channels_info_t *channels_info = (channels_info_t *)channel_info->channels_info;
	int fault;

	if(channel_info->request_state == CHANNEL_STATE_STOP) {
		return;
	}

	//channels faults
	fault = get_first_fault(channels_info->faults);

	if(fault != -1) {
		debug("channels fault:%s", get_channels_fault_des(fault));
		channel_request_stop(channel_info, channels_fault_to_channel_record_item_stop_reason(fault));
		return;
	}

	//channel faults
	fault = get_first_fault(channel_info->faults);

	if(fault != -1) {
		debug("channel %d fault:%s", channel_info->channel_id, get_channel_fault_des(fault));
		channel_request_stop(channel_info, channel_fault_to_channel_record_item_stop_reason(fault));
		return;
	}
}

static void handle_channel_stop(channel_info_t *channel_info)
{
	switch(channel_info->state) {
		case CHANNEL_STATE_START:
		case CHANNEL_STATE_STARTING:
		case CHANNEL_STATE_CHARGING: {
			handle_channel_faults_stop(channel_info);
		}
		break;

		default: {
		}
		break;
	}
}

static int set_channel_request_state(channel_info_t *channel_info, channel_state_t state)
{
	int ret = -1;

	if(channel_info->request_state == CHANNEL_STATE_NONE) {
		debug("request_state %s in state %s", get_channel_state_des(state), get_channel_state_des(channel_info->state));
	} else {
		debug("request_state %s in state %s busy!!!(%s)", get_channel_state_des(state), get_channel_state_des(channel_info->state), get_channel_state_des(channel_info->request_state));
	}

	channel_info->request_state = state;
	ret = 0;

	return ret;
}

static void channel_start_waiting(channel_info_t *channel_info)
{
}

static void handle_channel_request_state(channel_info_t *channel_info)
{
	if(channel_info->request_state == CHANNEL_STATE_NONE) {
		return;
	}

	if(channel_info->state != channel_info->request_state) {
		do_callback_chain(channel_info->state_changed_chain, channel_info);
	}
}

static void handle_channel_state(channel_info_t *channel_info)
{
	switch(channel_info->state) {
		case CHANNEL_STATE_IDLE: {
			if(channel_info->request_start_state == 1) {
				if(set_channel_request_state(channel_info, CHANNEL_STATE_START) == 0) {
					channel_info->request_start_state = 0;
				}
			}
		}
		break;

		case CHANNEL_STATE_START: {
			if(channel_info->channel_record_item.stop_reason != CHANNEL_RECORD_ITEM_STOP_REASON_NONE) {
				set_channel_request_state(channel_info, CHANNEL_STATE_STOP);
			}
		}
		break;

		case CHANNEL_STATE_WAITING: {
			channel_start_waiting(channel_info);

			if(channel_info->channel_record_item.stop_reason != CHANNEL_RECORD_ITEM_STOP_REASON_NONE) {
				set_channel_request_state(channel_info, CHANNEL_STATE_STOP);
			}
		}
		break;

		case CHANNEL_STATE_STARTING: {
			if(channel_info->channel_record_item.stop_reason != CHANNEL_RECORD_ITEM_STOP_REASON_NONE) {
				set_channel_request_state(channel_info, CHANNEL_STATE_STOP);
			} else {
				//from local bms
				if(channel_info->request_charging_state != 0) {
					if(set_channel_request_state(channel_info, CHANNEL_STATE_CHARGING) == 0) {
						channel_info->request_charging_state = 0;
					}
				}
			}
		}
		break;

		case CHANNEL_STATE_CHARGING: {
			if(channel_info->channel_record_item.stop_reason != CHANNEL_RECORD_ITEM_STOP_REASON_NONE) {
				set_channel_request_state(channel_info, CHANNEL_STATE_STOP);
			}
		}
		break;

		case CHANNEL_STATE_STOP: {
			set_channel_request_state(channel_info, CHANNEL_STATE_STOPPING);
		}
		break;

		case CHANNEL_STATE_STOPPING: {
			//from local bms
			if(channel_info->request_end_state != 0) {
				if(set_channel_request_state(channel_info, CHANNEL_STATE_END) == 0) {
					channel_info->request_end_state = 0;
				}
			}
		}
		break;

		case CHANNEL_STATE_END: {
			set_channel_request_state(channel_info, CHANNEL_STATE_CLEANUP);
		}
		break;

		case CHANNEL_STATE_CLEANUP: {
			//from local bms
			if(channel_info->request_idle_state != 0) {
				if(set_channel_request_state(channel_info, CHANNEL_STATE_IDLE) == 0) {
					channel_info->request_idle_state = 0;

					channel_info->request_start_state = 0;
					channel_info->request_charging_state = 0;
					channel_info->request_end_state = 0;
				}
			}
		}
		break;

		default: {
		}
		break;
	}
}

static void handle_channel_faults(channel_info_t *channel_info)
{
	channels_info_t *channels_info = (channels_info_t *)channel_info->channels_info;
	int fault;
	uint8_t channel_faults = 0;

	//channels faults
	fault = get_first_fault(channels_info->faults);

	if(fault != -1) {
		channel_faults = 1;
		debug("channels fault:%s", get_channels_fault_des(fault));
		goto exit;
	}

	//channel faults
	fault = get_first_fault(channel_info->faults);

	if(fault != -1) {
		channel_faults = 1;
		debug("channel %d fault:%s", channel_info->channel_id, get_channel_fault_des(fault));
		goto exit;
	}

exit:

	if(channels_info->channels_config->fault_port != NULL) {
		GPIO_PinState state = (channel_faults == 1) ? GPIO_PIN_SET : GPIO_PIN_RESET;
		HAL_GPIO_WritePin(channels_info->channels_config->fault_port, channels_info->channels_config->fault_pin, state);
	}
}

static void handle_channel_periodic(void *_channel_info, void *chain_ctx)
{
	channel_info_t *channel_info = (channel_info_t *)_channel_info;

	//debug("channel_info %d periodic!", channel_info->channel_id);

	handle_channel_stop(channel_info);
	handle_channel_state(channel_info);
	handle_channel_request_state(channel_info);
	handle_channel_faults(channel_info);

	do_callback_chain(channel_info->channel_periodic_chain, channel_info);
}

static int _handle_channel_event(channel_info_t *channel_info, channel_event_t *channel_event)
{
	int ret = -1;

	debug("channel_id %d process handler event %s!", channel_info->channel_id, get_channel_event_type_des(channel_event->type));

	switch(channel_event->type) {
		case CHANNEL_EVENT_TYPE_START_CHANNEL: {
			switch(channel_info->state) {
				case CHANNEL_STATE_IDLE: {
					if(channel_event->ctx != NULL) {
						channel_info->channel_event_start = (channel_event_start_t *)channel_event->ctx;

						channel_request_start(channel_info);
						ret = 0;
					} else {
						debug("channel %d channel event %p", channel_info->channel_id, channel_event->ctx);
					}
				}
				break;

				default: {
					debug("channel %d state %s", channel_info->channel_id, get_channel_state_des(channel_info->state));
				}
				break;
			}
		}
		break;

		case CHANNEL_EVENT_TYPE_STOP_CHANNEL: {
			switch(channel_info->state) {
				case CHANNEL_STATE_START:
				case CHANNEL_STATE_STARTING:
				case CHANNEL_STATE_CHARGING: {
					channel_request_stop(channel_info, channel_info->channel_event_stop.stop_reason);
					channel_info->channel_event_stop.stop_reason = CHANNEL_RECORD_ITEM_STOP_REASON_NONE;
					ret = 0;
				}
				break;

				default: {
					debug("channel %d state %s", channel_info->channel_id, get_channel_state_des(channel_info->state));
				}
				break;
			}
		}
		break;

		default: {
		}
		break;
	}

	return ret;
}

static void handle_channel_event(void *_channel_info, void *_channels_event)
{
	channel_info_t *channel_info = (channel_info_t *)_channel_info;
	channels_event_t *channels_event = (channels_event_t *)_channels_event;
	channel_event_t *channel_event = (channel_event_t *)channels_event->event;
	uint8_t match = 0;

	if(channels_event->type != CHANNELS_EVENT_CHANNEL) {
		return;
	}

	if(channel_event == NULL) {
		debug("");
		return;
	}

	if(channel_event->channel_id == 0xff) { //broadcast
		match = 1;
	}

	if(channel_event->channel_id == channel_info->channel_id) {
		match = 1;
	}

	if(match == 0) {
		return;
	}

	_handle_channel_event(channel_info, channel_event);
}

static void state_changed_notify(channel_info_t *channel_info)
{
	channels_info_t *channels_info = (channels_info_t *)channel_info->channels_info;
	channels_notify_ctx_t channels_notify_ctx;

	channels_notify_ctx.notify = CHANNELS_NOTIFY_CHANNEL_STATE_CHANGE;
	channels_notify_ctx.ctx = channel_info;

	do_callback_chain(channels_info->channels_notify_chain, &channels_notify_ctx);
}

static void prepare_channel_state_changed(channel_info_t *channel_info)
{
	switch(channel_info->request_state) {
		case CHANNEL_STATE_START: {
			do_callback_chain(channel_info->start_chain, channel_info);
		}
		break;

		case CHANNEL_STATE_STARTING: {
			//charger_info_t *charger_info = (charger_info_t *)channel_info->charger_info;
			//set_charger_bms_request_action(charger_info, CHARGER_BMS_REQUEST_ACTION_START);
			//to local bms
		}
		break;

		case CHANNEL_STATE_STOP: {
			//charger_info_t *charger_info = (charger_info_t *)channel_info->charger_info;
			//set_charger_bms_request_action(charger_info, CHARGER_BMS_REQUEST_ACTION_STOP);
			//to local bms
		}
		break;

		case CHANNEL_STATE_END: {
			//charger_info_t *charger_info = (charger_info_t *)channel_info->charger_info;
			//set_charger_bms_request_action(charger_info, CHARGER_BMS_REQUEST_ACTION_NONE);
			//to local bms
			do_callback_chain(channel_info->end_chain, channel_info);
		}
		break;

		default: {
		}
		break;
	}
}

static void state_changed(void *fn_ctx, void *chain_ctx)
{
	channel_info_t *channel_info = (channel_info_t *)fn_ctx;

	debug("channel state:%s -> %s!", get_channel_state_des(channel_info->state), get_channel_state_des(channel_info->request_state));
	state_changed_notify(channel_info);

	prepare_channel_state_changed(channel_info);

	channel_info->state = channel_info->request_state;
	channel_info->request_state = CHANNEL_STATE_NONE;
}

static void channel_record_init(channel_info_t *channel_info)
{
	memset(&channel_info->channel_record_item, 0, sizeof(channel_record_item_t));
}

static void channel_record_start(channel_info_t *channel_info)
{
	channel_info->channel_record_item.stop_reason = CHANNEL_RECORD_ITEM_STOP_REASON_NONE;
}

static void channel_start_callback(void *fn_ctx, void *chain_ctx)
{
	channel_info_t *channel_info = (channel_info_t *)fn_ctx;

	channel_record_init(channel_info);
	channel_record_start(channel_info);
}

static void channel_end_callback(void *fn_ctx, void *chain_ctx)
{
	//channel_info_t *channel_info = (channel_info_t *)fn_ctx;
}

static int init(void *_channel_info)
{
	int ret = 0;
	channel_info_t *channel_info = (channel_info_t *)_channel_info;
	channels_info_t *channels_info = (channels_info_t *)channel_info->channels_info;

	channel_info->periodic_callback_item.fn = handle_channel_periodic;
	channel_info->periodic_callback_item.fn_ctx = channel_info;
	OS_ASSERT(register_callback(channels_info->common_periodic_chain, &channel_info->periodic_callback_item) == 0);

	channel_info->event_callback_item.fn = handle_channel_event;
	channel_info->event_callback_item.fn_ctx = channel_info;
	OS_ASSERT(register_callback(channels_info->common_event_chain, &channel_info->event_callback_item) == 0);

	channel_info->channel_start_callback_item.fn = channel_start_callback;
	channel_info->channel_start_callback_item.fn_ctx = channel_info;
	OS_ASSERT(register_callback(channel_info->start_chain, &channel_info->channel_start_callback_item) == 0);

	channel_info->channel_end_callback_item.fn = channel_end_callback;
	channel_info->channel_end_callback_item.fn_ctx = channel_info;
	OS_ASSERT(register_callback(channel_info->end_chain, &channel_info->channel_end_callback_item) == 0);

	channel_info->channel_state_changed_callback_item.fn = state_changed;
	channel_info->channel_state_changed_callback_item.fn_ctx = channel_info;
	OS_ASSERT(register_callback(channel_info->state_changed_chain, &channel_info->channel_state_changed_callback_item) == 0);

	return ret;
}

static int deinit(void *_channel_info)
{
	int ret = 0;
	channel_info_t *channel_info = (channel_info_t *)_channel_info;
	channels_info_t *channels_info = (channels_info_t *)channel_info->channels_info;

	remove_callback(channels_info->common_periodic_chain, &channel_info->periodic_callback_item);
	remove_callback(channels_info->common_event_chain, &channel_info->event_callback_item);
	remove_callback(channel_info->start_chain, &channel_info->channel_start_callback_item);
	remove_callback(channel_info->end_chain, &channel_info->channel_end_callback_item);
	remove_callback(channel_info->state_changed_chain, &channel_info->channel_state_changed_callback_item);

	return ret;
}

channel_handler_t channel_handler_proxy_remote = {
	.channel_type = CHANNEL_TYPE_PROXY_REMOTE,
	.init = init,
	.deinit = deinit,
};
