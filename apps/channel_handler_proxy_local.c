

/*================================================================
 *
 *
 *   文件名称：channel_handler_proxy_local.c
 *   创 建 者：肖飞
 *   创建日期：2021年09月28日 星期二 15时29分35秒
 *   修改日期：2022年01月09日 星期日 14时24分13秒
 *   描    述：
 *
 *================================================================*/
#include "channel_handler_proxy_local.h"

#include "charger.h"
#include "charger_bms.h"

#include "log.h"

static void handle_channel_faults_stop(channel_info_t *channel_info)
{
	channels_info_t *channels_info = (channels_info_t *)channel_info->channels_info;
	int fault;

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

static void handle_channel_state(channel_info_t *channel_info)
{
	switch(channel_info->state) {
		case CHANNEL_STATE_IDLE: {
		}
		break;

		case CHANNEL_STATE_START: {
		}
		break;

		case CHANNEL_STATE_WAITING: {
		}
		break;

		case CHANNEL_STATE_STARTING: {
		}
		break;

		case CHANNEL_STATE_CHARGING: {
		}
		break;

		case CHANNEL_STATE_STOP: {
		}
		break;

		case CHANNEL_STATE_STOPPING: {
		}
		break;

		case CHANNEL_STATE_END: {
		}
		break;

		case CHANNEL_STATE_CLEANUP: {
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

	//channels faults
	fault = get_first_fault(channels_info->faults);

	if(fault != -1) {
		debug("channels fault:%s", get_channels_fault_des(fault));
		goto exit;
	}

	//channel faults
	fault = get_first_fault(channel_info->faults);

	if(fault != -1) {
		debug("channel %d fault:%s", channel_info->channel_id, get_channel_fault_des(fault));
		goto exit;
	}

exit:

	return;
}

static void handle_channel_periodic(void *_channel_info, void *chain_ctx)
{
	channel_info_t *channel_info = (channel_info_t *)_channel_info;

	//debug("channel_info %d periodic!", channel_info->channel_id);

	handle_channel_stop(channel_info);
	handle_channel_state(channel_info);
	handle_channel_faults(channel_info);

	do_callback_chain(channel_info->channel_periodic_chain, channel_info);
}

static int _handle_channel_event(channel_info_t *channel_info, channel_event_t *channel_event)
{
	int ret = -1;

	debug("channel_id %d process handler event %s!", channel_info->channel_id, get_channel_event_type_des(channel_event->type));

	switch(channel_event->type) {
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

static void channel_record_init(channel_info_t *channel_info)
{
	memset(&channel_info->channel_record_item, 0, sizeof(channel_record_item_t));
}

static void channel_start_callback(void *fn_ctx, void *chain_ctx)
{
	channel_info_t *channel_info = (channel_info_t *)fn_ctx;

	channel_record_init(channel_info);
}

static void channel_end_callback(void *fn_ctx, void *chain_ctx)
{
}

static int init(void *_channel_info)
{
	int ret = 0;
	channel_info_t *channel_info = (channel_info_t *)_channel_info;
	channels_info_t *channels_info = (channels_info_t *)channel_info->channels_info;
	channel_record_task_info_t *channel_record_task_info = get_or_alloc_channel_record_task_info(0);

	OS_ASSERT(channel_record_task_info != NULL);

	channel_info->channel_record_task_info = channel_record_task_info;

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

	return ret;
}

channel_handler_t channel_handler_proxy_local = {
	.channel_type = CHANNEL_TYPE_PROXY_LOCAL,
	.init = init,
	.deinit = deinit,
};
