#include "sdl_starter.h"
#include "sdl_assets_loader.h"
#include <time.h>
#include <deque>
#include <math.h>
#include <fstream>

SDL_Window *window = nullptr;
SDL_Renderer *renderer = nullptr;

Mix_Chunk *actionSound = nullptr;

bool isGamePaused;

int score;
int highScore;

SDL_Texture *pauseTexture = nullptr;
SDL_Rect pauseBounds;

SDL_Texture *scoreTexture = nullptr;
SDL_Rect scoreBounds;

SDL_Texture *highScoreTexture = nullptr;
SDL_Rect highScoreBounds;

TTF_Font *fontSquare = nullptr;

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

SDL_Rect foodBounds; 

int rand_range(int min, int max)
{
    return min + rand() / (RAND_MAX / (max - min + 1) + 1);
}

// check the random position, cuz sometimes sent the food out of bounds.
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
// Check whether two given vectors are almost equal
int vector2Equals(Vector2 vector1, Vector2 vector2)
{
    const float EPSILON = 0.000001f;
    int result = ((fabsf(vector1.x - vector2.x)) <= (EPSILON * fmaxf(1.0f, fmaxf(fabsf(vector1.x), fabsf(vector2.x))))) &&
                 ((fabsf(vector1.y - vector2.y)) <= (EPSILON * fmaxf(1.0f, fmaxf(fabsf(vector1.y), fabsf(vector2.y)))));

    return result;
}

double lastUpdateTime = 0;

// method for control the speed that the snake has to move.
bool eventTriggered(float deltaTime, float intervalUpdate)
{
    lastUpdateTime += deltaTime;

    if (lastUpdateTime >= intervalUpdate)
    {
        lastUpdateTime = 0;

        return true;
    }

    return false;
}

void saveScore()
{
    std::ofstream highScores("high-score.txt");

    std::string scoreString = std::to_string(score);
    highScores << scoreString;

    highScores.close();
}

int loadHighScore()
{
    std::string highScoreText;

    std::ifstream highScores("high-score.txt");

    if (!highScores.is_open())
    {
        saveScore();

        std::ifstream auxHighScores("high-score.txt");

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
    highScore = loadHighScore(); 

    if (score > highScore)
    {
        saveScore();

        std::string highScoreString = "High Score: " + std::to_string(score);

        updateTextureText(highScoreTexture, highScoreString.c_str(), fontSquare, renderer);
    }

    snake.body = {{6, 9}, {5, 9}, {4, 9}};
    snake.direction = {1, 0};

    score = 0;
    updateTextureText(scoreTexture, "Score: 0", fontSquare, renderer);
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

void quitGame()
{
    Mix_FreeChunk(actionSound);
    SDL_DestroyTexture(pauseTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    Mix_CloseAudio();
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();
}

void handleEvents()
{
    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT || event.key.keysym.sym == SDLK_ESCAPE)
        {
            quitGame();
            exit(0);
        }

        // To handle key pressed more precise, I use this method for handling pause the game or jumping.
        if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_SPACE)
        {
            isGamePaused = !isGamePaused;
            Mix_PlayChannel(-1, actionSound, 0);
        }
    }
}

void update(float deltaTime)
{
    const Uint8 *currentKeyStates = SDL_GetKeyboardState(NULL);

    if (eventTriggered(deltaTime, 0.2))
    {
        if (!snake.shouldAddSegment)
        {
            // we remove the last element (The tail of the snake) and we push at the head the head + the direction
            snake.body.pop_back();
            snake.body.push_front(vector2Add(snake.body[0], snake.direction));
        }
        else
        {
            // its better to add the new element at the head, cuz this gives a better visual effect.
            //  When the element is added we need to stop the snake movement, to complete the visual effect.
            snake.body.push_front(vector2Add(snake.body[0], snake.direction));

            snake.shouldAddSegment = false;
        }
    }

    if (currentKeyStates[SDL_SCANCODE_W] && snake.direction.y != 1)
    {
        snake.direction = {0, -1};
    }

    else if (currentKeyStates[SDL_SCANCODE_S] && snake.direction.y != -1)
    {
        snake.direction = {0, 1};
    }

    else if (currentKeyStates[SDL_SCANCODE_A] && snake.direction.x != 1)
    {
        snake.direction = {-1, 0};
    }

    else if (currentKeyStates[SDL_SCANCODE_D] && snake.direction.x != -1)
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

        std::string scoreString = "score: " + std::to_string(score);

        updateTextureText(scoreTexture, scoreString.c_str(), fontSquare, renderer);

        Mix_PlayChannel(-1, actionSound, 0);
    }
}

