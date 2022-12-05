#include <string.h>
#include <stdlib.h>
#include "sock5.h"
#include "common.h"

/// @brief 接收验证信息报文
struct SOCK5_VALID_REQ Sock5ValidReq_read(byte* buf)
{

    struct SOCK5_VALID_REQ request;

    request.version = buf[0];
    request.method_num = buf[1];

    // 其实没必要，因为只能指定一种方式，若大于一种方式接收函数是接收不到的
    request.methods = (byte *)malloc(request.method_num);


    byteCopy(request.methods,buf + 2,request.method_num);


    return request;
}

/// @brief 读取用于认证的用户名和密码
struct SOCK5_AUTH_REQ Sock5AuthReq_read(byte* buf)
{
    struct SOCK5_AUTH_REQ request;
    request.version = buf[0];
    request.ulen = buf[1];

    request.uname = (byte *)malloc(request.ulen);
    byteCopy(request.uname,buf + 2,request.ulen);

    request.plen = buf[2 + request.ulen];
    request.passwd = (byte *)malloc(request.plen);
    byteCopy(request.passwd,(buf + 2 + request.ulen + 1),request.plen);

    return request;
}


size_t Sock5BuildResponse_getLength(union SOCKS_REP response)
{
    return response.build_rep.addrlen + (response.build_rep.atyp == 0x03 ? 7 : 6);
}


/// @brief 将返回的数据转为字符串（C无法区分第一个参数....）
/// @param response 返回包的结构体
/// @param judge 用于区分当前转为字符串的种类
/// @return 返回传输的数据
char* Sock5Response_toString(union SOCKS_REP response,short int judge)
{
    // 0:验证阶段返回报文_转字符串
    if (judge == 0)
    {

        byte* result = (byte *)malloc(sizeof(byte) * 3);
        memset(result,0,sizeof(result));

        int p = 0;

        result[p++] = response.valid_rep.version;
        result[p] = response.valid_rep.methods;

        return (byte *)result;
    }
    // 1:认证阶段返回报文_转字符串
    else if (judge == 1)
    {
        byte* result = (byte *)malloc(sizeof(byte) * 3);
        memset(result,0,sizeof(result));

        int p = 0;

        result[p++] = response.auth_rep.version;
        result[p] = response.auth_rep.status;

        return (byte *)result;
    }
    else if (judge == -1)
    {
        byte *result = (byte *)malloc(Sock5BuildResponse_getLength(response));

        int p = 0;
        result[p++] = response.build_rep.version;
        result[p++] = response.build_rep.rep;
        result[p++] = response.build_rep.rsv;
        result[p++] = response.build_rep.atyp;
        if (response.build_rep.atyp == 0x03)
        {
            result[p++] = response.build_rep.addrlen;
        }
        byteCopy(result + p, response.build_rep.bndaddr, response.build_rep.addrlen);
        p += response.build_rep.addrlen;
        result[p++] = (byte)(response.build_rep.bndport >> 8);
        result[p] = (byte)(response.build_rep.bndport & 0xff);


        return (char *)result;
    }

}

struct SOCK5_BUILD_REQ Sock5BuildRequest_read(byte *buf)
{
    struct SOCK5_BUILD_REQ request;

    int p = 0;
    request.version = buf[p++];
    request.cmd = buf[p++];
    request.rsv = buf[p++];
    request.atyp = buf[p++];


    switch (request.atyp)
    {
        case 0x01: // IPv4
            request.addlen = 4;
            break;

        case 0x03: // 可变域名
            request.addlen = buf[p++];
            break;

        case 0x04: // IPv6
            request.addlen = 16;
            break;

        default:
            request.addlen = 0;
    }

    request.dstaddr = (byte *)malloc(request.addlen);
    byteCopy(request.dstaddr, buf + p, request.addlen);
    p += request.addlen;
    request.dstport = (buf[p] << 8) + buf[p+1];


    return request;
}                                      
