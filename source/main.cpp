#include <citro2d.h>
#include <time.h>
#include <deque>
#include <math.h>
#include <fstream>

const int CELL_SIZE = 11;
const int CELL_COUNT = 20;

const int TOP_SCREEN_WIDTH = CELL_SIZE * CELL_COUNT;
const int BOTTOM_SCREEN_WIDTH = 320;
const int SCREEN_HEIGHT = CELL_SIZE * CELL_COUNT;

C3D_RenderTarget *topScreen = nullptr;
C3D_RenderTarget *bottomScreen = nullptr;

bool isGamePaused;

int score;
int highScore;

C2D_TextBuf scoreDynamicBuffer;
C2D_TextBuf highScoreDynamicBuffer;

C2D_TextBuf textStaticBuffer;
C2D_Text staticTexts[1];

float textSize = 0.9f;

// Create colors
const u32 WHITE = C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF);
const u32 BLACK = C2D_Color32(0x00, 0x00, 0x00, 0x00);
const u32 GREEN = C2D_Color32(0x00, 0xFF, 0x00, 0xFF);
const u32 RED = C2D_Color32(0xFF, 0x00, 0x00, 0xFF);
const u32 BLUE = C2D_Color32(0x00, 0x00, 0xFF, 0xFF);

typedef struct
{
	float x;
	float y;
	float z;
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

Rectangle foodBounds;

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

	//wait 0.2 seconds.
	if (lastUpdateTime >= 20)
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
	highScore = loadHighScore();

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

	if (keyDown & KEY_UP && snake.direction.y != 1)
	{
		snake.direction = {0, -1};
	}

	else if (keyDown & KEY_DOWN && snake.direction.y != -1)
	{
		snake.direction = {0, 1};
	}

	else if (keyDown & KEY_LEFT && snake.direction.x != 1)
	{
		snake.direction = {-1, 0};
	}

	else if (keyDown & KEY_RIGHT && snake.direction.x != -1)
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

void renderTopScreen()
{
	C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
	C2D_TargetClear(topScreen, BLACK);
	C2D_SceneBegin(topScreen);

	for (size_t i = 0; i < snake.body.size(); i++)
	{
		int positionX = snake.body[i].x;
		int positionY = snake.body[i].y;

		Rectangle bodyBounds = {(float) positionX * CELL_SIZE, (float)positionY * CELL_SIZE, 0, CELL_SIZE, CELL_SIZE, WHITE};

		C2D_DrawRectSolid(bodyBounds.x, bodyBounds.y, bodyBounds.z, bodyBounds.w, bodyBounds.h, bodyBounds.color);
	}

	//(float) to avoid warning of conversion.
	foodBounds = {(float) food.position.x * CELL_SIZE, (float)food.position.y * CELL_SIZE, 0, CELL_SIZE, CELL_SIZE, WHITE};

	C2D_DrawRectSolid(foodBounds.x, foodBounds.y, foodBounds.z, foodBounds.w, foodBounds.h, foodBounds.color);

	C2D_DrawLine(0, 1, WHITE, TOP_SCREEN_WIDTH, 1, WHITE, 1, 1);
    C2D_DrawLine(0, SCREEN_HEIGHT - 1, WHITE,  TOP_SCREEN_WIDTH, SCREEN_HEIGHT - 1, WHITE, 1, 1);
    C2D_DrawLine(0, 0, WHITE, 0, SCREEN_HEIGHT, WHITE, 1, 1);
    C2D_DrawLine(TOP_SCREEN_WIDTH - 1, 0, WHITE, TOP_SCREEN_WIDTH - 1, SCREEN_HEIGHT, WHITE, 1, 1);

	C3D_FrameEnd(0);
}

void renderBottomScreen()
{
	C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
	C2D_TargetClear(bottomScreen, BLACK);
	C2D_SceneBegin(bottomScreen);

	C2D_TextBufClear(scoreDynamicBuffer);
	C2D_TextBufClear(highScoreDynamicBuffer);

	char buf[160];
	C2D_Text dynamicText;
	snprintf(buf, sizeof(buf), "Score: %d", score);
	C2D_TextParse(&dynamicText, scoreDynamicBuffer, buf);
	C2D_TextOptimize(&dynamicText);
	C2D_DrawText(&dynamicText, C2D_AlignCenter | C2D_WithColor, 250, 20, 0, textSize, textSize, WHITE);

	char buf2[160];
	C2D_Text dynamicText2;
	snprintf(buf2, sizeof(buf2), "High Score: %d", highScore);
	C2D_TextParse(&dynamicText2, highScoreDynamicBuffer, buf2);
	C2D_TextOptimize(&dynamicText2);
	C2D_DrawText(&dynamicText2, C2D_AlignCenter | C2D_WithColor, 90, 20, 0, textSize, textSize, WHITE);

	if (isGamePaused)
	{
		C2D_DrawText(&staticTexts[0], C2D_AtBaseline | C2D_WithColor, 80, 80, 0, textSize, textSize, WHITE);
	}

	C3D_FrameEnd(0);
}

int main(int argc, char *argv[])
{
	romfsInit();
	gfxInitDefault();
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
	C2D_Prepare();

	highScore = loadHighScore();

	topScreen = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
	bottomScreen = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

	textStaticBuffer = C2D_TextBufNew(1024);
	scoreDynamicBuffer = C2D_TextBufNew(4096);
	highScoreDynamicBuffer = C2D_TextBufNew(4096);

	C2D_TextParse(&staticTexts[0], textStaticBuffer, "Game Paused");

	C2D_TextOptimize(&staticTexts[0]);

	srand(time(NULL));

	Vector2 initialFoodPosition = generateRandomPosition();

	food = {CELL_COUNT, CELL_SIZE, initialFoodPosition, false};

	std::deque<Vector2> initialBody = {{6, 9}, {5, 9}, {4, 9}};
	Vector2 direction = {1, 0};

	snake = {CELL_COUNT, CELL_SIZE, initialBody, direction, false};

	while (aptMainLoop())
	{
		hidScanInput();

		int keyDown = hidKeysDown();

		if (keyDown & KEY_START)
		{
			isGamePaused = !isGamePaused;
		}

		if (!isGamePaused)
		{
			update(keyDown);
		}

		renderTopScreen();

		renderBottomScreen();
	}

	C2D_TextBufDelete(scoreDynamicBuffer);
	C2D_TextBufDelete(textStaticBuffer);

	C2D_Fini();
	C3D_Fini();
	gfxExit();
}
