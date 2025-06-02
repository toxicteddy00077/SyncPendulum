// File: double_pendulum_glut_network_compressed_ring.cpp

#include <GL/glut.h>
#include <deque>
#include <utility>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <zlib.h>

const double m1 = 1.0, m2 = 1.0, l1 = 1.0, l2 = 1.0, g = 9.81;

// Network
#define SERVER_IP   "192.168.1.225"
#define SERVER_PORT 5555
int sockfd;
struct sockaddr_in server_addr;

typedef struct {
    float theta1, omega1, theta2, omega2;
} PendulumF;

PendulumF pendulum = { M_PI, 0.0f, M_PI + 1, 0.0f };

// Ring buffer for the trail(green line)
std::deque<std::pair<double,double>> trail;
const size_t MAX_TRAIL = 100000;

// Draw a filled circle
void drawCircle(double x, double y, double R, int segs = 64) {
    glBegin(GL_TRIANGLE_FAN);
      glVertex3f(x, y, 0);
      for (int i = 0; i <= segs; ++i) {
        double ang = 2*M_PI*i/segs;
        glVertex3f(x + cos(ang)*R, y + sin(ang)*R, 0);
      }
    glEnd();
}

// Initialize UDP socket (non-blocking)
void init_network() {
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) { perror("socket"); exit(1); }
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);
}

// Exchange state (with compresion)
void network_update() {
    // Compress outgoing state
    PendulumF sendState = pendulum;
    uLongf compSize = compressBound(sizeof(sendState));
    Bytef compBuf[128];
    if (compress(compBuf, &compSize,
                 reinterpret_cast<Bytef*>(&sendState),
                 sizeof(sendState)) != Z_OK) {
        fprintf(stderr, "Compression failed\n");
        return;
    }

    uint16_t netSize = htons(compSize);
    sendto(sockfd, &netSize, sizeof(netSize), 0,
           (sockaddr*)&server_addr, sizeof(server_addr));
    sendto(sockfd, compBuf, compSize, 0,
           (sockaddr*)&server_addr, sizeof(server_addr));

    uint16_t respSizeNet;
    if (recvfrom(sockfd, &respSizeNet, sizeof(respSizeNet),
                 0, nullptr, nullptr) != sizeof(respSizeNet)) {
        return;
    }
    uLongf respSize = ntohs(respSizeNet);
    if (respSize > sizeof(compBuf)) return;

    if (recvfrom(sockfd, compBuf, respSize, 0, nullptr, nullptr)
        != (ssize_t)respSize) {
        return;
    }
    // Decompression
    PendulumF recvState;
    uLongf decompSize = sizeof(recvState);
    if (uncompress(reinterpret_cast<Bytef*>(&recvState), &decompSize,
                   compBuf, respSize) != Z_OK) {
        fprintf(stderr, "Decompression failed\n");
        return;
    }
    pendulum = recvState;

    double x2 = l1 * sin(pendulum.theta1)
              + l2 * sin(pendulum.theta2);
    double y2 = - (l1 * cos(pendulum.theta1)
                +  l2 * cos(pendulum.theta2));
    trail.emplace_back(x2, y2);
    if (trail.size() > MAX_TRAIL) {
        trail.pop_front();  // constant time :contentReference[oaicite:3]{index=3}
    }
}

// Rendering
void drawPendulum() {
    double x1 = l1 * sin(pendulum.theta1);
    double y1 = -l1 * cos(pendulum.theta1);
    double x2 = x1 + l2 * sin(pendulum.theta2);
    double y2 = y1 - l2 * cos(pendulum.theta2);

    glColor3f(1,0,0);
    glLineWidth(2);
    glBegin(GL_LINES);
      glVertex3f(0,0,0);      glVertex3f(x1,y1,0);
      glVertex3f(x1,y1,0);    glVertex3f(x2,y2,0);
    glEnd();

    glColor3f(0,0,1);
    drawCircle(x1,y1,0.05);
    drawCircle(x2,y2,0.05);
}

void drawTrail() {
    glColor3f(0,1,0);
    glBegin(GL_LINE_STRIP);
      for (auto &p : trail) {
        glVertex3f(p.first, p.second, 0);
      }
    glEnd();
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    glTranslatef(0,0,-7);

    network_update();  
    drawTrail();
    drawPendulum();
    glutSwapBuffers();
}

void idle()  { glutPostRedisplay(); }
void reshape(int w,int h) {
    glViewport(0,0,w,h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (double)w/h, 1.0, 10.0);
    glMatrixMode(GL_MODELVIEW);
}
void init_gl() {
    glClearColor(0,0,0,1);
    glEnable(GL_DEPTH_TEST);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB|GLUT_DEPTH);
    glutInitWindowSize(1000,800);
    glutCreateWindow("Double Pendulum Ring Buffer");

    init_gl();
    init_network();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutIdleFunc(idle);
    glutMainLoop();
    return 0;
}
