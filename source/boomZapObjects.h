//
// Created by Sean Coursey on 2/14/2021.
//

#ifndef BOOMZAP_BOOMZAPOBJECTS_H
#define BOOMZAP_BOOMZAPOBJECTS_H

#include "glfwShapeObjects.h"

//Helper Functions
int randPosOrNeg() {
    int i = 0;
    float randFrac = rand() / (float) RAND_MAX;
    (randFrac >= 0.5 ? i = 1 : i = -1);
    return i;
}

//Defining Player Class
class Player {
public:
    glfwCircle body;
    bool movingUp = false;
    bool movingDown = false;
    bool movingRight = false;
    bool movingLeft = false;
    bool zapping = false;
    bool booming = false;
    int lives = 3;
    int score = 0;

    Player() {
        body.radius = 0.1;
        body.color[0] = 0.1;
        body.color[1] = 0.2;
        body.color[3] = 0.3;
        body.vel[0] = 0;
        body.vel[1] = 0;
        srand(time(NULL));
    }

    void updateColor(void) {
        body.color[0] += rand() / (float) RAND_MAX / 10;
        if (body.color[0] > 1) { body.color[0] = rand() / (float) RAND_MAX; }
        body.color[1] += rand() / (float) RAND_MAX / 10;
        if (body.color[1] > 1) { body.color[1] = rand() / (float) RAND_MAX; }
        body.color[2] += rand() / (float) RAND_MAX / 10;
        if (body.color[2] > 1) { body.color[2] = rand() / (float) RAND_MAX; }
    }

    void updatePos(float timeStep) {
        body.updatePos(timeStep);
        if (body.pos[0] > 1 || body.pos[0] < -1) {
            body.pos[0] *= -1;
        }
        if (body.pos[1] > 1 || body.pos[1] < -1) {
            body.pos[1] *= -1;
        }
    }

    void draw(double cursorX, double cursorY, float ratio) {
        if (zapping && !booming) {
            glBegin(GL_LINES);
            glColor3f(.8, .8, 0);
            glVertex2f(body.pos[0] / ratio, body.pos[1]);
            glVertex2f(cursorX / ratio, cursorY);
            glEnd();
        } else if (booming) {
            glfwCircle boomOuter(0.25, body.pos[0], body.pos[1], .6, .2, 0);
            boomOuter.draw(ratio);
            glfwCircle boomInner(0.175, body.pos[0], body.pos[1], .5, .5, 0);
            boomInner.draw(ratio);
        }
        body.draw(ratio);

        if (ratio != 1) {
            glBegin(GL_LINES);
            glColor3f(1,1,1);
            glVertex2f(1/ratio, 1);
            glVertex2f(1/ratio, -1);
            glEnd();
            glBegin(GL_LINES);
            glVertex2f(-1/ratio, 1);
            glVertex2f(-1/ratio, -1);
            glEnd();
        }
    }
};

//Defining Enemy Class
class Enemy {
private:
    glfwCircle body;
    
public:
    int health;

    //Prototyping Initializer
    Enemy(Player &bob);

    //Re-initialize the enemy after being destroyed
    void reInnit(Player &bob) {

        /* Make sure the enemy doesn't spawn too close to the player */
        do {
            body.radius = .08 + (rand() / (float) RAND_MAX / 30);
            body.pos[0] = rand() / (float) RAND_MAX * 2 - 1;
            body.pos[1] = rand() / (float) RAND_MAX * 2 - 1;
        } while (pow(bob.body.pos[0] - body.pos[0], 2) + pow(bob.body.pos[1] - body.pos[1], 2) <=
                 pow((bob.body.radius + body.radius) * 3, 2));

        /* Initialize color to grey */
        body.color[0] = 0.4;
        body.color[1] = 0.4;
        body.color[2] = 0.4;

        /* Intialize velocity to a random velocity, min = 0.21, max = 0.91 */
        int posOrNeg = randPosOrNeg();
        body.vel[0] = (posOrNeg * (0.3 + rand() / (float) RAND_MAX))*0.7;
        posOrNeg = randPosOrNeg();
        body.vel[1] = (posOrNeg * (0.3 + rand() / (float) RAND_MAX))*0.7;

        /* Reset health */
        health = 2;

        /* Add to player score */
        bob.score += 1;
    }

    //Update Position
    void updatePos(float timeStep) {
        body.updatePos(timeStep);
        if (body.pos[0] > 1 || body.pos[0] < -1) {
            body.pos[0] *= -1;
            body.updatePos(timeStep);
        }
        if (body.pos[1] > 1 || body.pos[1] < -1) {
            body.pos[1] *= -1;
            body.updatePos(timeStep);
        }
    }

    void detectCollision(Player &bob, double cursorX, double cursorY, float ratio, float timeStep) {
        if (pow(bob.body.pos[0] - body.pos[0], 2) + pow(bob.body.pos[1] - body.pos[1], 2) <=
            pow((bob.body.radius + body.radius), 2)) {
            reInnit(bob);
            bob.lives -= 1;
        } else if (pow(bob.body.pos[0] - body.pos[0], 2) + pow(bob.body.pos[1] - body.pos[1], 2) <=
                   pow((0.25 + body.radius), 2) && bob.booming) {
            health = 1;
            body.color[0] = 1;
            body.color[1] = 0.5;
            body.color[2] = 0.6;
            int posOrNeg = randPosOrNeg();
            body.vel[0] += posOrNeg * rand() / (float) RAND_MAX * 20 * timeStep;
            posOrNeg = randPosOrNeg();
            body.vel[1] += posOrNeg * rand() / (float) RAND_MAX * 20 * timeStep;
        } else if (bob.zapping && pow(cursorX - body.pos[0], 2) + pow(cursorY - body.pos[1], 2) <=
                                  pow(body.radius, 2) && health == 1) {
            reInnit(bob);
        }
    }

    void draw(float ratio) {
        body.draw(ratio);
    }
};

Enemy::Enemy(Player &bob) { //Defining Initializer
    reInnit(bob);
    bob.score -= 1;
}

#endif //BOOMZAP_BOOMZAPOBJECTS_H
