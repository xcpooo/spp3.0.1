/**
 * @brief nest address 
 * @info  屏蔽地址差异, 兼容unixlocal地址与ipv4地址, 可扩展IPv6地址
 */

#ifndef __NEST_ADDRESS_H_
#define __NEST_ADDRESS_H_

#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <vector>

namespace nest
{
    ///< 地址真实类型定义, 可扩展定义, 支持ipv6等
    typedef enum __addr_type
    {
        NEST_ADDR_UNDEF = 0,
        NEST_ADDR_IPV4  = 1,
        NEST_ADDR_UNIX  = 2,
        NEST_ADDR_IPV6  = 3,
    }TNestAddrType;

    ///< 地址实际的buff定义, 可扩展
    typedef union __addr_buff
    {
        struct sockaddr_in  ipv4_addr;
        struct sockaddr_un  unix_addr;
        struct sockaddr_in6 ipv6_addr;
    }TNestAddrBuff;


    /**
     * @brief 通用地址管理, 适配linux接口
     */
    class TNestAddress
    {
    public:

        /**
         * @brief 构造函数与析构函数
         */
        TNestAddress() {
            _type       = NEST_ADDR_UNDEF;
            _addr_len   = 0;
        };
        ~TNestAddress() {};

        /**
         * @brief 拷贝构造函数与赋值构造函数
         */
        TNestAddress(const TNestAddress& addr) {
            this->_type       = addr._type;
            this->_addr_len   = addr._addr_len;
            memcpy(&this->_address, &addr._address, sizeof(_address));
        };
        TNestAddress& operator=(const TNestAddress& addr) {
            this->_type       = addr._type;
            this->_addr_len   = addr._addr_len;
            memcpy(&this->_address, &addr._address, sizeof(_address));
            return *this;
        };

        /**
         * @brief 比较函数
         */
        bool operator<(const TNestAddress& addr) {
            if (this->_type < addr._type) {
                return true;
            }
            if (this->_addr_len < addr._addr_len) {
                return true;
            }
            if (memcmp(&this->_address, &addr._address, this->_addr_len) < 0) {
                return true;
            }
            return false;          
        };

        /**
         * @brief 比较函数
         */
        bool operator==(const TNestAddress& addr) {
            if (this->_type < addr._type) {
                return false;
            }
            if (this->_addr_len < addr._addr_len) {
                return false;
            }
            if (memcmp(&this->_address, &addr._address, this->_addr_len) != 0) {
                return false;
            }
            return true;          
        };
        

        /**
         * @brief 地址设置函数, 重载支持各类地址IPV4 IPV6 UNIX等
         */
        void SetAddress(struct sockaddr_in* addr) {
            _type       = NEST_ADDR_IPV4;
            _addr_len   = sizeof(struct sockaddr_in);
            memcpy(&_address, addr, sizeof(*addr));
        };
        
        void SetAddress(struct sockaddr_un* addr, bool abstract = false) {
            _type       = NEST_ADDR_UNIX;
            _addr_len   = SUN_LEN(addr);
            memcpy(&_address, addr, sizeof(*addr));
            if (abstract) {
                _address.unix_addr.sun_path[0] = '\0';
            }
        };

        /**
         * @brief 设置socket地址
         */
        void SetSockAddress(TNestAddrType type, struct sockaddr* addr, socklen_t len) {
            memset(&_address, 0, sizeof(_address));
            _type       = type;
            _addr_len   = (len > sizeof(_address)) ?  sizeof(_address) : len;
            memcpy(&_address, addr, _addr_len);
        };

        /**
         * @brief 地址格式化文本串, 传入buff可保证重入, 不传则使用static信息
         */
        char* ToString(char* buff, uint32_t len) {
            static char buf[256];
            
            char* addr_str = buf;
            uint32_t buff_len = sizeof(buf);
            if (buff) {
                addr_str = buff;
                buff_len = len;
            }
            
            if (_type == NEST_ADDR_IPV4) {
                snprintf(addr_str, buff_len, "%s:%d", inet_ntoa(_address.ipv4_addr.sin_addr),
                        ntohs(_address.ipv4_addr.sin_port));
            } else if (_type == NEST_ADDR_UNIX) {
                if (_address.unix_addr.sun_path[0] != '\0') {
                    snprintf(addr_str, buff_len, "unix: %s", _address.unix_addr.sun_path);
                } else {
                    snprintf(addr_str, buff_len, "unix: @%s", _address.unix_addr.sun_path + 1);
                }
            } else {
                snprintf(addr_str, buff_len, "unknown address");
            }

            return addr_str;
        };

        /**
         * @brief 仅获取IP的字符串
         */
        char* IPString(char* buff, uint32_t len) {
            static char buf[256];
            
            char* addr_str = buf;
            uint32_t buff_len = sizeof(buf);
            if (buff) {
                addr_str = buff;
                buff_len = len;
            }
            
            if (_type == NEST_ADDR_IPV4) {
                snprintf(addr_str, buff_len, "%s", inet_ntoa(_address.ipv4_addr.sin_addr));
            } else if (_type == NEST_ADDR_UNIX) {
                if (_address.unix_addr.sun_path[0] != '\0') {
                    snprintf(addr_str, buff_len, "unix: %s", _address.unix_addr.sun_path);
                } else {
                    snprintf(addr_str, buff_len, "unix: @%s", _address.unix_addr.sun_path + 1);
                }
            } else {
                snprintf(addr_str, buff_len, "unknown address");
            }
            
            return addr_str;
        };

        /**
         * @brief 适配LINUX的socket系列接口函数
         */
        socklen_t GetAddressLen() {
            return _addr_len;
        };
        struct sockaddr* GetAddress() {
            return (struct sockaddr*)&_address;
        };

        /**
         * @brief 管理接口,获取类型与域信息
         */
        TNestAddrType GetAddressType() {
            return _type;
        };

        int32_t GetAddressDomain() {
            if (_type == NEST_ADDR_UNIX) {
                return AF_UNIX;
            } else {
                return AF_INET;
            }
        };
        
    private:

        TNestAddrType        _type;             // 地址类型
        socklen_t            _addr_len;         // 地址长度
        TNestAddrBuff        _address;          // 地址空间
    };

    typedef std::vector<TNestAddress>  TNestAddrArray;

    
}

#endif

