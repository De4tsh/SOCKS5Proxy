
#ifndef __SOCK5_H
#define __SOCK5_H

#include "common.h"

#define SOCK5_VALID_REQ_MAX_LEN 3
// #define SOCK5_VALID_REQ_MAX_LEN 2 + 8

#define SOCK5_BUILD_REQ_MAX_LEN 263


/// @brief 客户端第一个 sock5 请求包结构
struct SOCK5_VALID_REQ
{
    byte version;
    byte method_num;
    //byte* methods; // 画蛇添足
    // byte methods[256];
    // 221125 记得将其初始化为 -1
    byte* methods;
};

/// @brief 服务端对于第一个 sock5 请求包的回应包
struct SOCK5_VALID_REP
{
    byte version;
    byte methods;
};

/// @brief 用户明/密码 身份认证请求报文
struct SOCK5_AUTH_REQ
{
    byte version;
    int ulen;
    byte* uname;
    int plen;
    byte* passwd;
};

/// @brief 用户明/密码 身份认证返回报文
struct SOCK5_AUTH_REP
{
    byte version;
    byte status;
};


struct SOCK5_BUILD_REQ
{
    byte version;
    byte cmd;
    byte rsv;
    byte atyp;
    byte addlen;
    byte dstaddr;
    ushort dstport;
};

struct SOCK5_BUILD_REP
{
    byte version;
    byte rep;
    byte rsv;
    byte atyp;
    byte addrlen; // 地址长度
    byte *bndaddr;
    ushort bndport;
};


union SOCKS_REP
{
    struct SOCK5_VALID_REP valid_rep;
    struct SOCK5_AUTH_REP auth_rep;
    struct SOCK5_BUILD_REP build_rep;
};



/// @brief 接收验证信息报文
struct SOCK5_VALID_REQ Sock5ValidReq_read(byte* buf);

/// @brief 读取用于认证的用户名和密码
struct SOCK5_AUTH_REQ Sock5AuthReq_read(byte* buf);

size_t Sock5BuildResponse_getLength(union SOCKS_REP response) ;


/// @brief 将返回的数据转为字符串（C无法区分第一个参数....）
/// @param response 返回包的结构体
/// @param judge 用于区分当前转为字符串的种类
/// @return 返回传输的数据
char* Sock5Response_toString(union SOCKS_REP response,short int judge);


struct SOCK5_BUILD_REQ Sock5BuildRequest_read(byte *buf);


#endif
