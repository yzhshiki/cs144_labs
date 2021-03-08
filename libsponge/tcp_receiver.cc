#include "tcp_receiver.hh"
#include "tcp_helpers/tcp_header.hh"
#include "tcp_helpers/tcp_segment.hh"
// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    const TCPHeader header = seg.header();
    //先把seqno存下来，之后若检测到syn则+1，否则不变。
    WrappingInt32 seqno = header.seqno;
    if(header.syn){
        //isn就是第一个seqno
        _isn = header.seqno;
        _isn_set = true;
        seqno = seqno + 1;
    }
    //没收到syn，收到了其他segment时，直接返回。
    else if(_isn_set == false)
        return;
    //下面的index、absolute seqno、seqno的关系看讲义
    size_t checkpoint = _reassembler.stream_out().bytes_written();
    size_t abs_seqno = unwrap(seqno, _isn, checkpoint);
    size_t stream_index = abs_seqno - 1;
    //将数据写入字节流
    _reassembler.push_substring(seg.payload().copy(), stream_index, seg.header().fin);
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if(_isn_set == false)
        return std::nullopt;
    //如果已经收到FIN，因为FIN占用一个seqno，所以要比平时+1。
    if(_reassembler.stream_out().input_ended())
        return WrappingInt32(wrap(_reassembler.stream_out().bytes_written()+1, _isn)) + 1;
    return WrappingInt32(wrap(_reassembler.stream_out().bytes_written()+1, _isn));
}

size_t TCPReceiver::window_size() const {
//    return _capacity - _reassembler.unassembled_bytes();
    //
    return _capacity - (_reassembler.stream_out().bytes_written() - _reassembler.stream_out().bytes_read());
}
