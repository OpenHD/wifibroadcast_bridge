
#include <iostream>
#include <iomanip>

#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>

#include <transfer_stats.hh>

const float prev_weight = 0.02;

transfer_stats_t::transfer_stats_t(uint32_t _sequences, uint32_t _blocks_in, uint32_t _blocks_out,
				   uint32_t _bytes_in, uint32_t _bytes_out, uint32_t _block_errors,
				   uint32_t _sequence_errors, uint32_t _inject_errors,
				   float _encode_time, float _send_time, float _pkt_time,
				   float _latency, float _rssi) :
    sequences(_sequences), blocks_in(_blocks_in), blocks_out(_blocks_out),
    sequence_errors(_sequence_errors), block_errors(_block_errors), inject_errors(_inject_errors),
    bytes_in(_bytes_in), bytes_out(_bytes_out),
    encode_time(_encode_time), send_time(_send_time), pkt_time(_pkt_time), latency(_latency),
    rssi(_rssi) {
}

TransferStats::TransferStats(const std::string &name) :
  m_name(name), m_seq(0), m_blocks(0), m_bytes(0), m_block_errors(0), m_seq_errors(0),
  m_send_bytes(0), m_send_blocks(0), m_inject_errors(0), m_flushes(0), m_queue_size(0),
  m_enc_time(0), m_send_time(0), m_pkt_time(0), m_rssi(0), m_latency(0) {}

void TransferStats::add(const FECDecoderStats &cur, const FECDecoderStats &prev) {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_seq += cur.total_blocks - prev.total_blocks;
  m_blocks += cur.total_packets - prev.total_packets;
  m_bytes += cur.bytes - prev.bytes;
  m_block_errors += cur.dropped_packets - prev.dropped_packets;
  m_seq_errors += cur.dropped_blocks - prev.dropped_blocks;
}
void TransferStats::add_rssi(int8_t rssi) {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_rssi = prev_weight * m_rssi + (1.0 - prev_weight) * rssi;
}

void TransferStats::add_send_stats(uint32_t bytes, uint32_t nblocks, uint16_t inject_errors,
				   uint32_t queue_size, bool flush, float pkt_time) {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_send_bytes += bytes;
  m_send_blocks += nblocks;
  m_inject_errors += inject_errors;
  m_queue_size = prev_weight * m_queue_size + (1.0 - prev_weight) * queue_size;
  m_flushes += (flush ? 1 : 0);
  m_pkt_time = prev_weight * m_pkt_time + 1e6 * (1.0 - prev_weight) * pkt_time;
}

void TransferStats::add_encode_time(float t) {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_enc_time = prev_weight * m_enc_time + 1e6 * (1.0 - prev_weight) * t;
}

void TransferStats::add_send_time(float t) {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_send_time = prev_weight * m_send_time + 1e6 * (1.0 - prev_weight) * t;
}

void TransferStats::add_latency(uint8_t t) {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_latency = prev_weight * m_latency + (1.0 - prev_weight) * t;
}

transfer_stats_t TransferStats::get_stats() {
  std::lock_guard<std::mutex> lock(m_mutex);
  transfer_stats_t stats;
  stats.sequences = m_seq;
  stats.blocks_in = m_blocks;
  stats.blocks_out = m_send_blocks;
  stats.bytes_in = m_bytes;
  stats.bytes_out = m_send_bytes;
  stats.encode_time = m_enc_time;
  stats.send_time = m_send_time;
  stats.pkt_time = m_pkt_time;
  stats.sequence_errors = m_seq_errors;
  stats.block_errors = m_block_errors;
  stats.inject_errors = m_inject_errors;
  stats.latency = m_latency;
  stats.rssi = static_cast<int8_t>(std::round(m_rssi));
  return stats;
}

void TransferStats::update(const std::string &s) {
  std::lock_guard<std::mutex> lock(m_mutex);
  boost::char_separator<char> sep(",");
  boost::tokenizer<boost::char_separator<char> > tok(s, sep);
  boost::tokenizer<boost::char_separator<char> >::iterator i = tok.begin();
  m_name = boost::lexical_cast<std::string>(*i++);
  m_seq = boost::lexical_cast<uint32_t>(*i++);
  m_blocks = boost::lexical_cast<uint32_t>(*i++);
  m_bytes = boost::lexical_cast<uint32_t>(*i++);
  m_block_errors = boost::lexical_cast<uint32_t>(*i++);
  m_seq_errors = boost::lexical_cast<uint32_t>(*i++);
  m_send_bytes = boost::lexical_cast<uint32_t>(*i++);
  m_send_blocks = boost::lexical_cast<uint32_t>(*i++);
  m_inject_errors = boost::lexical_cast<uint32_t>(*i++);
  m_queue_size = boost::lexical_cast<float>(*i++);
  m_enc_time = boost::lexical_cast<float>(*i++);
  m_send_time = boost::lexical_cast<float>(*i++);
  m_pkt_time = boost::lexical_cast<float>(*i++);
  m_latency = boost::lexical_cast<float>(*i++);
  m_rssi = boost::lexical_cast<float>(*i++);
}

std::string TransferStats::serialize() {
  std::lock_guard<std::mutex> lock(m_mutex);
  std::stringstream ss;
  ss << std::setprecision(6)
     << m_name << ","
     << m_seq << ","
     << m_blocks << ","
     << m_bytes << ","
     << m_block_errors << ","
     << m_seq_errors << ","
     << m_send_bytes << ","
     << m_send_blocks << ","
     << m_inject_errors << ","
     << m_queue_size << ","
     << m_enc_time << ","
     << m_send_time << ","
     << m_pkt_time << ","
     << m_latency << ","
     << m_rssi;
  return ss.str();
}

const std::string &TransferStats::name() {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_name;
}
