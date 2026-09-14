#include "WSTask.h"
#include "websocket/WebSocketFrame.h"
#include "transport/BufferedReader.h"
#include <utility>
#include <string>

WSTask::WSTask(Connection conn) : connection_(std::move(conn)) {}


void WSTask::run_task() {
    printf("WebSocket uprade established\n");

    BufferedReader reader(connection_);
    size_t pOpc = 0;
    std::string accumulator_;

    for(;;) {
        tcp::Result<WsFrame> try_parse = WebSocket::parse_frame(reader);
        if (!try_parse) {
            fprintf(stderr, "WS parse failed: code %d\n", try_parse.error());
            return;
        }
        size_t opcode_ = try_parse.value().opcode;
        bool finbit_ = try_parse.value().finbit;

        switch(opcode_)
        {
            //Echo
            case(0x0):
            {
                if(pOpc == 0) {
                    fprintf(stderr, "Protocol Error: Continuation with nothing open to continue."); return;
                }
                WsFrame ret;
                if(finbit_) {
                    ret.finbit = true;
                    ret.opcode = pOpc;
                } else {
                    ret.finbit = false;
                }

                accumulator_ += try_parse.value().payload;

                if(finbit_) {
                    ret.payload = accumulator_;

                    std::string out = WebSocket::serialize_frame(ret);
                    tcp::Result<void> try_write = connection_.write_all(out.data(), out.size());

                    if(!try_write) {
                        fprintf(stderr, "Connection write error failed: code %d\n", try_write.error());
                    }
                    accumulator_.clear();
                    pOpc = 0;
                }
            }
            break;
            //Text
            case(0x1):
                {
                    if(pOpc != 0) {
                        fprintf(stderr, "Protocol Error: The client started a new message before finishing the last fragmented one."); return;
                    }
                    WsFrame ret;
                    if(finbit_) {
                        ret.finbit = true;
                        ret.opcode = 0x1;
                    } else {
                        ret.finbit = false;
                        pOpc = opcode_;
                    }
                    if(finbit_) {
                        ret.payload = try_parse.value().payload;
                        std::string out = WebSocket::serialize_frame(ret);
                        tcp::Result<void> try_write = connection_.write_all(out.data(), out.size());

                        if(!try_write) {
                            fprintf(stderr, "Connection write error failed: code %d\n", try_write.error());
                        }
                    } else {
                        accumulator_ += try_parse.value().payload;

                    }
                }
                break;

            //Binary
            case(0x2):
                {
                    if(pOpc != 0) {
                        fprintf(stderr, "Protocol Error: The client started a new message before finishing the last fragmented one."); return;
                    }
                    WsFrame ret;
                    if(finbit_) {
                        ret.finbit = true;
                        ret.opcode = 0x2;
                    } else {
                        ret.finbit = false;
                        pOpc = opcode_;
                    }

                    if(finbit_) {
                        ret.payload = try_parse.value().payload;
                        std::string out = WebSocket::serialize_frame(ret);
                        tcp::Result<void> try_write = connection_.write_all(out.data(), out.size());

                        if(!try_write) {
                            fprintf(stderr, "Connection write error failed: code %d\n", try_write.error());
                        }
                    } else {
                        accumulator_ += try_parse.value().payload;
                    }
                }
                break;
            //Close
            case(0x8):
                    {
                        WsFrame ret;
                        ret.finbit = true;
                        ret.opcode = 0x8;
                        ret.payload = try_parse.value().payload;

                        std::string out = WebSocket::serialize_frame(ret);
                        tcp::Result<void> try_write = connection_.write_all(out.data(), out.size());
                        if(!try_write) {
                            fprintf(stderr, "Connection write error failed: code %d\n", try_write.error());
                        }
                    }
                    return;
            //Ping
            case(0x9):
                {
                    WsFrame ret;
                    ret.finbit = true;
                    ret.opcode = 0xA;
                    ret.payload = try_parse.value().payload;

                    std::string out = WebSocket::serialize_frame(ret);
                    tcp::Result<void> try_write = connection_.write_all(out.data(), out.size());
                    if(!try_write) {
                        fprintf(stderr, "Connection write error failed: code %d\n", try_write.error());
                    }
                }
                break;
            //Pong
            case(0xA):
                break;
        }
    }
    return;
}
