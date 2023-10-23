/**
* Author:  Jack Ma
* Assignment: Pong Clone
* Date due: 2023-10-21, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/


#define GL_SILENCE_DEPRECATION
#define GL_GLEXT_PROTOTYPES 1
#define LOG(argument) std::cout << argument << '\n'
#define STB_IMAGE_IMPLEMENTATION

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"                // 4x4 Matrix
#include "glm/gtc/matrix_transform.hpp"  // Matrix transformation methods
#include "ShaderProgram.h"               // We'll talk about these later in the course
#include "stb_image.h"

SDL_Window* displayWindow;
bool gameIsRunning = true;

bool isRoundOver = true;
bool allowMovement = true;
bool goUp = true;
bool p1Wins = false;
bool p2Wins = false;

ShaderProgram program;
glm::mat4 viewMatrix, player1Matrix, player2Matrix, ballMatrix, projectionMatrix;

// player 1
glm::vec3 player1Position = glm::vec3(-4.6, 0, 0);
glm::vec3 player1Movement = glm::vec3(0, 0, 0);

// player 2
glm::vec3 player2Position = glm::vec3(4.6, 0, 0);
glm::vec3 player2Movement = glm::vec3(0, 0, 0);

// paddles
glm::vec3 paddleSize = glm::vec3(0.5f, 2.0f, 1.0f);
float paddleHeight = 1.0f * paddleSize.y;
float paddleWidth = 1.0f * paddleSize.x;
float paddleSpeed = 3.0f;

// ball
glm::vec3 ballPosition = glm::vec3(0, 0, 0);
glm::vec3 ballMovement = glm::vec3(0, 0, 0);
float ballSpeed = 3.0f;
glm::vec3 ballSize = glm::vec3(0.25f, 0.25f, 1.0f);
float ballWidth = 1.0f * ballSize.x;
float ballHeight = 1.0f * ballSize.y;

GLuint player1TextureID;
GLuint player2TextureID;
GLuint ballTextureID;

// for deltaTime
float lastTicks = 0.0f;


GLuint LoadTexture(const char* filePath) {
    int w, h, n;
    unsigned char* image = stbi_load(filePath, &w, &h, &n, STBI_rgb_alpha);

    if (image == NULL) {
        std::cout << "Unable to load image. Make sure the path is correct\n";
        assert(false);
    }

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    stbi_image_free(image);
    return textureID;
}

void Initialize() {
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("Pong!", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(0, 0, 640, 480);

    program.Load("shaders/vertex_textured.glsl", "shaders/fragment_textured.glsl");

    viewMatrix = glm::mat4(1.0f);
    player1Matrix = glm::mat4(1.0f);
    player2Matrix = glm::mat4(1.0f);
    ballMatrix = glm::mat4(1.0f);
    projectionMatrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);


    program.SetProjectionMatrix(projectionMatrix);
    program.SetViewMatrix(viewMatrix);
    //program.SetColor(1.0f, 0.0f, 0.0f, 1.0f);

    glUseProgram(program.programID);

    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    player1TextureID = LoadTexture("assets/creeper.png");
    player2TextureID = LoadTexture("assets/steve.png");
    // couldnt load diamond texture, so i just gave up
    //ballTextureID = LoadTexture("assets/Diamond.png");

}

bool checkHitTop(glm::vec3 position, float heightDifference) {
    if (position.y + heightDifference >= 3.75f) {
        return true;
    }
    return false;
}

bool checkHitBottom(glm::vec3 position, float heightDifference) {
    if (position.y - heightDifference <= -3.75f) {
        return true;
    }
    return false;
}

// box to box detection
bool collisionDetection(bool player) {
    float ballX = ballPosition.x;
    float ballY = ballPosition.y;

    // player 1
    float playerX, playerY;
    if (player) {
        playerX = player1Position.x;
        playerY = player1Position.y;
    }
    else {
        playerX = player2Position.x;
        playerY = player2Position.y;
    }

    float xdist = fabs(playerX - ballX) - ((ballWidth + paddleWidth) / 2.0f);
    float ydist = fabs(playerY - ballY) - ((ballHeight  + paddleHeight) / 2.0f);

    // colliding
    if (xdist < 0 && ydist < 0) {
        return true;
    }
    else {
        return false;
    }
}

void singlePlayer() {
    if (goUp) {
        player2Movement.y = 1.0f;
    }
    else {
        player2Movement.y = 1.0f;
    }
    if (player2Position.y > 3.0f) {
        goUp = false;
    }
    else if (player2Position.y < -2.9f) {
        goUp = true;
    }
    else {
        player2Movement.y = 0.0f;
    }
}

void ProcessInput() {

    player1Movement = glm::vec3(0);
    player2Movement = glm::vec3(0);

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT:

        case SDL_WINDOWEVENT_CLOSE:
            gameIsRunning = false;
            break;

        case SDL_KEYDOWN:
            switch (event.key.keysym.sym) {
            case SDLK_LEFT:
                // Move player 2 up
                break;
            case SDLK_RIGHT:
                // Move player 2 down
                break;
            case SDLK_SPACE:
                // Starts the game
                break;
            case SDLK_t:
                // Goes into single player
                break;
            }
            break; // SDL_KEYDOWN
        }
    }

    const Uint8* keys = SDL_GetKeyboardState(NULL);

    // press space to start game
    if (keys[SDL_SCANCODE_SPACE]) {
        int slope = rand();
        ballMovement.x = 1.0f * slope;
        ballMovement.y = 1.0f * slope;
    }
    if (glm::length(ballMovement) > 1.0f) {
        ballMovement = glm::normalize(ballMovement);
    }

    if (keys[SDL_SCANCODE_W] && !checkHitTop(player1Position, paddleHeight / 2)) {
        player1Movement.y = 1.0f;
    }
    else if (keys[SDL_SCANCODE_S] && !checkHitBottom(player1Position, paddleHeight / 2)) {
        player1Movement.y = -1.0f;
    }

    if (keys[SDL_SCANCODE_UP] && !checkHitTop(player2Position, paddleHeight / 2)) {
        player2Movement.y = 1.0f;
    }
    else if (keys[SDL_SCANCODE_DOWN] && !checkHitBottom(player2Position, paddleHeight / 2)) {
        player2Movement.y = -1.0f;
    }
    // if T is pressed, switch to single player mode
    while (keys[SDL_SCANCODE_T] && !checkHitBottom(player2Position, paddleHeight / 2) && !checkHitTop(player2Position, paddleHeight / 2)) {
        singlePlayer();
    }
}

void Update() {
    float ticks = (float)SDL_GetTicks() / 1000.0f;
    float deltaTime = ticks - lastTicks;
    lastTicks = ticks;

    if (ballPosition.x >= 5.0f || ballPosition.x <= -5.0f) {
        gameIsRunning = false;
    }

    player1Matrix = glm::mat4(1.0f);
    player1Position += player1Movement * paddleSpeed * deltaTime;
    player1Matrix = glm::translate(player1Matrix, player1Position);
    player1Matrix = glm::scale(player1Matrix, paddleSize);

    player2Matrix = glm::mat4(1.0f);
    player2Position += player2Movement * paddleSpeed * deltaTime;
    player2Matrix = glm::translate(player2Matrix, player2Position);
    player2Matrix = glm::scale(player2Matrix, paddleSize);

    ballMatrix = glm::mat4(1.0f);
    if (checkHitTop(ballPosition, ballHeight) || checkHitBottom(ballPosition, ballHeight)) {
        ballMovement.y *= -1.0f;
    }
    if (collisionDetection(true) ||
        collisionDetection(false)) {
        ballMovement.x *= -1.0f;
    }
    ballPosition += ballMovement * ballSpeed * deltaTime;
    ballMatrix = glm::translate(ballMatrix, ballPosition);
    ballMatrix = glm::scale(ballMatrix, ballSize);
}


void Render() {
    glClear(GL_COLOR_BUFFER_BIT);
    float vertices[] = { -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };
    float texCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
    glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(program.positionAttribute);
    glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
    glEnableVertexAttribArray(program.texCoordAttribute);

    program.SetModelMatrix(player1Matrix);
    glBindTexture(GL_TEXTURE_2D, player1TextureID);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    program.SetModelMatrix(player2Matrix);
    glBindTexture(GL_TEXTURE_2D, player2TextureID);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    program.SetModelMatrix(ballMatrix);
    glBindTexture(GL_TEXTURE_2D, ballTextureID);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDisableVertexAttribArray(program.positionAttribute);
    glDisableVertexAttribArray(program.texCoordAttribute);

    SDL_GL_SwapWindow(displayWindow);
}

void Shutdown() {
    SDL_Quit();
}

int main(int argc, char* argv[]) {
    Initialize();
    
    while (gameIsRunning) {
        ProcessInput();
        Update();
        Render();
    }

    Shutdown();
    return 0;
}
