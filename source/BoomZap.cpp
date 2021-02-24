#include "shader.h"
#include <GLFW/glfw3.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <math.h>
#include <stdlib.h>
#include <cstring>
#include <ctime>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <map>
#include <math.h>
#include <vector>

#include "boomZapObjects.h"

//Defining
#define MAIN_MENU 0
#define GAME_PLAYING 1
#define GAME_OVER 2

//Prototyping
void mouse_button_callback(GLFWwindow *window, int button, int action, int mods);
static void cursor_position_callback(GLFWwindow *window, double xPos, double yPos);
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

void get_resolution(int &windowwidth, int &windowheight);
void installShaders();
void RenderText(Shader &s, std::string text, float x, float y, float scale, glm::vec3 color);

//Initializing
const float PLAYER_SPEED = 0.7;
int WINDOW_HEIGHT;
int WINDOW_WIDTH;
double dt = 0;
double t1 = 0;
double t2 = 0;
unsigned int VAO, VBO;
unsigned short int gameState = MAIN_MENU;

//Character struct
struct Character {
    unsigned int    TextureID;  // ID handle of the glyph texture
    glm::ivec2      Size;       // Size of glyph
    glm::ivec2      Bearing;    // Offset from baseline to left/top of glyph
    unsigned int    Advance;    // Offset to advance to next glyph
};
std::map<char, Character> Characters;

//Initializing Game Objects
Player player;
std::vector<Enemy> enemies;

