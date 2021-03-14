#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity)
    , _bytes_in_flight(0)
    ,_ackGot(_isn.raw_value())
    ,_true_window_size(0)
    ,_consecutive_retransmission(0)
    ,_fin_flag(false)
    ,timer(retx_timeout){}

uint64_t TCPSender::bytes_in_flight() const { return _bytes_in_flight; }

void TCPSender::fill_window() {
    if(_fin_flag)
        return;
    //用于区分窗口为1、窗口为0视作1的情况
    bool zero_flag = false;
    //字节流中下一个字节的seqno
    WrappingInt32 _next_seq32 = wrap(_next_seqno, _isn);
    //字节流中下一个字节的absolute seqno为0 说明还没有握手
    if(_next_seqno == 0){
        TCPSegment segment;
        segment.header().seqno = _next_seq32;
        segment.header().syn = true;
        sendSegment(segment);
        return;
    }
    if(_true_window_size == 0){
        zero_flag = true;
        _true_window_size = 1;
    }

    size_t remain;
    //判断窗口剩余空间，是否还能发送
    while((remain = _true_window_size - (_next_seq32.raw_value() - _ackGot.raw_value()))!=0 && !_fin_flag){
        TCPSegment segment;
        segment.header().seqno = _next_seq32;
        //注意Buffer的构造函数，只允许传右值（即常数等不能放在等式左边，有地址的值）。
        size_t dataLen = min(size_t(remain), TCPConfig::MAX_PAYLOAD_SIZE);
        segment.payload() = _stream.read(dataLen);
        //字节流已结束，且有空间留给FIN标志
        if(_stream.eof() && remain >= segment.length_in_sequence_space() + 1){
            segment.header().fin = true;
            _fin_flag = true;
        }

        //字节流是空的，但没有eof(这是由使用者指定的，并不是空了就结束了)。
        if(segment.length_in_sequence_space()==0){
            return;
        }
        sendSegment(segment);
        //更新seqno用于循环判断
        _next_seq32 = wrap(_next_seqno, _isn);
    }
    if(zero_flag){
        _true_window_size = 0;
    }

}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    if(ackno.raw_value() > wrap(_next_seqno, _isn).raw_value())
        return;
    //将已发送报文段中，完全被ack的那些丢弃。
    while(!outSegments.empty()){
        TCPSegment segment = outSegments.front();
        if(segment.header().seqno.raw_value() + segment.length_in_sequence_space() <= ackno.raw_value()){
            _bytes_in_flight -= segment.length_in_sequence_space();
            outSegments.pop();
        }
        else
            break;
    }
    //收到一个比之前的ackno都大的新ackno后
    if(ackno.raw_value() > _ackGot.raw_value()) {
        _ackGot = ackno;
        timer.resetTime();
        timer.resetRTO(_initial_retransmission_timeout);
        if (!outSegments.empty())
            timer.start();
        _consecutive_retransmission = 0;
    }
    _true_window_size = window_size;
    fill_window();
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    timer.passTime(ms_since_last_tick);
    if(!timer.isRunning() && !outSegments.empty()){
        TCPSegment segment = outSegments.front();
        _segments_out.push(segment);
        if(_true_window_size!=0 || _next_seqno == 1){
            _consecutive_retransmission++;
            timer.doubleRTO();
        }
        timer.start();
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retransmission; }

void TCPSender::send_empty_segment() {}

void TCPSender::sendSegment(TCPSegment &segment) {
    _segments_out.push(segment);
    outSegments.push(segment);
    _next_seqno += segment.length_in_sequence_space();
    _bytes_in_flight += segment.length_in_sequence_space();
    if(!timer.isRunning()){
        timer.resetTime();
        timer.start();
    }
}

RETimer::RETimer(unsigned int _initial_retrans_timeout) :
    rto(_initial_retrans_timeout)
    ,_time(0)
    ,_isrunning(true){}
bool RETimer::isRunning() { return _isrunning; }
void RETimer::passTime(unsigned int passedTime) {
    if(!isRunning())
        return;
    _time += passedTime;
    if(_time >= rto){
        _isrunning = false;
        _time = 0;
    }
}
void RETimer::doubleRTO() {
    rto *= 2;
}
void RETimer::resetTime(){
    _time = 0;
}
void RETimer::resetRTO(unsigned int _initial_retrans_timeout) {
    rto = _initial_retrans_timeout;
}
void RETimer::start() { _isrunning = true;}
