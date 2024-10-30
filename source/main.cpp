#include <citro2d.h>
#include <time.h>
#include <deque>
#include <math.h>
#include <fstream>

const int CELL_SIZE = 10;
const int CELL_COUNT = 27;
// the 3ds has different screen width, but the same screen height.
const int TOP_SCREEN_WIDTH = CELL_SIZE * CELL_COUNT;
const int BOTTOM_SCREEN_WIDTH = 320;
const int SCREEN_HEIGHT = CELL_SIZE * CELL_COUNT;

C3D_RenderTarget *topScreen = nullptr;
C3D_RenderTarget *bottomScreen = nullptr;

bool isGamePaused;

int score;
int highScore;

C2D_TextBuf textDynamicBuffer;

C2D_TextBuf textStaticBuffer;
C2D_Text staticTexts[1];

float textSize = 1.0f;

C2D_Image sprite;

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

Rectangle spriteBounds = {TOP_SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, 0, 32, 32, WHITE};
Rectangle ball = {TOP_SCREEN_WIDTH / 2 - 50, SCREEN_HEIGHT / 2, 0, 20, 20, WHITE};
Rectangle bottomBounds = {BOTTOM_SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, 0, 32, 32, BLUE};
Rectangle touchBounds = {0, 0, 0, 8, 8, WHITE};

const int PLAYER_SPEED = 10;

int ballVelocityX = 5;
int ballVelocityY = 5;

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
        saveScore();

        std::string highScoreString = "High Score: " + std::to_string(score);

        // updateTextureText(highScoreTexture, highScoreString.c_str(), fontSquare, renderer);
    }

    snake.body = {{6, 9}, {5, 9}, {4, 9}};
    snake.direction = {1, 0};

    score = 0;
    // updateTextureText(scoreTexture, "Score: 0", fontSquare, renderer);
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

void update()
{
	// Respond to user input
	int keyHeld = hidKeysHeld();

	if (keyHeld & KEY_LEFT && spriteBounds.x > 0)
	{
		spriteBounds.x -= PLAYER_SPEED;
	}

	else if (keyHeld & KEY_RIGHT && spriteBounds.x < TOP_SCREEN_WIDTH - spriteBounds.w)
	{
		spriteBounds.x += PLAYER_SPEED;
	}

	else if (keyHeld & KEY_UP && spriteBounds.y > 0)
	{
		spriteBounds.y -= PLAYER_SPEED;
	}
	
	else if (keyHeld & KEY_DOWN && spriteBounds.y < SCREEN_HEIGHT - spriteBounds.h)
	{
		spriteBounds.y += PLAYER_SPEED;
	}

	if (ball.x < 0 || ball.x > TOP_SCREEN_WIDTH - ball.w)
	{
		ballVelocityX *= -1;

		ball.color = GREEN;
	}

	else if (ball.y < 0 || ball.y > SCREEN_HEIGHT - ball.h)
	{
		ballVelocityY *= -1;

		ball.color = RED;
	}

	else if (hasCollision(spriteBounds, ball))
	{
		ballVelocityX *= -1;
		ballVelocityY *= -1;

		ball.color = BLUE;

		score++;
	}

	ball.x += ballVelocityX;
	ball.y += ballVelocityY;
}

