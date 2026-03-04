#include "EmuLnkServer.h"

#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <android/log.h>

#include "NDS.h"
#include "NDS_Header.h"

#define LOG_TAG "EmuLnkServer"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#define EMULNK_MAX_READ 2048
#define EMULNK_VIRTUAL_SERIAL_ADDR 0x00200000

EmuLnkServer::EmuLnkServer() : sockFd(-1), port(55355), thread(0), running(false), nds(nullptr) {
}

EmuLnkServer::~EmuLnkServer() {
    stop();
}

void EmuLnkServer::start(int port, melonDS::NDS* nds) {
    if (running)
        return;

    this->port = port;
    this->nds = nds;

    sockFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockFd < 0) {
        LOGE("Failed to create UDP socket");
        return;
    }

    int reuse = 1;
    setsockopt(sockFd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(sockFd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        LOGE("Failed to bind UDP socket on port %d", port);
        close(sockFd);
        sockFd = -1;
        return;
    }

    running = true;
    pthread_create(&thread, nullptr, listenerThread, this);
    pthread_setname_np(thread, "EmuLnkServer");
    LOGI("Started on port %d", port);
}

void EmuLnkServer::stop() {
    if (!running)
        return;

    running = false;

    if (sockFd >= 0) {
        shutdown(sockFd, SHUT_RDWR);
        close(sockFd);
        sockFd = -1;
    }

    if (thread) {
        pthread_join(thread, nullptr);
        thread = 0;
    }

    nds = nullptr;
    LOGI("Stopped");
}

void* EmuLnkServer::listenerThread(void* arg) {
    EmuLnkServer* server = (EmuLnkServer*)arg;
    char buf[4096];

    while (server->running) {
        struct sockaddr_in clientAddr;
        socklen_t addrLen = sizeof(clientAddr);

        int received = recvfrom(server->sockFd, buf, sizeof(buf), 0,
                                (struct sockaddr*)&clientAddr, &addrLen);

        if (received <= 0) {
            if (!server->running) break;
            continue;
        }

        server->handlePacket(buf, received, &clientAddr);
    }

    return nullptr;
}

static bool isBinaryPacket(const char* buf, int len) {
    if (len < 8) return false;
    for (int i = 0; i < 4; i++) {
        unsigned char c = (unsigned char)buf[i];
        if (c < 0x20 || c > 0x7E) return true;
    }
    return false;
}

void EmuLnkServer::handlePacket(const char* buf, int len, struct sockaddr_in* clientAddr) {
    // EMLK handshake: 4 bytes "EMLK"
    if (len == 4 && memcmp(buf, "EMLK", 4) == 0) {
        const char* response = "melonds";
        sendto(sockFd, response, strlen(response), 0,
               (struct sockaddr*)clientAddr, sizeof(*clientAddr));
        return;
    }

    // Binary protocol: at least 8 bytes with non-printable chars in address field
    if (len >= 8 && isBinaryPacket(buf, len)) {
        uint32_t address;
        uint32_t size;
        memcpy(&address, buf, 4);
        memcpy(&size, buf + 4, 4);

        if (len == 8) {
            // Memory read request
            if (size > EMULNK_MAX_READ) size = EMULNK_MAX_READ;

            // Virtual serial address: return game identification
            if (address == EMULNK_VIRTUAL_SERIAL_ADDR) {
                char serial[16];
                if (nds && nds->CartInserted()) {
                    auto* cart = nds->GetNDSCart();
                    snprintf(serial, sizeof(serial), "NDS:%.4s", cart->GetHeader().GameCode);
                } else {
                    snprintf(serial, sizeof(serial), "NDS:????");
                }
                int serialLen = strlen(serial);
                if ((uint32_t)serialLen > size) serialLen = size;
                sendto(sockFd, serial, serialLen, 0,
                       (struct sockaddr*)clientAddr, sizeof(*clientAddr));
                return;
            }

            // Normal memory read from MainRAM
            if (nds && nds->MainRAM != nullptr) {
                char response[EMULNK_MAX_READ];
                for (uint32_t i = 0; i < size; i++) {
                    response[i] = nds->MainRAM[(address + i) & nds->MainRAMMask];
                }
                sendto(sockFd, response, size, 0,
                       (struct sockaddr*)clientAddr, sizeof(*clientAddr));
            }
        } else if ((uint32_t)len >= 8 + size) {
            // Memory write request
            if (nds && nds->MainRAM != nullptr) {
                const char* data = buf + 8;
                for (uint32_t i = 0; i < size; i++) {
                    nds->MainRAM[(address + i) & nds->MainRAMMask] = data[i];
                }
            }
        }
    }
}