void render()
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    if (isGamePaused)
    {
        SDL_RenderCopy(renderer, pauseTexture, NULL, &pauseBounds);
    }

    SDL_QueryTexture(scoreTexture, NULL, NULL, &scoreBounds.w, &scoreBounds.h);
    scoreBounds.x = SCREEN_WIDTH / 2 + 90;
    scoreBounds.y = scoreBounds.h / 2;
    SDL_RenderCopy(renderer, scoreTexture, NULL, &scoreBounds);

    SDL_QueryTexture(highScoreTexture, NULL, NULL, &highScoreBounds.w, &highScoreBounds.h);
    highScoreBounds.x = 50;
    highScoreBounds.y = highScoreBounds.h / 2;
    SDL_RenderCopy(renderer, highScoreTexture, NULL, &highScoreBounds);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    for (size_t i = 0; i < snake.body.size(); i++)
    {
        int positionX = snake.body[i].x;
        int positionY = snake.body[i].y;

        SDL_Rect bodyBounds = {positionX * CELL_SIZE, positionY * CELL_SIZE, CELL_SIZE, CELL_SIZE};

        SDL_RenderFillRect(renderer, &bodyBounds);
    }

    foodBounds = {food.position.x * CELL_SIZE, food.position.y * CELL_SIZE, CELL_SIZE, CELL_SIZE};

    SDL_RenderFillRect(renderer, &foodBounds);

    SDL_RenderDrawLine(renderer, 0, 1, SCREEN_WIDTH, 1);
    SDL_RenderDrawLine(renderer, 0, SCREEN_HEIGHT - 1, SCREEN_WIDTH, SCREEN_HEIGHT - 1);
    SDL_RenderDrawLine(renderer, 0, 0, 0, SCREEN_HEIGHT);
    SDL_RenderDrawLine(renderer, SCREEN_WIDTH - 1, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT);

    SDL_RenderPresent(renderer);
}

int main(int argc, char *args[])
{
    window = SDL_CreateWindow("My Window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (startSDL(window, renderer) > 0)
    {
        return 1;
    }

    fontSquare = TTF_OpenFont("res/fonts/square_sans_serif_7.ttf", 22);

    updateTextureText(pauseTexture, "Game Paused", fontSquare, renderer);

    SDL_QueryTexture(pauseTexture, NULL, NULL, &pauseBounds.w, &pauseBounds.h);
    pauseBounds.x = SCREEN_WIDTH / 2 - pauseBounds.w / 2;
    pauseBounds.y = 100;

    highScore = loadHighScore();

    std::string highScoreString = "High Score: " + std::to_string(highScore);

    updateTextureText(highScoreTexture, highScoreString.c_str(), fontSquare, renderer);

    updateTextureText(scoreTexture, "Score: 0", fontSquare, renderer);

    actionSound = loadSound("res/sounds/magic.wav");

    Mix_VolumeChunk(actionSound, MIX_MAX_VOLUME / 2);

    Uint32 previousFrameTime = SDL_GetTicks();
    Uint32 currentFrameTime = previousFrameTime;
    float deltaTime = 0.0f;

    srand(time(NULL));

    Vector2 initialFoodPosition = generateRandomPosition();

    food = {CELL_COUNT, CELL_SIZE, initialFoodPosition, false};

    std::deque<Vector2> initialBody = {{6, 9}, {5, 9}, {4, 9}};
    Vector2 direction = {1, 0};

    snake = {CELL_COUNT, CELL_SIZE, initialBody, direction, false};

    while (true)
    {
        currentFrameTime = SDL_GetTicks();
        deltaTime = (currentFrameTime - previousFrameTime) / 1000.0f;
        previousFrameTime = currentFrameTime;

        SDL_GameControllerUpdate();

        handleEvents();

        if (!isGamePaused)
        {
            update(deltaTime);
        }

        render();

        capFrameRate(currentFrameTime);
    }
}