#include <grrlib.h>
#include <ogc/pad.h>
#include <time.h>
#include <deque>
#include <math.h>
#include <fstream>
#include "BMfont3_png.h"

#define BLACK 0x000000FF
#define WHITE 0xFFFFFFFF
#define GREEN 0x008000FF
#define RED 0xFF0000FF
#define BLUE 0x0000FFFF

const int CELL_SIZE = 20;
const int CELL_COUNT = 24;

const int SCREEN_WIDTH = CELL_SIZE * CELL_COUNT;
const int SCREEN_HEIGHT = CELL_SIZE * CELL_COUNT;

bool isGamePaused;

int score;
int highScore;

typedef struct
{
    float x;
    float y;
    float w;
    float h;
    unsigned int color;
} Rectangle;

typedef struct
{
    int x;
    int y;
} Vector2;

typedef struct
{
    int cellCount;
    int cellSize;
    std::deque<Vector2> body;
    Vector2 direction;
    bool shouldAddSegment;
} Snake;

Snake snake;

typedef struct
{
    int cellCount;
    int cellSize;
    Vector2 position;
    bool isDestroyed;
} Food;

Food food;

int rand_range(int min, int max)
{
    return min + rand() / (RAND_MAX / (max - min + 1) + 1);
}

Vector2 generateRandomPosition()
{
    int positionX = rand_range(0, CELL_COUNT - 1);
    int positionY = rand_range(0, CELL_COUNT - 1);

    return Vector2{positionX, positionY};
}

Vector2 vector2Add(Vector2 vector1, Vector2 vector2)
{
    Vector2 result = {vector1.x + vector2.x, vector1.y + vector2.y};

    return result;
}

int vector2Equals(Vector2 vector1, Vector2 vector2)
{
    const float EPSILON = 0.000001f;
    int result = ((fabsf(vector1.x - vector2.x)) <= (EPSILON * fmaxf(1.0f, fmaxf(fabsf(vector1.x), fabsf(vector2.x))))) &&
                 ((fabsf(vector1.y - vector2.y)) <= (EPSILON * fmaxf(1.0f, fmaxf(fabsf(vector1.y), fabsf(vector2.y)))));

    return result;
}

double lastUpdateTime = 0;

bool eventTriggered(int counter)
{
    lastUpdateTime += counter;

    if (lastUpdateTime >= 70)
    {
        lastUpdateTime = 0;

        return true;
    }

    return false;
}

void saveScore()
{
    std::string path = "high-score.txt";

    std::ofstream highScores(path);

    std::string scoreString = std::to_string(score);
    highScores << scoreString;

    highScores.close();
}

int loadHighScore()
{
    std::string highScoreText;

    std::string path = "high-score.txt";

    std::ifstream highScores(path);

    if (!highScores.is_open())
    {
        saveScore();

        std::ifstream auxHighScores(path);

        getline(auxHighScores, highScoreText);

        highScores.close();

        int highScore = stoi(highScoreText);

        return highScore;
    }

    getline(highScores, highScoreText);

    highScores.close();

    int highScore = stoi(highScoreText);

    return highScore;
}

void resetSnakePosition()
{
    // highScore = loadHighScore();

    if (score > highScore)
    {
        highScore = score;
        saveScore();
    }

    snake.body = {{6, 9}, {5, 9}, {4, 9}};
    snake.direction = {1, 0};

    score = 0;
}

bool checkCollisionWithFood(Vector2 foodPosition)
{
    if (vector2Equals(snake.body[0], foodPosition))
    {
        snake.shouldAddSegment = true;
        return true;
    }

    return false;
}

void checkCollisionWithEdges()
{
    if (snake.body[0].x == CELL_COUNT || snake.body[0].x == -1 || snake.body[0].y == CELL_COUNT || snake.body[0].y == -1)
    {
        resetSnakePosition();
    }
}

void checkCollisionBetweenHeadAndBody()
{
    for (size_t i = 1; i < snake.body.size(); i++)
    {
        if (vector2Equals(snake.body[0], snake.body[i]))
        {
            resetSnakePosition();
        }
    }
}

bool hasCollision(Rectangle &bounds, Rectangle &ball)
{
    return bounds.x < ball.x + ball.w && bounds.x + bounds.w > ball.x &&
           bounds.y < ball.y + ball.h && bounds.y + bounds.h > ball.y;
}