int main() {
    //Seeding
    srand(time(NULL));

    //Initial Window Setup GLFW
    if (!glfwInit()) {
        exit(EXIT_FAILURE);
    }

    get_resolution(WINDOW_WIDTH, WINDOW_HEIGHT);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    GLFWwindow *window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "BoomZap", NULL, NULL);
    if (!window) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glfwMakeContextCurrent(window);

    glfwSwapInterval(1);

    //Load Glad
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){
        std::cout << "failed to initialize Glad" << std::endl;
    }

    //OpenGL state
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    //Install Shaders
    Shader shader("text.vs", "text.fs");
    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(WINDOW_WIDTH), 0.0f, static_cast<float>(WINDOW_HEIGHT));
    shader.use();
    glUniformMatrix4fv(glGetUniformLocation(shader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    //FreeType
    FT_Library library;
    FT_Face face;
    int error_check = FT_Init_FreeType(&library);
    if (error_check != FT_Err_Ok) {
        std::cout << "An error occured during freetype library initialization." << std::endl;
    }
    error_check = FT_New_Face (library, "Poppins-Regular.ttf", 0, &face);
    if (error_check != FT_Err_Ok) {
        std::cout << "An error occured during freetype face initialization." << std::endl;
        if (error_check == FT_Err_Unknown_File_Format) {
            std::cout << "The font file could be read but has an unsupported format." << std::endl;
        }
    }
    error_check = FT_Set_Pixel_Sizes(face, 0, 48);
    if (error_check != FT_Err_Ok) {
        std::cout << "An error occured while setting the pixel size." << std::endl;
    };
    // disable byte-alignment restriction
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);


    //Populating Character Map
    for (unsigned char c= 0; c < 128; c++) {
        /* load character glyphs */
        error_check = FT_Load_Char(face, c, FT_LOAD_RENDER);
        if (error_check != FT_Err_Ok) {
            std::cout << "An error occured while setting the pixel size." << std::endl;
        };
        /* generate texture */
        unsigned int texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            face->glyph->bitmap.buffer
        );
        /* set texture options */
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        /* now store character for later use */
        Character character = {
            texture, 
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            face->glyph->advance.x
        };
        Characters.insert(std::pair<char, Character>(c, character));
    }
    FT_Done_Face(face);
    FT_Done_FreeType(library);

    //Config VBO and VAO for rendering quads
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    //Redefining Cursor
    unsigned char pixels[16*16*4];
    memset(pixels, 0xdd, sizeof(pixels));
    GLFWimage image;
    image.width = 16;
    image.height = 16;
    image.pixels = pixels;
    GLFWcursor *cursor = glfwCreateCursor(&image, 0, 0);
    glfwSetCursor(window, cursor);
    double xpos = 0, ypos = 0;
    
    //Setting Callbacks
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    //Initializing Objects
    for (int i = 0; i < 3; i++) {
        Enemy enemy(player);
        enemies.push_back(enemy);
    }
    glfwCircle lifeCircle1(0.02, -.9, -.9, 0, 1, 0, 0, 0);
    glfwCircle lifeCircle2(0.02, -.84, -.9, 0, 1, 0, 0, 0);
    glfwCircle lifeCircle3(0.02, -.78, -.9, 0, 1, 0, 0, 0);

    //Run-Loop
    while (!glfwWindowShouldClose(window)) {
        //Keeping track of time
        auto start = clock();

        //Setup View
        float ratio;
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        ratio = width / (float) height;
        glViewport(0, 0, width, height);
        //glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        //Handle User Input Main Menu
        glfwGetCursorPos(window, &xpos, &ypos);

        //Main Menu
        if (gameState == MAIN_MENU) {
            //For resetting
            player.score = 0;

            //Game name text
            std::string GameName = "BoomZap 0.5 Alpha";
            float scale = 3.0f * 1920 / WINDOW_WIDTH;
            float textPixelLength = 0;
            std::string::const_iterator c;
            for (c = GameName.begin(); c != GameName.end(); c++) {
                Character ch = Characters[*c];
                textPixelLength += (ch.Advance >> 6) * scale;
            }
            RenderText(shader, GameName, static_cast<float>(WINDOW_WIDTH) / 2 - textPixelLength / 2, static_cast<float>(WINDOW_HEIGHT) * 3/5, scale, glm::vec3(1.0f, 1.0f, 1.0f));
            glUseProgram(0);
            std::string playButton = "Click to play!";
            scale = 1.0f * 1920 / WINDOW_WIDTH;
            textPixelLength = 0;
            for (c = GameName.begin(); c != GameName.end(); c++) {
                Character ch = Characters[*c];
                textPixelLength += (ch.Advance >> 6) * scale;
            }
            RenderText(shader, playButton, static_cast<float>(WINDOW_WIDTH) / 2 - textPixelLength / 2, static_cast<float>(WINDOW_HEIGHT) * 2/5, scale, glm::vec3(1.0f, 1.0f, 1.0f));
            glUseProgram(0);
        }

        //Game playing
        if (gameState == GAME_PLAYING) {
            //Handle User Input Gameplay
            xpos = (xpos*2/width - 1) * ratio;
            ypos = -1*(ypos*2/height) + 1;
            
            if (t1 > 1/60) {
                //Update Velocities
                if (player.movingUp && !player.movingDown && !(player.movingLeft ^ player.movingRight)) {
                    player.body.vel[1] = PLAYER_SPEED;
                    player.body.vel[0] = 0;
                } else if (player.movingDown && !player.movingUp && !(player.movingLeft ^ player.movingRight)) {
                    player.body.vel[1] = -1*PLAYER_SPEED;
                    player.body.vel[0] = 0;
                } else if (player.movingLeft && !player.movingRight && !(player.movingUp ^ player.movingDown)) {
                    player.body.vel[0] = -1*PLAYER_SPEED;
                    player.body.vel[1] = 0;
                } else if (player.movingRight && !player.movingLeft && !(player.movingUp ^ player.movingDown)) {
                    player.body.vel[0] = PLAYER_SPEED;
                    player.body.vel[1] = 0;
                } else if (player.movingUp && player.movingRight && !(player.movingDown || player.movingLeft)) {
                    player.body.vel[0] = sqrt(pow(PLAYER_SPEED, 2) / 2);
                    player.body.vel[1] = sqrt(pow(PLAYER_SPEED, 2) / 2);
                } else if (player.movingUp && player.movingLeft && !(player.movingDown || player.movingRight)) {
                    player.body.vel[0] = -1 * sqrt(pow(PLAYER_SPEED, 2) / 2);
                    player.body.vel[1] = sqrt(pow(PLAYER_SPEED, 2) / 2);
                } else if (player.movingDown && player.movingRight && !(player.movingUp || player.movingLeft)) {
                    player.body.vel[0] = sqrt(pow(PLAYER_SPEED, 2) / 2);
                    player.body.vel[1] = -1 * sqrt(pow(PLAYER_SPEED, 2) / 2);
                } else if (player.movingDown && player.movingLeft && !(player.movingUp || player.movingRight)) {
                    player.body.vel[0] = -1*sqrt(pow(PLAYER_SPEED, 2) / 2);
                    player.body.vel[1] = -1*sqrt(pow(PLAYER_SPEED, 2) / 2);
                } else {
                    player.body.vel[0] = 0;
                    player.body.vel[1] = 0;
                }

                //Update Positions
                player.updatePos(dt);
                for (int i = 0; i < enemies.size(); i++) {
                    enemies[i].updatePos(dt);
                }

                //Collision Detection
                for (int i = 0; i < enemies.size(); i++) {
                    enemies[i].detectCollision(player, xpos, ypos, ratio, dt);
                }

                //Update Colors
                if (t2 > 1.0/30) {
                    player.updateColor();
                    t2 = 0;
                }

                //Draw
                player.draw(xpos, ypos, ratio);
                for (int i = 0; i < enemies.size(); i++) {
                    enemies[i].draw(ratio);
                }
                if (player.lives >= 1) {
                    lifeCircle1.draw(ratio);
                }
                if (player.lives >= 2) {
                    lifeCircle2.draw(ratio);
                }
                if (player.lives == 3) {
                    lifeCircle3.draw(ratio);
                }
                if (player.lives <= 0) {
                    gameState = GAME_OVER;
                }
                //Score Counter
                std::string scoreStr = std::to_string(player.score);
                float scale = 2.0f * 1920 / WINDOW_WIDTH;
                float textPixelLength = 0;
                std::string::const_iterator c;
                for (c = scoreStr.begin(); c != scoreStr.end(); c++) {
                    Character ch = Characters[*c];
                    textPixelLength += (ch.Advance >> 6) * scale;
                }
                RenderText(shader, scoreStr, static_cast<float>(WINDOW_WIDTH) - textPixelLength - 10 * 1920 / WINDOW_WIDTH, 10 * 1920 / WINDOW_WIDTH, scale, glm::vec3(1.0f, 1.0f, 1.0f));
                glUseProgram(0);

                //Create more Enemies
                if (enemies.size() < 3 + player.score / 10) {
                    Enemy enemy(player);
                    enemies.push_back(enemy);
                }
                t1 = 0;
            }
        }

        if (gameState == GAME_OVER){
            player.lives = 3;
            player.body.pos[0] = 0;
            player.body.pos[1] = 0;
            player.movingUp = false;
            player.movingDown = false;
            player.movingLeft = false;
            player.movingRight = false;
            player.booming = false;
            player.zapping = false;
            for (int i = 0; i < enemies.size() - 3; i++){
                enemies.pop_back();
            }
            for (int i = 0; i < 3; i++){
                enemies[i].reInnit(player);
                player.score -= 1;
            }
            std::string scoreStr = "You scored " + std::to_string(player.score) + " points!";
            float scale = 2.0f * 1920 / WINDOW_WIDTH;
            float textPixelLength = 0;
            std::string::const_iterator c;
            for (c = scoreStr.begin(); c != scoreStr.end(); c++) {
                Character ch = Characters[*c];
                textPixelLength += (ch.Advance >> 6) * scale;
            }
            RenderText(shader, scoreStr, static_cast<float>(WINDOW_WIDTH) / 2 - textPixelLength / 2, static_cast<float>(WINDOW_HEIGHT) / 2, scale, glm::vec3(1.0f, 1.0f, 1.0f));
            glUseProgram(0);
            
            std::string spaceToContinueStr = "Press [SPACE] to return to main menu.";
            scale = 1.0f * 1920 / WINDOW_WIDTH;
            textPixelLength = 0;
            for (c = spaceToContinueStr.begin(); c != spaceToContinueStr.end(); c++) {
                Character ch = Characters[*c];
                textPixelLength += (ch.Advance >> 6) * scale;
            }
            RenderText(shader, spaceToContinueStr, static_cast<float>(WINDOW_WIDTH) / 2 - textPixelLength / 2, static_cast<float>(WINDOW_HEIGHT) / 3, scale, glm::vec3(1.0f, 1.0f, 1.0f));
            glUseProgram(0);
        }

        //Swap Buffer and Poll Events
        glfwSwapBuffers(window);
        glfwPollEvents();

        //Keeping track of time
        auto end = clock();
        dt = difftime(end, start) / CLOCKS_PER_SEC;
        t1 += dt;
        t2 += dt;
    }
    
    //Close Window
    glfwTerminate();
    exit(EXIT_SUCCESS);
}

