//
// Created by Sean Coursey on 2/14/2021.
//

#ifndef BOOMZAP_BOOMZAPOBJECTS_H
#define BOOMZAP_BOOMZAPOBJECTS_H

#include "glfwShapeObjects.h"

//Helper Functions
int randPosOrNeg() {
    /* Pseudo-randomly generates 1 or -1 using rand() */
    int i = 0;
    float randFrac = rand() / (float) RAND_MAX;
    (randFrac >= 0.5 ? i = 1 : i = -1);
    return i;
}

//Defining Player Class
class Player {
public:
    //Public Fields//
    glfwCircle body;
    bool movingUp = false;
    bool movingDown = false;
    bool movingRight = false;
    bool movingLeft = false;
    bool zapping = false;
    bool booming = false;
    int lives = 3;
    int score = 0;

    //Public Methods//

    //Initializer
    Player() {
        body.radius = 0.1;
        body.color[0] = 0.1;
        body.color[1] = 0.2;
        body.color[3] = 0.3;
        body.vel[0] = 0;
        body.vel[1] = 0;
        srand(time(NULL)); // This is where rand is seeded
    }


    //Randomly Increment Color
    void updateColor(void) {
        /* randomly adds a frac (< 0.1) to each color value (rgb) and resets the
           color value to a random frac (<= 1) if it exceeds 1 */
        body.color[0] += rand() / (float) RAND_MAX / 10;
        if (body.color[0] > 1) { body.color[0] = rand() / (float) RAND_MAX; }
        body.color[1] += rand() / (float) RAND_MAX / 10;
        if (body.color[1] > 1) { body.color[1] = rand() / (float) RAND_MAX; }
        body.color[2] += rand() / (float) RAND_MAX / 10;
        if (body.color[2] > 1) { body.color[2] = rand() / (float) RAND_MAX; }
    }

    //Update Position
    void updatePos(float timeStep) {
        body.updatePos(timeStep); // Calls update pos from glfwShapeObjects

        /* These if statements make it so if you go off on one side of the screen you pop back in the on the other */
        if (body.pos[0] > 1 || body.pos[0] < -1) {
            body.pos[0] *= -1;
        }
        if (body.pos[1] > 1 || body.pos[1] < -1) {
            body.pos[1] *= -1;
        }
    }

    //Draw Player Body and Effects
    void draw(double cursorX, double cursorY, float ratio) {
        /* if the player is right clicking */
        if (booming) {
            /* draw a red circle radius 0.25 centered on the player */
            glfwCircle boomOuter(0.25, body.pos[0], body.pos[1], .6, .2, 0);
            boomOuter.draw(ratio);
            /* draw a yellow circle radius 0.175 centered on the player */
            glfwCircle boomInner(0.175, body.pos[0], body.pos[1], .5, .5, 0);
            boomInner.draw(ratio);
        }
        /* if the player is left clicking && not right clicking */
        else if (zapping && !booming) {
            /* draw a yellow line from the middle of the player to the cursor */
            glBegin(GL_LINES);
                glColor3f(.8, .8, 0);
                glVertex2f(body.pos[0] / ratio, body.pos[1]);
                glVertex2f(cursorX / ratio, cursorY);
            glEnd();
        }
        body.draw(ratio); // self explanatory

        /* if the window is not square */
        if (ratio != 1) {
            /* draw vertical white lines on either side of the square gamespace */
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
    //Private Fields//
    glfwCircle body;
    
public:
    //Public Fields//
    int health;

    //Public Methods//

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

        /* Call update position from glfwShapeObjects.h */
        body.updatePos(timeStep);

        /* Check horizontal position, flip sides if it's off-screen */
        if (body.pos[0] > 1 || body.pos[0] < -1) {
            body.pos[0] *= -1;
            /* Update position again so that the object doesn't get caught flipping back and forth */
            body.updatePos(timeStep);
        }
        /* Check vertical position, flip sides if off-screen */
        if (body.pos[1] > 1 || body.pos[1] < -1) {
            body.pos[1] *= -1;
            /* Update again to avoid back-and-forth flip */
            body.updatePos(timeStep);
        }
    }

    //Collission Detection and Handling
    void detectCollision(Player &bob, double cursorX, double cursorY, float ratio, float timeStep) {

        /* if enemy touching player */
        if (pow(bob.body.pos[0] - body.pos[0], 2) + pow(bob.body.pos[1] - body.pos[1], 2) <=
            pow((bob.body.radius + body.radius), 2)) {
            /* re-initialize enemy */
            reInnit(bob);
            /* remove a life from the player */
            bob.lives -= 1;

        /* if enemy within booming radius of player and player is booming */
        } else if (pow(bob.body.pos[0] - body.pos[0], 2) + pow(bob.body.pos[1] - body.pos[1], 2) <=
                   pow((0.25 + body.radius), 2) && bob.booming) {
            health = 1; // self explanatory
            /* set color to pink */
            body.color[0] = 1; 
            body.color[1] = 0.5;
            body.color[2] = 0.6;
            /* randomly modify the velocity a little */
            int posOrNeg = randPosOrNeg();
            body.vel[0] += posOrNeg * rand() / (float) RAND_MAX * 20 * timeStep;
            posOrNeg = randPosOrNeg();
            body.vel[1] += posOrNeg * rand() / (float) RAND_MAX * 20 * timeStep;

        /* if player is zapping, the cursor is on enemy, and enemy has been boomed */
        } else if (bob.zapping && pow(cursorX - body.pos[0], 2) + pow(cursorY - body.pos[1], 2) <=
                                  pow(body.radius, 2) && health == 1) {
            reInnit(bob); // self explanatory
        }
    }

    //Draw Enemy Body
    void draw(float ratio) {
        body.draw(ratio);
    }
};

//Defining Initializer
Enemy::Enemy(Player &bob) {
    reInnit(bob); // self explanatory
    bob.score -= 1;
}

#endif //BOOMZAP_BOOMZAPOBJECTS_H