void renderTopScreen()
{
	C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
	C2D_TargetClear(topScreen, BLACK);
	C2D_SceneBegin(topScreen);

	// (float x, float y, float z, float  w, float h, u32 clr)
	C2D_DrawRectSolid(ball.x, ball.y, ball.z, ball.w, ball.h, ball.color);

	// ( 	C2D_Image img, float x, float y, float depth, const C2D_ImageTint *tint C2D_OPTIONALnullptr, float scaleX C2D_OPTIONAL1.0f, float scaleY C2D_OPTIONAL1.0f)
	C2D_DrawImageAt(sprite, spriteBounds.x, spriteBounds.y, 0, NULL, 1, 1);

	if (isGamePaused)
	{
		// Draws text using the GPU.

		// Parameters
		//     [in]	text	Pointer to text object.
		//     [in]	flags	Text drawing flags.
		//     [in]	x	Horizontal position to draw the text on.
		//     [in]	y	Vertical position to draw the text on. If C2D_AtBaseline is not specified (default), this is the top left corner of the block of text; otherwise this is the position of the baseline of the first line of text.
		//     [in]	z	Depth value of the text. If unsure, pass 0.0f.
		//     [in]	scaleX	Horizontal textSize of the font. 1.0f corresponds to the native textSize of the font.
		//     [in]	scaleY	Vertical textSize of the font. 1.0f corresponds to the native textSize of the font.
		// Draw static text strings
		C2D_DrawText(&staticTexts[0], C2D_AtBaseline | C2D_WithColor, 110, 60, 0, textSize, textSize, WHITE);
	}

	C3D_FrameEnd(0);
}

void renderBottomScreen()
{
	C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
	C2D_TargetClear(bottomScreen, BLACK);
	C2D_SceneBegin(bottomScreen);

	C2D_DrawRectSolid(bottomBounds.x, bottomBounds.y, bottomBounds.z, bottomBounds.w, bottomBounds.h, bottomBounds.color);

	C2D_TextBufClear(textDynamicBuffer);

	// Generate and draw dynamic text
	char buf[160];
	C2D_Text dynamicText;
	snprintf(buf, sizeof(buf), "Total collisions: %d", score);
	C2D_TextParse(&dynamicText, textDynamicBuffer, buf);
	C2D_TextOptimize(&dynamicText);
	C2D_DrawText(&dynamicText, C2D_AlignCenter | C2D_WithColor, 150, 175, 0, textSize, textSize, WHITE);

	C3D_FrameEnd(0);
}

int main(int argc, char *argv[])
{
	// Init libs
	romfsInit();
	gfxInitDefault();
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
	C2D_Prepare();

	// Create top and bottom screens
	topScreen = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
	bottomScreen = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

	// Create two text buffers: one for static text, and another one for
	// dynamic text - the latter will be cleared at each frame.
	textStaticBuffer = C2D_TextBufNew(1024); // support up to 4096 glyphs in the buffer
	textDynamicBuffer = C2D_TextBufNew(4096);

	// Parse the static text strings
	C2D_TextParse(&staticTexts[0], textStaticBuffer, "Game Paused");

	// Optimize the static text strings
	C2D_TextOptimize(&staticTexts[0]);

	// Load the sprite
	C2D_SpriteSheet sheet = C2D_SpriteSheetLoad("romfs:/gfx/alien_1.t3x");
	sprite = C2D_SpriteSheetGetImage(sheet, 0);

	touchPosition touch;

	// Main loop
	while (aptMainLoop())
	{
		hidScanInput();

		// Read the touch screen coordinates
		hidTouchRead(&touch);

		if (touch.px > 0 && touch.py > 0 && touch.px < BOTTOM_SCREEN_WIDTH - bottomBounds.w && touch.py < SCREEN_HEIGHT - bottomBounds.h)
		{
			bottomBounds.x = touch.px;
			bottomBounds.y = touch.py;
		}

		touchBounds.x = touch.px;
		touchBounds.y = touch.py;

		if (hasCollision(touchBounds, bottomBounds))
		{
			bottomBounds.color = RED;
		}
		else
		{
			bottomBounds.color = BLUE;
		}

		int keyDown = hidKeysDown();

		if (keyDown & KEY_START)
		{
			isGamePaused = !isGamePaused;
		}

		if (!isGamePaused)
		{
			update();
		}

		renderTopScreen();

		renderBottomScreen();
	}

	// Delete the text buffers
	C2D_TextBufDelete(textDynamicBuffer);
	C2D_TextBufDelete(textStaticBuffer);

	// Deinit libs
	C2D_Fini();
	C3D_Fini();
	gfxExit();
}