//Defining
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods){
    if (gameState == GAME_PLAYING){
        switch (key) {
            case GLFW_KEY_W:
                if (action == GLFW_REPEAT || action == GLFW_PRESS) {
                    player.movingUp = true;
                } else {
                    player.movingUp = false;
                }
                break;
            case GLFW_KEY_S:
                if (action == GLFW_PRESS || action == GLFW_REPEAT){
                    player.movingDown = true;
                } else {
                    player.movingDown = false;
                }
                break;
            case GLFW_KEY_A:
                if (action == GLFW_PRESS || action == GLFW_REPEAT){
                    player.movingLeft = true;
                } else {
                    player.movingLeft = false;
                }
                break;
            case GLFW_KEY_D:
                if (action == GLFW_PRESS || action == GLFW_REPEAT){
                    player.movingRight = true;
                } else {
                    player.movingRight = false;
                }
                break;
            default:
                break;
        }
    }
    if (gameState == GAME_OVER) {
        if (key == GLFW_KEY_SPACE) {
            gameState = MAIN_MENU;
        }
    }
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods){
    if (gameState == MAIN_MENU) {
        if (action == GLFW_PRESS) {
            gameState = GAME_PLAYING;
        }
    }
    if (gameState == GAME_PLAYING){
        switch (button) {
            case GLFW_MOUSE_BUTTON_LEFT:
                if (action == GLFW_PRESS || action == GLFW_REPEAT) {
                    player.zapping = true;
                } else  {
                    player.zapping = false;
                }
                break;
            case GLFW_MOUSE_BUTTON_RIGHT:
                if (action == GLFW_PRESS || action == GLFW_REPEAT) {
                    player.booming = true;
                } else {
                    player.booming = false;
                }
                break;
            default:
                break;
        }
    }
}

void get_resolution(int &windowwidth, int &windowheight) {
    const GLFWvidmode * mode = glfwGetVideoMode(glfwGetPrimaryMonitor());

    windowwidth = (mode->height) - (mode->height)/10;
    windowheight = (mode->height) - (mode->height)/10;
}

void RenderText(Shader &shader, std::string text, float x, float y, float scale, glm::vec3 color)
{
    // activate corresponding render state	
    shader.use();
    glUniform3f(glGetUniformLocation(shader.ID, "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);

    // iterate through all characters
    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++) 
    {
        Character ch = Characters[*c];

        float xpos = x + ch.Bearing.x * scale;
        float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        float w = ch.Size.x * scale;
        float h = ch.Size.y * scale;
        // update VBO for each character
        float vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 0.0f },            
            { xpos,     ypos,       0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 1.0f },

            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 0.0f }           
        };
        // render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        // update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); // be sure to use glBufferSubData and not glBufferData

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        // render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
        x += (ch.Advance >> 6) * scale; // bitshift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}
