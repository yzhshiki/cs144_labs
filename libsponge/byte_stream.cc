#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

// 需要使用初始化列表来初始化
ByteStream::ByteStream(const size_t capacity) :
    _buf({}),
    _capacity(capacity),
    _write_count(0),
    _read_count(0),
    _is_eof(false)
{}

size_t ByteStream::write(const string &data) {
    size_t len = data.size();
    if(len > _capacity - _buf.size()){
        len = _capacity - _buf.size();
    }
    _buf += data.substr(0, len);
    _write_count += len;
    return len;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    return _buf.substr(0, len);
}

//! \param[in] len bytes will be removed from the output side of the buffer
// 需要在这一步增加read_count
void ByteStream::pop_output(const size_t len) {
    _read_count += len;
    _buf.erase(0, len);
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    std::size_t read_len = len > _buf.size()? _buf.size() : len;
    string ans = peek_output(read_len);
    pop_output(read_len);
    return ans;
}

void ByteStream::end_input() { _is_eof = 1; }

bool ByteStream::input_ended() const { return _is_eof; }

size_t ByteStream::buffer_size() const { return _buf.size(); }

bool ByteStream::buffer_empty() const { return _buf.size() == 0; }

bool ByteStream::eof() const { return input_ended() && buffer_empty(); }

size_t ByteStream::bytes_written() const { return _write_count; }

size_t ByteStream::bytes_read() const { return _read_count; }

size_t ByteStream::remaining_capacity() const { return _capacity - _buf.size(); }
