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

//Prototyping
void get_resolution(int &windowwidth, int &windowheight);
void installShaders();
void RenderText(Shader &s, std::string text, float x, float y, float scale, glm::vec3 color);

//Initializing
int WINDOW_HEIGHT;
int WINDOW_WIDTH;
double dt = 0;
double t = 0;
unsigned int VAO, VBO;
GLuint programID;

//Character struct
struct Character {
    unsigned int    TextureID;  // ID handle of the glyph texture
    glm::ivec2      Size;       // Size of glyph
    glm::ivec2      Bearing;    // Offset from baseline to left/top of glyph
    unsigned int    Advance;    // Offset to advance to next glyph
};
std::map<char, Character> Characters;

int main() {
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
    error_check = FT_New_Face (library, "/home/sean/Documents/Fonts/Poppins/Poppins-Regular.otf", 0, &face);
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
        
        // //Drawing
        // const float DEG2RAD = 3.14159 / 180;
        // if (t > 5) {
        //     glColor3f(0, 1, 0);
        // } else {
        //     glColor3f(1, 0, 0);
        // }
        // glBegin(GL_POLYGON);
        //     for (int i = 0; i < 360; i++) {
        //         float degInRad = i*DEG2RAD;
        //         glVertex2f((cos(degInRad)*1 + 0) / ratio, sin(degInRad)*1 + 0);
        //     }
        // glEnd();

        //Text
        RenderText(shader, "BoomZap", 0, 0, 1.0f, glm::vec3(1.0f, 1.0f, 1.0f));
        
        //Swap Buffer and Poll Events
        glfwSwapBuffers(window);
        glfwPollEvents();

        //Keeping track of time
        auto end = clock();
        dt = difftime(end, start) / CLOCKS_PER_SEC;
        t += dt;
    }
    
    //Close Window
    glfwTerminate();
    exit(EXIT_SUCCESS);
}

//Defining
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

// //Shaders
// const char* vertexShaderCode =
//     "#version 330 core\n"
//     ""
//     "layout (location = 0) in vec4 vertex;"
//     "out vec2 TexCoords;"
//     ""
//     "uniform mat4 projection;"
//     ""
//     "void main()"
//     "{"
//         "gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);"
//         "TexCoords = vertex.zw;"
//     "}";

// const char* fragmentShaderCode =
//     "#version 330 core\n"
//     ""
//     "in vec2 TexCoords;"
//     "out vec4 color;"
//     ""
//     "uniform sampler2D text;"
//     "uniform vec3 textColor;"
//     ""
//     "void main()"
//     "{"
//         "vec 4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);"
//         "color = vec4(textColor, 1.0) * sampled;"
//     "}";

// void installShaders() {
//     GLuint vertexShaderID = glCreateShader(GL_VERTEX_SHADER);
//     GLuint fragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

//     const char* adapter[1];
//     adapter[0] = vertexShaderCode;
//     glShaderSource(vertexShaderID, 1, adapter, 0);
//     adapter[0] = fragmentShaderCode;
//     glShaderSource(fragmentShaderID, 1, adapter, 0);

//     glCompileShader(vertexShaderID);
//     glCompileShader(fragmentShaderID);

//     programID = glCreateProgram();
//     glAttachShader(programID, vertexShaderID);
//     glAttachShader(programID, fragmentShaderID);
//     glLinkProgram(programID);

//     glUseProgram(programID);
// }