#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include "ws_client.h"

using namespace godot;

class ZappyWS : public Node
{
    GDCLASS(ZappyWS, Node);

protected:
    static void _bind_methods()
    {
        ClassDB::bind_method(D_METHOD("init"), &ZappyWS::init);
        ClassDB::bind_method(D_METHOD("send", "msg"), &ZappyWS::send);
        ClassDB::bind_method(D_METHOD("recv"), &ZappyWS::recv);
        ClassDB::bind_method(D_METHOD("close"), &ZappyWS::close);
    }

public:
    void init()
    {
        int ret = ws_init();
        if (ret != 0)
        {
            UtilityFunctions::print("[ZappyWS] Failed to initialize WebSocket client");
        }
        else
        {
            UtilityFunctions::print("[ZappyWS] WebSocket client initialized");
        }
    }

    void send(String msg)
    {
        int ret = ws_send(msg.utf8().get_data());
        if (ret < 0)
        {
            UtilityFunctions::print("[ZappyWS] Failed to send message");
        }
        else if (ret == 0)
        {
            UtilityFunctions::print("[ZappyWS] ws_send(): would block, try later");
        }
        else
        {
            UtilityFunctions::print("[ZappyWS] Sent ", ret, " bytes");
        }
    }

    String recv()
    {
        char buf[1024];
        int r = ws_recv(buf, sizeof(buf));
        if (r > 0)
        {
            return String::utf8(buf);
        }
        else if (r == 0)
        {
            return String();
        }
        else if (r == -2)
        {
            UtilityFunctions::print("[ZappyWS] Connection closed by server");
            return String();
        }
        else
        {
            UtilityFunctions::print("[ZappyWS] ws_recv() error");
            return String();
        }
    }




    void close() { ws_close(); }
};

