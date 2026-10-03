#include <memory>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <vector>

#include <iostream>

#pragma comment(lib, "ws2_32.lib")

// https://www.rfc-editor.org/info/rfc1035/#section-3.2.2
enum Types {
    A = 1, // a host address

    NS = 2, // an authoritative name server

    MD = 3, // a mail destination (Obsolete - use MX)

    MF = 4, // a mail forwarder (Obsolete - use MX)

    CNAME = 5, // the canonical name for an alias

    SOA = 6, // marks the start of a zone of authority

    MB = 7, // a mailbox domain name (EXPERIMENTAL)

    MG = 8, // a mail group member (EXPERIMENTAL)

    MR = 9, // a mail rename domain name (EXPERIMENTAL)

    NLL = 10, // a null RR (EXPERIMENTAL)

    WKS = 11, // a well known service description

    PTR = 12, // a domain name pointer

    HINFO = 13, // host information

    MINFO = 14, // mailbox or mail list information

    MX = 15, // mail exchange

    TXT = 16 // text strings
};

enum QTypes & Types {
}

//
// 1  1  1  1  1  1
// 0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// |                      ID                       |
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// |QR|   Opcode  |AA|TC|RD|RA|   Z    |   RCODE   |
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// |                    QDCOUNT                    |
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// |                    ANCOUNT                    |
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// |                    NSCOUNT                    |
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// |                    ARCOUNT                    |
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
struct DNSHeader {
    uint16_t id;
    uint16_t flags;
    uint16_t qdcount;
    uint16_t ancount;
    uint16_t nscount;
    uint16_t arcount;
};

struct DNSQuestion {
};

class BufferReader {
public:
    BufferReader(std::vector<char> buffer) : _buffer(std::move(buffer)), _offset(0) {
    }

    template<typename T>
    T read() {
        const std::size_t size = sizeof(T);

        if (_offset + size > _buffer.size()) {
            throw std::runtime_error("Buffer overflow");
        }

        T val;
        std::memcpy(&val, _buffer.data() + _offset, size);
        _offset += size;

        return val;
    }

    std::string read_string(const int length) {
        if (_offset + length > _buffer.size()) {
            throw std::runtime_error("Buffer overflow");
        }

        std::string result(_buffer.data() + _offset, length);

        _offset += length;

        return result;
    }

    std::vector<char> _buffer;
    std::size_t _offset;
};

int main() {
    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return 1;
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (serverSocket == INVALID_SOCKET) {
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8053);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, reinterpret_cast<sockaddr *>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed. Error: " << WSAGetLastError() << "\n";
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    constexpr int BUFFER_SIZE = 1024;

    std::vector<char> buffer(BUFFER_SIZE);
    sockaddr_in clientAddr{};
    int clientAddrLen = sizeof(clientAddr);

    std::cout << "Listening" << std::endl;

    while (true) {
        std::fill(buffer.begin(), buffer.end(), 0);

        int bytesReceived = recvfrom(serverSocket, buffer.data(), BUFFER_SIZE - 1, 0,
                                     reinterpret_cast<sockaddr *>(&clientAddr), &clientAddrLen);

        if (bytesReceived == SOCKET_ERROR) {
            int errorCode = WSAGetLastError();

            if (errorCode == WSAEWOULDBLOCK) {
                Sleep(10);
                continue;
            }

            std::cerr << "Recvfrom failed. Error Code: " << errorCode << "\n";
            break;
        }

        char clientIp[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIp, INET6_ADDRSTRLEN);
        int clientPort = ntohs(clientAddr.sin_port);


        BufferReader reader(buffer);

        // std::vector<uint8_t> bytes = reader.read_bytes(12);
        // const uint16_t* raw = reinterpret_cast<const uint16_t*>(bytes.data());

        DNSHeader header{
            .id = ntohs(reader.read<uint16_t>()),
            .flags = ntohs(reader.read<uint16_t>()), // |QR|   Opcode  |AA|TC|RD|RA|   Z    |   RCODE   |
            .qdcount = ntohs(reader.read<uint16_t>()),
            .ancount = ntohs(reader.read<uint16_t>()),
            .nscount = ntohs(reader.read<uint16_t>()),
            .arcount = ntohs(reader.read<uint16_t>())
        };

        const uint8_t name_length = reader.read<uint8_t>();
        std::string domain_name = reader.read_string(name_length);

        const uint8_t top_leveL_domain_length = reader.read<uint8_t>();
        const std::string top_level_domain = reader.read_string(top_leveL_domain_length);

        const uint8_t nulL_terminator = reader.read<uint8_t>();

        const uint16_t q_type = ntohs(reader.read<uint16_t>());
        const uint16_t q_class = ntohs(reader.read<uint16_t>());

        std::cout << "Received " << bytesReceived << " bytes from " << clientIp << ":" << clientPort << std::endl;
        std::cout << "Data: " << buffer.data() << std::endl;
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