int counter = 0;

void update(int keyDown)
{
    counter++;

    if (eventTriggered(counter))
    {
        counter = 0;
        if (!snake.shouldAddSegment)
        {
            snake.body.pop_back();
            snake.body.push_front(vector2Add(snake.body[0], snake.direction));
        }
        else
        {
            snake.body.push_front(vector2Add(snake.body[0], snake.direction));
            snake.shouldAddSegment = false;
        }
    }

    if (keyDown & PAD_BUTTON_UP && snake.direction.y != 1)
    {
        snake.direction = {0, -1};
    }

    else if (keyDown & PAD_BUTTON_DOWN && snake.direction.y != -1)
    {
        snake.direction = {0, 1};
    }

    else if (keyDown & PAD_BUTTON_LEFT && snake.direction.x != 1)
    {
        snake.direction = {-1, 0};
    }

    else if (keyDown & PAD_BUTTON_RIGHT && snake.direction.x != -1)
    {
        snake.direction = {1, 0};
    }

    checkCollisionWithEdges();
    checkCollisionBetweenHeadAndBody();

    food.isDestroyed = checkCollisionWithFood(food.position);

    if (food.isDestroyed)
    {
        food.position = generateRandomPosition();
        score++;
    }
}

int main(int argc, char **argv)
{
    GRRLIB_Init();

    GRRLIB_texImg *blueFonts = GRRLIB_LoadTexture(BMfont3_png);

    GRRLIB_InitTileSet(blueFonts, 32, 32, 32);

    // highScore = loadHighScore();

    srand(time(NULL));

    Vector2 initialFoodPosition = generateRandomPosition();

    food = {CELL_COUNT, CELL_SIZE, initialFoodPosition, false};

    std::deque<Vector2> initialBody = {{6, 9}, {5, 9}, {4, 9}};
    Vector2 direction = {1, 0};

    snake = {CELL_COUNT, CELL_SIZE, initialBody, direction, false};

    PAD_Init();

    while (true)
    {
        PAD_ScanPads();

        int padDown = PAD_ButtonsDown(0);

        if (padDown & PAD_BUTTON_START)
        {
            isGamePaused = !isGamePaused;
        }

        if (!isGamePaused)
        {
            update(padDown);
        }

        GRRLIB_FillScreen(BLACK);

        for (size_t i = 0; i < snake.body.size(); i++)
        {
            int positionX = snake.body[i].x;
            int positionY = snake.body[i].y;

            Rectangle bodyBounds = {(float)positionX * CELL_SIZE, (float)positionY * CELL_SIZE, CELL_SIZE, CELL_SIZE, WHITE};

            GRRLIB_Rectangle(bodyBounds.x, bodyBounds.y, bodyBounds.w, bodyBounds.h, bodyBounds.color, 1);
        }

        //(float) to avoid warning of conversion.
        Rectangle foodBounds = {(float)food.position.x * CELL_SIZE, (float)food.position.y * CELL_SIZE, CELL_SIZE, CELL_SIZE, WHITE};

        GRRLIB_Rectangle(foodBounds.x, foodBounds.y, foodBounds.w, foodBounds.h, foodBounds.color, 1);

        GRRLIB_Line(0, 1, SCREEN_WIDTH, 1, WHITE);
        GRRLIB_Line(0, SCREEN_HEIGHT - 1, SCREEN_WIDTH, SCREEN_HEIGHT - 1, WHITE);
        GRRLIB_Line(0, 0, 0, SCREEN_HEIGHT, WHITE);
        GRRLIB_Line(SCREEN_WIDTH - 1, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT, WHITE);

        if (isGamePaused)
        {
            GRRLIB_Printf(60, 100, blueFonts, WHITE, 1, "GAME PAUSED");
        }

        std::string highScoreString = "HIGH:" + std::to_string(highScore);
        GRRLIB_Printf(10, 20, blueFonts, WHITE, 1, highScoreString.c_str());

        std::string scoreString = "SCORE:" + std::to_string(score);
        GRRLIB_Printf(250, 20, blueFonts, WHITE, 1, scoreString.c_str());

        GRRLIB_Render();
    }

    GRRLIB_FreeTexture(blueFonts);

    GRRLIB_Exit();
    exit(0);
}
