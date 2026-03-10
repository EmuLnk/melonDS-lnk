#ifndef EMULNKSERVER_H
#define EMULNKSERVER_H

#include <pthread.h>
#include <atomic>
#include "NDS.h"

class EmuLnkServer {
public:
    EmuLnkServer();
    ~EmuLnkServer();

    bool start(int port, melonDS::NDS* nds);
    void stop();

private:
    static void* listenerThread(void* arg);
    void handlePacket(const char* buf, int len, struct sockaddr_in* clientAddr);

    int sockFd;
    int port;
    pthread_t thread;
    std::atomic_bool running;
    melonDS::NDS* nds;
};

#endif // EMULNKSERVER_H
