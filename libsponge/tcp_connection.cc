#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity(); }

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return _time_since_last_segment_received; }

void TCPConnection::segment_received(const TCPSegment &seg) {
    if(!_active)
        return;
    //如果是由对面发起连接，则要注意syn标志前的报文段不接收
    if(!_syn_set && !seg.header().syn)
        return;
    else if(!_syn_set && seg.header().syn){
        _syn_set = true;
        _sender.fill_window();
    }

    _time_since_last_segment_received = 0;
    //如果RST标志已设置，将inbound和outbound流都设为error状态并永远关闭连接。
    if(seg.header().rst){
        if(!_syn_set && !seg.header().ack)
            return;
        _receiver.stream_out().set_error();
        _sender.stream_in().set_error();
        _rst_set = true;
        _active = false;
        return;
    }

    //将报文段传递给TCPReceiver
    _receiver.segment_received(seg);
    //如果ACK标志已设置，则将ackno和window size告诉TCPSender
    if(seg.header().ack){
        _sender.ack_received(seg.header().ackno, seg.header().win);
        //可能是passive close情况下的第四次挥手，所以可能使连接在此结束。
        clean_shutdown();
    }
    //如果已经发送了挥手时的ack报文段，那么连接只需要检测可能的对面的FIN重传
    if(_fin_ack_sent && !seg.header().fin)
        return;
    if(seg.header().fin)
        _rec_fin_seqno = seg.header().seqno;
    if(seg.length_in_sequence_space() > 0){
        sendseg();
    }
    clean_shutdown();
}

bool TCPConnection::active() const { return _active; }

size_t TCPConnection::write(const string &data) {
    size_t Written = _sender.stream_in().write(data);
    _sender.fill_window();
    sendseg();
    return Written;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    if(!_active)
        return;
    _time_since_last_segment_received += ms_since_last_tick;
    _sender.tick(ms_since_last_tick);
    if(_sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS){
        _rst_set = true;
        sendseg();
    }
    clean_shutdown();
    if(!_active)
        return;

    if(!_sender.segments_out().empty()){
        sendseg();
    }

}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    _sender.fill_window();
    _closedByApp = true;
    sendseg();
}

void TCPConnection::connect() {
    _sender.fill_window();
    sendseg();
    return;
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";
            _rst_set = true;
            sendseg();
            // Your code here: need to send a RST segment to the peer
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}

void TCPConnection::sendseg() {
    TCPSegment seg;
    if(_sender.segments_out().empty())
        _sender.segments_out().push(seg);
    while(!_sender.segments_out().empty()){
        seg = _sender.segments_out().front();
        _sender.segments_out().pop();

        if(seg.header().syn)
            _syn_set = true;
        if(_receiver.ackno().has_value()){
            seg.header().ackno = _receiver.ackno().value();
            seg.header().ack = true;
            seg.header().win = _receiver.window_size();
            if(_rec_fin_seqno.has_value() && seg.header().ackno.raw_value() > _rec_fin_seqno.value().raw_value())
                _fin_ack_sent = true;
        }
        if(_rst_set){
            seg.header().rst = true;
            _receiver.stream_out().set_error();
            _sender.stream_in().set_error();
            _active = false;
        }
        segments_out().push(seg);
    }
    clean_shutdown();
}

void TCPConnection::clean_shutdown() {
//    if (_receiver.stream_out().input_ended() && !(_sender.stream_in().eof())) {
//        _linger_after_streams_finish = false;
//    }
//    if (_sender.stream_in().eof() && _sender.bytes_in_flight() == 0 && _receiver.stream_out().input_ended()) {
//        if (!_linger_after_streams_finish || time_since_last_segment_received() >= 10 * _cfg.rt_timeout) {
//            _active = false;
//        }
//    }
    if(_receiver.stream_out().input_ended() && !(_sender.stream_in().eof()))
        _linger_after_streams_finish = false;

    if(_receiver.unassembled_bytes() == 0 && _receiver.stream_out().input_ended()){
        if(_sender.stream_in().input_ended() && _sender.segments_out().empty()){
            if(_sender.bytes_in_flight() == 0){
                if(_time_since_last_segment_received >= 10 * _cfg.rt_timeout || _linger_after_streams_finish == false)
                    _active = false;
            }
        }
    }
}