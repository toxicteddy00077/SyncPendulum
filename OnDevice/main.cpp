#include <GL/glut.h>
#include <vector>
#include <cmath>

// Physical constants
const double m1 = 1.0;    // mass of pendulum 
const double m2 = 1.0;   
const double l1 = 1.0;    // length of pendulum 
const double l2 = 1.0;    
const double g  = 9.81;   // gravitational acceleration
const double dt = 0.005;  // simulation time step

//State
struct Pendulum {
    double theta1, theta2;
    double omega1, omega2;
};

Pendulum pendulum = { M_PI , M_PI+M_PI/10, 0.0, 0.0 };

// Trail storage for the second mass
std::vector<std::pair<double, double>> trail;
const size_t MAX_TRAIL = 500;

void drawCircle(double x, double y, double R, int segments=64) {
    glBegin(GL_TRIANGLE_FAN);
    {    
        glVertex3f(x, y, 0.0f);
        for (int i = 0; i <= segments; ++i) {
            double ang = 2 * M_PI * i / segments;
            glVertex3f(x + cos(ang) * R, y + sin(ang) * R, 0.0f);
        }
    }
    glEnd();
}

// Euler Integration 
void updatePendulum() {
    double delta = pendulum.theta2 - pendulum.theta1;
    double denom1 = (m1 + m2) * l1 - m2 * l1 * cos(delta) * cos(delta);
    double denom2 = (l2 / l1) * denom1;

    // angular accelerations
    double a1 = ( m2 * l1 * pendulum.omega1 * pendulum.omega1 * sin(delta) * cos(delta)
                + m2 * g * sin(pendulum.theta2) * cos(delta)
                + m2 * l2 * pendulum.omega2 * pendulum.omega2 * sin(delta)
                - (m1 + m2) * g * sin(pendulum.theta1) ) / denom1;

    double a2 = ( -m2 * l2 * pendulum.omega2 * pendulum.omega2 * sin(delta) * cos(delta)
                + (m1 + m2) * g * sin(pendulum.theta1) * cos(delta)
                - (m1 + m2) * l1 * pendulum.omega1 * pendulum.omega1 * sin(delta)
                - (m1 + m2) * g * sin(pendulum.theta2) ) / denom2;

    // piecewise integration
    pendulum.omega1 += a1 * dt;
    pendulum.omega2 += a2 * dt;
    pendulum.theta1 += pendulum.omega1 * dt;
    pendulum.theta2 += pendulum.omega2 * dt;

    // record second mass position for trail
    double x2 = l1 * sin(pendulum.theta1) + l2 * sin(pendulum.theta2);
    double y2 = -(l1 * cos(pendulum.theta1) + l2 * cos(pendulum.theta2));
    trail.emplace_back(x2, y2);
    if (trail.size() > MAX_TRAIL) {
        trail.erase(trail.begin());
    }
}

// Draw the double pendulum rods and masses (as circles)
void drawPendulum() {
    double x1 = l1 * sin(pendulum.theta1);
    double y1 = -l1 * cos(pendulum.theta1);
    double x2 = x1 + l2 * sin(pendulum.theta2);
    double y2 = y1 - l2 * cos(pendulum.theta2);

    // Draw rods
    glColor3f(1.0f, 0.0f, 0.0f);
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    {
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(x1, y1, 0.0f);
        glVertex3f(x1, y1, 0.0f);
        glVertex3f(x2, y2, 0.0f);
    }
    glEnd();

    // Draw masses as filled circles
    double radius = 0.05;
    glColor3f(0.0f, 0.0f, 1.0f);
    drawCircle(x1, y1, radius);
    drawCircle(x2, y2, radius);
}

// Draw the trail of the second mass
void drawTrail() {
    glColor3f(0.0f, 1.0f, 0.0f);
    glBegin(GL_LINE_STRIP);
    for (auto &p : trail) {
        glVertex3f(p.first, p.second, 0.0f);
    }
    glEnd();
}

// Display callback
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, -5.0f);

    updatePendulum();      
    drawPendulum();         
    drawTrail();          

    glutSwapBuffers();      // swap buffers to display
}

void idle() {
    glutPostRedisplay();    
}

// Window reshape callback
void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (double)w / (double)h, 1.0, 10.0);
    glMatrixMode(GL_MODELVIEW);
}

// Initialization
void init() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
}

// Entry point
typedef void (*funcptr)();
int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1000, 800);
    glutCreateWindow("Double Pendulum - GLUT");

    init();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutIdleFunc(idle);   

    glutMainLoop();
    return 0;
}
