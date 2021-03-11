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
    ,_window_size(0)
    ,_true_window_size(0)
    ,_consecutive_retransmission(0)
    ,_fin_flag(false)
    ,timer(retx_timeout){}

uint64_t TCPSender::bytes_in_flight() const { return _bytes_in_flight; }

void TCPSender::fill_window() {
//    printf("***Start fill window***\n");
//    printf("finflag: %d, _window_size:%d _true_window_size: %d\n",_fin_flag, _window_size, _true_window_size);
    if(_fin_flag)
        return;
    bool zero_flag = false;
    //字节流中下一个字节的seqno
    WrappingInt32 _next_seq32 = wrap(_next_seqno, _isn);
    if(_next_seqno == 0){
//        printf("**Start SYN**\n");
        TCPSegment segment;
        segment.header().seqno = _next_seq32;
        segment.header().syn = true;
        sendSegment(segment);
//        printf("**End SYN**\n");
        return;
    }
    //如果这个字节的seqno在窗口内，则发送
    if(_true_window_size == 0){
        zero_flag = true;
        _window_size = 1;
        _true_window_size = 1;
    }

    size_t remain;
    while((remain = _true_window_size - (_next_seq32.raw_value() - _ackGot.raw_value()))!=0 && !_fin_flag){
//        printf("**Common data**\n");
//        printf("_next_32: %u, ackGot: %u, windowsize: %d, stream empty: %d\n", _next_seq32.raw_value(), _ackGot.raw_value(), _window_size, _stream.buffer_empty());
        TCPSegment segment;
        segment.header().seqno = _next_seq32;
        if(_next_seqno == 0)
            segment.header().syn = true;
        //注意Buffer的构造函数，只允许传右值。
        size_t dataLen = min(size_t(remain), TCPConfig::MAX_PAYLOAD_SIZE);
        segment.payload() = _stream.read(dataLen);
        if(_stream.eof() && _window_size >= segment.length_in_sequence_space() + 1){
//            printf("**Start FIN**\n");
            segment.header().fin = true;
            _fin_flag = true;
//            printf("**End FIN**\n");
        }

        //important
        if(segment.length_in_sequence_space()==0){
            return;
        }
        sendSegment(segment);
        _next_seq32 = wrap(_next_seqno, _isn);
    }
    if(zero_flag){
        _window_size = 0;
        _true_window_size = 0;
    }
//    printf("***End fill window***\n");

}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
//    printf("***Start receive***\n");
    while(!outSegments.empty()){
        TCPSegment segment = outSegments.front();
        if(segment.header().seqno.raw_value() + segment.length_in_sequence_space() <= ackno.raw_value()){
            _bytes_in_flight -= segment.length_in_sequence_space();
            outSegments.pop();
        }
        else
            break;
    }
    if(ackno.raw_value() > _ackGot.raw_value()) {
        timer.resetTime();
        timer.resetRTO(_initial_retransmission_timeout);
        if (!outSegments.empty())
            timer.start();
        _consecutive_retransmission = 0;
    }
    _window_size = window_size;
    _true_window_size = window_size;
    if(ackno.raw_value() >= _ackGot.raw_value()) {
        _ackGot = ackno;
//        _window_size = window_size;

    }
//    printf("Receiving ackno: %u, ackGot: %u window size: %d\n",ackno.raw_value(), _ackGot.raw_value(),_window_size);
    fill_window();
//    printf("***End receive***\n");
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    timer.passTime(ms_since_last_tick);
    if(!timer.isRunning() && !outSegments.empty()){
        TCPSegment segment = outSegments.front();
        _segments_out.push(segment);
//        printf("resending : %s\n", segment.payload().copy().c_str());
//        printf("Ticking and window size: %d\n",_window_size);
        if(_true_window_size!=0 || _next_seqno == 1){
            _consecutive_retransmission++;
            timer.doubleRTO();
//            printf("Doubled rto\n");
        }
        timer.start();
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retransmission; }

void TCPSender::send_empty_segment() {}

void TCPSender::sendSegment(TCPSegment &segment) {
//    printf("sending : %s\n", segment.payload().copy().c_str());
//    printf("Sending....\n");
//    printf("window size: %d, dataLen: %ld, streamLen: %ld\n",_window_size, segment.length_in_sequence_space(),stream_in().buffer_size());
//    printf("TCPHeader Summary: %s with %ld space and %ld bytes\n", segment.header().summary().c_str(), segment.length_in_sequence_space(), segment.payload().size());
    _segments_out.push(segment);
    //如果不是空报文段，则加入暂存区
//        if(dataLen!=0)
    outSegments.push(segment);
    _next_seqno += segment.length_in_sequence_space();
    _bytes_in_flight += segment.length_in_sequence_space();
    if(_window_size >= segment.length_in_sequence_space())
        _window_size -= segment.length_in_sequence_space();
    else
        _window_size = 0;
    if(!timer.isRunning()){
        timer.resetTime();
        timer.start();
    }
//    printf("After send,  window size: %d\n",_window_size);
//    printf("\n next_seqno: %ld %ld\n",_next_seqno, bytes_in_flight());
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
