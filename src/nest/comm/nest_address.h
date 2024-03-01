/**
 * @brief nest address 
 * @info  ���ε�ַ����, ����unixlocal��ַ��ipv4��ַ, ����չIPv6��ַ
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
    ///< ��ַ��ʵ���Ͷ���, ����չ����, ֧��ipv6��
    typedef enum __addr_type
    {
        NEST_ADDR_UNDEF = 0,
        NEST_ADDR_IPV4  = 1,
        NEST_ADDR_UNIX  = 2,
        NEST_ADDR_IPV6  = 3,
    }TNestAddrType;

    ///< ��ַʵ�ʵ�buff����, ����չ
    typedef union __addr_buff
    {
        struct sockaddr_in  ipv4_addr;
        struct sockaddr_un  unix_addr;
        struct sockaddr_in6 ipv6_addr;
    }TNestAddrBuff;


    /**
     * @brief ͨ�õ�ַ����, ����linux�ӿ�
     */
    class TNestAddress
    {
    public:

        /**
         * @brief ���캯������������
         */
        TNestAddress() {
            _type       = NEST_ADDR_UNDEF;
            _addr_len   = 0;
        };
        ~TNestAddress() {};

        /**
         * @brief �������캯���븳ֵ���캯��
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
         * @brief �ȽϺ���
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
         * @brief �ȽϺ���
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
         * @brief ��ַ���ú���, ����֧�ָ����ַIPV4 IPV6 UNIX��
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
         * @brief ����socket��ַ
         */
        void SetSockAddress(TNestAddrType type, struct sockaddr* addr, socklen_t len) {
            memset(&_address, 0, sizeof(_address));
            _type       = type;
            _addr_len   = (len > sizeof(_address)) ?  sizeof(_address) : len;
            memcpy(&_address, addr, _addr_len);
        };

        /**
         * @brief ��ַ��ʽ���ı���, ����buff�ɱ�֤����, ������ʹ��static��Ϣ
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
         * @brief ����ȡIP���ַ���
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
         * @brief ����LINUX��socketϵ�нӿں���
         */
        socklen_t GetAddressLen() {
            return _addr_len;
        };
        struct sockaddr* GetAddress() {
            return (struct sockaddr*)&_address;
        };

        /**
         * @brief ����ӿ�,��ȡ����������Ϣ
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

        TNestAddrType        _type;             // ��ַ����
        socklen_t            _addr_len;         // ��ַ����
        TNestAddrBuff        _address;          // ��ַ�ռ�
    };

    typedef std::vector<TNestAddress>  TNestAddrArray;

    
}

#endif

