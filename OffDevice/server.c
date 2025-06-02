
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <zlib.h>

#define PORT 5555
#define DT   0.005f

const float m1 = 1.0f, m2 = 1.0f;
const float l1 = 1.0f, l2 = 1.0f;
const float g  = 9.81f;

typedef struct {
    float theta1, omega1, theta2, omega2;
} PendulumF;

void integrate(PendulumF *s) {
    float delta = s->theta2 - s->theta1;
    float denom1 = (m1 + m2) * l1 - m2 * l1 * cosf(delta) * cosf(delta);
    float denom2 = (l2 / l1) * denom1;

    float a1 = ( m2 * l1 * s->omega1 * s->omega1 * sinf(delta) * cosf(delta)
               + m2 * g * sinf(s->theta2) * cosf(delta)
               + m2 * l2 * s->omega2 * s->omega2 * sinf(delta)
               - (m1 + m2) * g * sinf(s->theta1) ) / denom1;

    float a2 = ( -m2 * l2 * s->omega2 * s->omega2 * sinf(delta) * cosf(delta)
               + (m1 + m2) * g * sinf(s->theta1) * cosf(delta)
               - (m1 + m2) * l1 * s->omega1 * s->omega1 * sinf(delta)
               - (m1 + m2) * g * sinf(s->theta2) ) / denom2;

    s->omega1  += a1 * DT;
    s->omega2  += a2 * DT;
    s->theta1  += s->omega1 * DT;
    s->theta2  += s->omega2 * DT;
}

int main() {
    int sock;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t cli_len = sizeof(cli_addr);

    Bytef compBuf[128];
    Bytef decompBuf[128];

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket"); exit(1);
    }
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family      = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port        = htons(PORT);

    if (bind(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("bind"); exit(1);
    }

    printf("Compressed physics server listening on UDP port %d\n", PORT);

    while (1) {
        uint16_t netSize;
        ssize_t n = recvfrom(sock, &netSize, sizeof(netSize), 0,
                             (struct sockaddr*)&cli_addr, &cli_len);
        if (n != sizeof(netSize)) continue;
        uLongf compSize = ntohs(netSize);
        if (compSize > sizeof(compBuf)) continue;

        n = recvfrom(sock, compBuf, compSize, 0,
                     (struct sockaddr*)&cli_addr, &cli_len);
        if (n != (ssize_t)compSize) continue;

        PendulumF state;
        uLongf decompSize = sizeof(state);
        if (uncompress((Bytef*)&state, &decompSize, compBuf, compSize) != Z_OK) {
            fprintf(stderr, "Decompression failed\n");
            continue;
        }

        integrate(&state);

        uLongf outSize = compressBound(sizeof(state));
        if (compress(compBuf, &outSize, (Bytef*)&state, sizeof(state)) != Z_OK) {
            fprintf(stderr, "Compression failed\n");
            continue;
        }

        uint16_t outNet = htons(outSize);
        sendto(sock, &outNet, sizeof(outNet), 0,
               (struct sockaddr*)&cli_addr, cli_len);
        sendto(sock, compBuf, outSize, 0,
               (struct sockaddr*)&cli_addr, cli_len);
    }

    close(sock);
    return 0;
}