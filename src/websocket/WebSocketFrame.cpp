#include "WebSocketFrame.h"


std::string WebSocket::serialize_frame(const WsFrame& in) {

    std::string ret;
    ret += static_cast<char>((in.finbit << 7) | (in.opcode & 0x0F));

    if (in.payload.size() <= 125) {
        ret += static_cast<char>(in.payload.size());
    } else if (in.payload.size() >= 126 && in.payload.size() <= 65535) {
        ret += static_cast<char>(126);
        //Top bits
        ret += static_cast<char>((in.payload.size() >> 8) & 0xFF);

        //Bottom bits
        ret += static_cast<char>(in.payload.size() & 0xFF);
    } else if (in.payload.size() > 65535) {

        ret += static_cast<char>(127);

        //High - Low shift/masks
        ret += static_cast<char>((in.payload.size() >> 56) & 0xFF);
        ret += static_cast<char>((in.payload.size() >> 48) & 0xFF);
        ret += static_cast<char>((in.payload.size() >> 40) & 0xFF);
        ret += static_cast<char>((in.payload.size() >> 32) & 0xFF);
        ret += static_cast<char>((in.payload.size() >> 24) & 0xFF);
        ret += static_cast<char>((in.payload.size() >> 16) & 0xFF);
        ret += static_cast<char>((in.payload.size() >> 8) & 0xFF);
        ret += static_cast<char>(in.payload.size() & 0xFF);
    }
    ret += in.payload;
    return ret;
}

tcp::Result<WsFrame> WebSocket::parse_frame(BufferedReader& reader) {
    WsFrame local_frame;
    tcp::Result<tcp::BufferedResult> try_read = reader.read_exact(2);
    if(!try_read) {
        //WebSocket read error
        return tcp::Result<WsFrame>::err(try_read.error());
    }
    if(!try_read.value().complete) {
        //Incomplete read ?
        return tcp::Result<WsFrame>::err(-1);
    }

    size_t b0 = try_read.value().bytes[0];
    size_t finbit_ = (b0 >> 7) & 0x1;
    size_t opcode_ = b0 & 0x0F;

    local_frame.finbit = finbit_;
    local_frame.opcode = opcode_;

    size_t b1 = try_read.value().bytes[1];

    size_t top = (b1 >> 7) & 0x1;
    if (top != 1) {
        return tcp::Result<WsFrame>::err(-1); // RFC 6455 5.1: client frames must be masked
    }
    size_t bot = b1 & 0x7F;

    if(bot == 126) {

        tcp::Result<tcp::BufferedResult> next2 = reader.read_exact(2);

        if(!next2) {
            //WebSocket read error
            return tcp::Result<WsFrame>::err(next2.error());
        }
        if(!next2.value().complete) {
            //Incomplete read ?
            return tcp::Result<WsFrame>::err(-1);
        }
        bot = (static_cast<unsigned char>(next2.value().bytes[0]) << 8) | static_cast<unsigned char>(next2.value().bytes[1]);

    } else if (bot == 127) {
        tcp::Result<tcp::BufferedResult> next8 = reader.read_exact(8);
        if(!next8) {
            //WebSocket read error
            return tcp::Result<WsFrame>::err(next8.error());
        }
        if(!next8.value().complete) {
            //Incomplete read ?
            return tcp::Result<WsFrame>::err(-1);
        }

        bot = (static_cast<size_t>(static_cast<unsigned char>(next8.value().bytes[0])) << 56)
            | (static_cast<size_t>(static_cast<unsigned char>(next8.value().bytes[1])) << 48)
            | (static_cast<size_t>(static_cast<unsigned char>(next8.value().bytes[2])) << 40)
            | (static_cast<size_t>(static_cast<unsigned char>(next8.value().bytes[3])) << 32)
            | (static_cast<size_t>(static_cast<unsigned char>(next8.value().bytes[4])) << 24)
            | (static_cast<size_t>(static_cast<unsigned char>(next8.value().bytes[5])) << 16)
            | (static_cast<size_t>(static_cast<unsigned char>(next8.value().bytes[6])) << 8)
            |  static_cast<size_t>(static_cast<unsigned char>(next8.value().bytes[7]));
    }

    if ((opcode_ & 0x8) && (!finbit_ || bot > 125)) {
        return tcp::Result<WsFrame>::err(-1);
    }

    if (bot > kMaxWsFramePayload) {
        return tcp::Result<WsFrame>::err(-1);
    }

    std::string mask_key;
    tcp::Result<tcp::BufferedResult> next4 = reader.read_exact(4);
    if(!next4) {
        //WebSocket read error
        return tcp::Result<WsFrame>::err(next4.error());
    }
    if(!next4.value().complete) {
        //Incomplete read ?
        return tcp::Result<WsFrame>::err(-1);
    }
    mask_key = next4.value().bytes;

    tcp::Result<tcp::BufferedResult> try_all = reader.read_exact(bot);
    if(!try_all) {
        //WebSocket read error
        return tcp::Result<WsFrame>::err(try_all.error());
    }
    if(!try_all.value().complete) {
        //Incomplete read ?
        return tcp::Result<WsFrame>::err(-1);
    }

    int len = try_all.value().bytes.size();
    for(int i = 0; i < len; i++) {
        local_frame.payload += try_all.value().bytes[i] ^ mask_key[i % 4];
    }
    return tcp::Result<WsFrame>::ok(local_frame);
}
