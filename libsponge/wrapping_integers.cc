#include "wrapping_integers.hh"

// Dummy implementation of a 32-bit wrapping integer

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
//! \param n The input absolute 64-bit sequence number
//! \param isn The initial sequence number
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {
    //截取n的后32位并加上isn即可
    return WrappingInt32(((n << 32) >> 32) + isn.raw_value());
}

//! Transform a WrappingInt32 into an "absolute" 64-bit sequence number (zero-indexed)
//! \param n The relative sequence number
//! \param isn The initial sequence number
//! \param checkpoint A recent absolute 64-bit sequence number
//! \returns the 64-bit sequence number that wraps to `n` and is closest to `checkpoint`
//!
//! \note Each of the two streams of the TCP connection has its own ISN. One stream
//! runs from the local TCPSender to the remote TCPReceiver and has one ISN,
//! and the other stream runs from the remote TCPSender to the local TCPReceiver and
//! has a different ISN.
uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
    //后32位是n - isn
    uint64_t backPart = n.raw_value() - isn.raw_value();
    //前32位是checkpoint的前32位
    uint64_t prePart = checkpoint & (0xFFFFFFFF00000000);
    uint64_t ans = prePart + backPart;
    //在ans,ans+1ul<<32,ans-1ul<<32三个中取距离checkpoint最近的一个
    //这里比较差值大小要用int64而不是unsigned int64，因为小数减大数我们需要的就是负数的绝对值
    if(abs(int64_t(ans + (1ul << 32) - checkpoint)) < abs(int64_t(ans - checkpoint)))
        return ans + (1ul << 32);
    //需要判断ans>=(1ul<<32),因为是int64。
    if(ans >= (1ul<<32) && abs(int64_t(ans - (1ul << 32) - checkpoint)) < abs(int64_t(ans - checkpoint)))
        return ans - (1ul << 32);
    return ans;
}
