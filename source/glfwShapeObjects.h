//
// Created by Sean Coursey on 2/14/2021.
//

#ifndef GLFWSHAPEOBJECTS_H
#define GLFWSHAPEOBJECTS_H

// CIRCLE //////////////////////////////////////////////////////////////////////////////////////////////////////////////
class glfwCircle {
private:
    const float DEG2RAD = 3.14159 / 180;

public:
    float radius;
    float pos[2];
    float color[3];
    float vel[2];

    glfwCircle(){
        radius = .1;
        pos[0] = 0;
        pos[1] = 0;
        vel[0] = 0;
        vel[1] = 0;
        color[0] = 1;
        color[1] = 1;
        color[2] = 1;
    }

    explicit glfwCircle(float rad, float x, float y, float r = 1, float g = 1, float b = 1, float vx = 0, float vy = 0) {
        radius = rad;
        pos[0] = x;
        pos[1] = y;
        vel[0] = vx;
        vel[1] = vy;
        color[0] = r;
        color[1] = g;
        color[2] = b;
    }

    void draw(float ratio) {
        glColor3f(color[0], color[1], color[2]);
        glBegin(GL_POLYGON);
        for (int i = 0; i < 360; i++) {
            float degInRad = i*DEG2RAD;
            glVertex2f((cos(degInRad)*radius + pos[0]) / ratio, sin(degInRad)*radius + pos[1]);
        }
        glEnd();
    }

    void updatePos(float timeStep) {
        pos[0] += vel[0] * timeStep;
        pos[1] += vel[1] * timeStep;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#endif //GLFWSHAPEOBJECTS_H
