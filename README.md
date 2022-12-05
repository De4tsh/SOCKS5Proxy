# SOCKS5 代理 Server 与 Client 的实现

# SOCK5 协议

`socks` 是一种互联网协议，它通过一个代理服务器在客户端和服务端之间交换网络数据，所以其本身就是一种代理协议，在 `Client` 与 `Server` 之间转发数据

`socks5` 在 `socks4` 的基础上新增了 `UDP` 转发 和 认证功能，于 `1996` 年正式发布，`socks5` 不支持 `socks4` ，`socks5` 的诞生的原因是因为：在互联网早期，企业内部为了保证安全性，都位于防火墙后，早期为了解决外部访问公司内部的资源的问题，诞生 `socks5`

## SOCKS5 的应用

但目前早已不在这方面使用， 多用于本地代理中，比如在之前打靶过程中通过 `venom` 实现内网穿透时，就是先将位于本机的 `venom` 服务端运行，并启动 `socks5` 代理监听 `1080` 端口，之后当位于本机的 `venom` 服务端被传到目标内网中的 `venom` 客户端连接后，我们通过配置 `proxychain` 就可将本机的扫描流量代理传送到目标内网中，还可配置浏览器的 `socks5` 代理将浏览器的流量引向目标内网

![image-20221114210345008](https://raw.githubusercontent.com/De4tsh/typoraPhoto/main/img/202211142103150.png)

## SOCKS5 的交互过程

![client-socks5_f](https://raw.githubusercontent.com/De4tsh/typoraPhoto/main/img/202211142106713.jpg)

1. 1. `SOCKS5` 握手阶段

      1. 协商阶段

         在客户端正式向 `SOCKS5 Server` 发起请求前，双方要先进行协商，协商的内容主要为：

         - 协议版本
         - 支持的认证方式
           - 支持认证方式的数量
           - 支持哪些认证方式（每种认证方式用 `1` 字节标识）

      2. 认证阶段（可选）

   2. `SOCKS5` 请求阶段

      协商成功后，按照协商的内容，客户端向 `SOCKS5 Server` 发起请求

      请求的内容包括：要访问目标的 域名/`IP`、端口等信息

   3. `SOCKS5 relay` 阶段

      `SOCKS5` 代理收到客户端的请求后，解析请求代替客户端向目标服务器发起 `TCP` 连接

# SOCKS5 详细协议细节

## 握手阶段

### 协商阶段

**<u>客户端 发送报文格式</u>** ：（第一行为含义，第二行为所占字节数）

| **VER** | **NMETHODS** | **METHODS** |
| ------- | ------------ | ----------- |
| `1`     | `1`          | `1 to 255`  |

- **VER** 协议版本 `SOCKS5` 为 `0x05` 
- **NMETHODS** 支持认证的方法数量
- **METHODS** 所支持的每种方法分别是什么

`sizeof(METHODS)` ==  `NMETHODS` `->` `true` 由于每个方法的种类由 `1` 个字节表示，所以有多少种方法，`METHODS` 大小就为多少字节

- `0x00` NO AUTHENTICATION REQUIRED **无需认证**
- `0x01` GSSAPI 
- `0x02` USERNAME/PASSWORD **用户名/密码**
- `0x03 to 0x7F` IANA ASSIGNED **INAN 指定**
- `0x80 to 0xFE` RESERVED FOR PRIVATE METHODS **为私有方法保留**
- `0xFF` NO ACCEPTABLE METHODS **无可接受的方法**

**<u>服务端 返回报文格式</u>** ：（第一行为含义，第二行为所占字节数）

| **VER** | **METHOD** |
| ------- | ---------- |
| `1`     | `1`        |

服务端所选中的方法 `METHOD`

如上所述，

- 返回 `FF` 就说明无可接受的方法

- 返回 `00` 标识不进行认证阶段直接请求即可

### 认证阶段（可选）

若上一阶段选择了 `0x02` 这种用户名密码的认证方式，则会进行到此阶段

**<u>客户端 发送报文格式</u>** ：（第一行为含义，第二行为所占字节数）

| **VER** | **ULEN** | **UNAME**  | **PLEN** | **PASASWD** |
| ------- | -------- | ---------- | -------- | ----------- |
| `1`     | `1`      | `1 to 255` | `1`      | `1 to 255`  |

- **VER** 版本 通常为 `0x01`
- **ULEN** 用户名长度 （ `UNAME` 所占的字节数 ）
- **UNAME** 用户名
- **PLEN** 密码长度 （ `PASASWD` 所占的字节数 ）
- **PASASWD** 密码

**<u>服务端 返回报文格式</u>** ：（第一行为含义，第二行为所占字节数）

| **VER** | **STATUS** |
| ------- | ---------- |
| 1       | 1          |

服务端接收到客户端的认证请求后，使用其中携带的认证信息在本地进行验证，验证信息是否合法，并将状态返回给客户端

- **STATUS** 认证状态
  - `0x00` 认证成功
  - 其余的值表示认证失败

客户端收到认证失败的响应后，就会断开连接

## 请求阶段

顺利通过握手阶段后，**客户端将向服务器发送 `SOCKS5` 请求报文**，**用于告知告知代理服务器目标地址**

**<u>客户端 发送报文格式</u>** ：（第一行为含义，第二行为所占字节数）

| **VER** | **CMD** | **RSV** | **ATYP** | **DST.ADDR** | **DST.PORT** |
| ------- | ------- | ------- | -------- | ------------ | ------------ |
| `1`     | `1`     | `0x00`  | `1`      | `Variable`   | `2`          |

- **VER** 协议版本 `SOCKS5` 为 `0x05` 
- **CMD** 告知服务端需要进行的操作
  - `CONNECT` `0x01` 发起 `connect()` 连接
  - `BIND` `0x02` 端口绑定（在 `Server` 上监听一个端口）
  - `UDP ASSOCIATE` `0x03` 使用 `UDP`
- **RSV** 保留字段 值为 `0x00`
- **ATYP** 目标地址类型 根据类型的不同 `DST.ADDR` 的长度不同
  - `0x01` 表示为 `IPv4` 地址 **`DST.ADDR`为 `4` 字节**
  - `0x03` 表示域名 **`DST.ADDR`是一个可变长度的域名**
  - `0x04` 表示为 `IPv6` 地址 **`DST.ADDR`为 `16` 字节长度**
- **DST.ADDR** 可变长度，用于存放 IP/域名
- **DST.PORT** 目标端口 `2` 字节

**<u>服务端 返回报文格式</u>** ：（第一行为含义，第二行为所占字节数）

| **VER** | **REP** | **RSV** | **ATYP** | **BND.ADDR** | **BND.PORT** |
| ------- | ------- | ------- | -------- | ------------ | ------------ |
| `1`     | `1`     | `0x00`  | `1`      | `Variable`   | `2`          |

- **VER** 协议版本 `SOCKS5` 为 `0x05` 
- **REP** 服务器返回的状态码
  - `0x00` succeeded
  - `0x01` general SOCKS server failure
  - `0x02` connection not allowed by ruleset
  - `0x03` Network unreachable
  - `0x04` Host unreachable
  - `0x05` Connection refused
  - `0x06` TTL expired
  - `0x07` Command not supported
  - `0x08` Address type not supported
  - `0x09` to `0xff` unassigned
- **RSV** 保留字段 值为 `0x00`
- **ATYP** 是目标地址类型，有如下取值
  - `0x01` IPv4
  - `0x03` 域名
  - `0x04` IPv6
- **BND.ADDR** 服务绑定的地址
- **BND.PORT **服务绑定的端口 `DST.PORT`

`BND.ADDR `和 `BND.PORT` 表示服务器的地址和接口，当 `CMD` 为 `0x01` 的情况下，绝大多数客户

端会忽略这两个字端

<!-- 为什么会有这里两个字段：看上图中，可以发现在图中 socks5 既充当 socks 服务器，又充当 relay(中继) 服务器。实际上这两个是可以被拆开的，当我们的 socks 服务器 和 relay(中继) 服务器 不是一体的，就需要告知客户端 relay(中继) 服务器 的地址，这个地址就是 BND.ADDR 和 BND.PORT。当我们的 relay(中继) 服务器 和 socks 服务器 是同一台服务器时，BND.ADDR 和 BND.PORT 的值全部为 0 即可 -->

## Relay 阶段

`socks5` 服务器收到请求后，解析内容。如果是 `UDP` 请求，服务器直接转发; 如果是 `TCP` 请求，服务器向目标服务器建立 `TCP` 连接，后续负责把客户端的所有数据转发到目标服务。


# 实现之中的问题与解决

## 1：返回报文的 `_toString` 转换函数

`char* Sock5Response_toString(struct SOCK5_VALID_REP response,short int judge)` 

由于在返回报文的时候不是将对应的结构体直接返回，而是将结构体中的每个字段的值拿出来放入形成一个字符串返回回去，所以这就涉及到在各个阶段中需要不同的 `_toString` 函数，于是想将这些函数聚集为一个，但这势必就面临一个问题，不同的阶段返回的参数是不同的，返回的值对应的结构体是不同的，由于 `C` 本身不支持默认参数与重载所以目前的想法是：定义一个大结构体，其中包含所有返回包的结构体，此后再根据需要选择即可（但这样肯定会造成资源的浪费）

可以用联合体，就不会浪费空间了... 但需要改变原来使用到这几个结构体的地方

```C
union SOCKS_REP
{
    struct SOCK5_VALID_REP valid_rep;
    struct SOCK5_AUTH_REP auth_rep;
    struct SOCK5_BUILD_REP build_rep;
};
```
改为 `char* Sock5Response_toString(union SOCKS_REP response,short int judge)`

# 参考文章

https://wiyi.org/socks5-protocol-in-deep.html#25-%E5%8D%8F%E8%AE%AE%E7%BB%86%E8%8A%82

https://jiajunhuang.com/articles/2019_06_06-socks5.md.html

https://www.quarkay.com/code/383/socks5-protocol-rfc-chinese-traslation
