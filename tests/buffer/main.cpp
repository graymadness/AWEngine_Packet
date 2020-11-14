#include <AWEngine/Packet/PacketBuffer.hpp>

#include <cassert>

int main(int argc, const char** argv)
{
    using namespace AWEngine::Packet;

    PacketBuffer pb = PacketBuffer();

    uint8_t u8 = 1, u8_;
    int8_t i8 = 13, i8_;

    uint16_t u16 = (12 << 8) + 4, u16_;
    int16_t i16 = (21 << 8) + 40, i16_;

    uint32_t u32 = (1 << 24) + (12 << 16) + (31 << 8) + 4, u32_;
    int32_t i32 = (8 << 24) + (42 << 16) + (21 << 8) + 40, i32_;

    uint64_t u64 = (1 << 24) + (12 << 16) + (31 << 8) + 4, u64_;
    int64_t i64 = (8 << 24) + (42 << 16) + (21 << 8) + 40, i64_;

    float f = 3.1415926535, f_;
    double d = 6.283185307, d_;

    std::string s = "Lorem Ipsum", s_;

    pb << u8 << i8;
    pb << u16 << i16;
    pb << u32 << i32;
    pb << u64 << i64;
    pb << f << d;
    pb << s;

    std::cout << "size: " << pb.size() << std::endl;

    pb >> u8_ >> i8_;
    assert(u8 == u8_);
    assert(i8 == i8_);

    pb >> u16_ >> i16_;
    assert(u16 == u16_);
    assert(i16 == i16_);

    pb >> u32_ >> i32_;
    assert(u32 == u32_);
    assert(i32 == i32_);

    pb >> u64_ >> i64_;
    assert(u64 == u64_);
    assert(i64 == i64_);

    pb >> f_ >> d_;
    assert(f == f_);
    assert(d == d_);

    pb >> s_;
    assert(s == s_);

    assert(pb.size() == 0);
}
