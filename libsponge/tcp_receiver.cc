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
    WrappingInt32 seqno = header.seqno;
    if(header.syn){
        _isn = header.seqno;
        _isn_set = true;
        seqno = seqno + 1;
    }
    else if(_isn_set == false)
        return;
    size_t checkpoint = _reassembler.stream_out().bytes_written();
    size_t abs_seqno = unwrap(seqno, _isn, checkpoint);
    size_t stream_index = abs_seqno - 1;
    _reassembler.push_substring(seg.payload().copy(), stream_index, seg.header().fin);
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if(_isn_set == false)
        return std::nullopt;
    if(_reassembler.stream_out().input_ended())
        return WrappingInt32(wrap(_reassembler.stream_out().bytes_written()+1, _isn)) + 1;
    return WrappingInt32(wrap(_reassembler.stream_out().bytes_written()+1, _isn));
}

size_t TCPReceiver::window_size() const {
//    return _capacity - _reassembler.unassembled_bytes();
    return _reassembler.stream_out().bytes_read() + _capacity - _reassembler.stream_out().bytes_written();
}
