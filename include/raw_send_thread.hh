
#pragma once

#include <memory>

#include <shared_queue.hh>
#include <wfb_bridge.hh>
#include <log_thread.hh>

void raw_send_thread(SharedQueue<std::shared_ptr<Message> > &outqueue,
		     RawSendSocket &raw_send_sock, uint16_t max_queue_size,
		     TransferStats &trans_stats, bool &terminate);
