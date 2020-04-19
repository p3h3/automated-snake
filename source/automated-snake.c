#include <stdio.h>
#include <stdlib.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
#include "hamiltonian-cycle.h"

// Settings
#define gridTileSize 16
#define gridSize 16
#define width 640
#define height 480
#define initialSnakeLength 4

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

// Cursor potitions for the first WiiMote
int cursorX = 0;
int cursorY = 0;

// Data structure for the WiiMote IR
ir_t ir;

// Game state variables
bool inGame = false;
bool paused = false;
int score = 0;

// Other variables needed for running
int dx = 0;
int dy = 1;
unsigned int framerateDevideCounter = 0;

// Create apple
int apple[2];

// Create snake
unsigned int snakeLength = initialSnakeLength;
int snake[gridSize*gridSize][2];

// Function to reset the snake array
void initialiseSnake(){
	for(unsigned int i = 0; i < gridSize*gridSize; i++){
		snake[i][0] = 0; // Reset x
		snake[i][1] = 0; // Reset y
	}
	dx = 0;
	dy = 1;
	for(unsigned int i = 0; i < snakeLength; i++){
		snake[i][0] = i + 1; // Initialise x
		snake[i][1] = 1; // Initialise y
	}
}

// Set new random apple position
void newApplePosition(){
	apple[0] = rand() % gridSize; // Setting x
	apple[1] = rand() % gridSize; // Setting y
}

int convertGridToX(int gridX){
	return (width/2 - (gridSize/2)*gridTileSize) + gridX * gridTileSize;
}
int convertGridToY(int gridY){
	return (height/2 - (gridSize/2)*gridTileSize) + gridY * gridTileSize;
}

void pointCursor(int row, int column){
	// The console understands VT terminal escape codes
	printf ("\x1b[%d;%dH", row, column);
}

void drawHLine(int x1, int x2, int y, int color){
	y *= 320;
	x1 >>= 1;
	x2 >>= 1;
	for (int i = x1; i <= x2; i++){
		u32 * tempfb = xfb;
		tempfb[y+i] = color;
	}
}
void drawVLine(int x, int y1, int y2, int color){
	x >>= 1;
	for (int i = y1; i <= y2; i++){
		u32 * tempfb = xfb;
		tempfb[x + (640*i)/2] = color;
	}
}
void drawBox(int x1, int y1, int x2, int y2, int color){
	drawHLine(x1, x2, y1, color);
	drawHLine(x1, x2, y2, color);
	drawVLine(x1, y1, y2, color);
	drawVLine(x2, y1, y2, color);
}
void drawSolidBox(int x1, int y1, int x2, int y2, int color){
	for(int i = y1; i <= y2; i++){
		drawHLine(x1, x2, i, color);
	}
}

// Doesn't do anything useful yet..
static void print_circuit(const unsigned int *circuit, size_t len) {
    for (unsigned int v = 0; v < len; v++) {
        printf("%d ", circuit[v]);
    }
    putchar('\n');
}

//---------------------------------------------------------------------------------
int main(int argc, char **argv) {
//---------------------------------------------------------------------------------

	unsigned int **adjmat;
 
    // Create a map the size of the grid
    adjmat = malloc(gridSize * sizeof(unsigned int *));
    for (unsigned int i = 0; i < gridSize; i++) {
        adjmat[i] = malloc(gridSize * sizeof(unsigned int));
        for (unsigned int j = 0; j < gridSize; j++) {
            adjmat[i][j] = 1;
        }
    }
	
	// Calculate all possible Hamiltonian Circle Paths
    hamiltonian_circuits(adjmat, gridSize, print_circuit);
	
	// Deallocating no longer needed variable
    for (unsigned int i = 0; i < gridSize; i++) {
        free(adjmat[i]);
    }
    free(adjmat);

	// Initialise the video system
	VIDEO_Init();

	// This function initialises the attached controllers
	WPAD_Init();
	WPAD_SetVRes(0, 640, 480);
	WPAD_SetDataFormat(WPAD_CHAN_0, WPAD_FMT_BTNS_ACC_IR);

	// Obtain the preferred video mode from the system
	rmode = VIDEO_GetPreferredMode(NULL);

	// Allocate memory for the display in the uncached region
	xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

	// Initialise the console, required for printf
	console_init(xfb,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);

	// Set up the video registers with the chosen mode
	VIDEO_Configure(rmode);

	// Tell the video hardware where our display memory is
	VIDEO_SetNextFramebuffer(xfb);

	// Make the display visible
	VIDEO_SetBlack(FALSE);

	// Flush the video register changes to the hardware
	VIDEO_Flush();

	// Wait for Video setup to complete
	VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();

	while(1) {

		// Updates the controller states
		WPAD_ScanPads();
		u32 pressed = WPAD_ButtonsDown(0);
		
		// Updates IR cursor movement
		WPAD_IR(0, &ir);
		
		if(inGame){
			// Making snake longer based on increased score
			snakeLength = initialSnakeLength + score;
		
			// Making the framebuffer black
			VIDEO_ClearFrameBuffer(rmode, xfb, COLOR_BLACK);
			
			// Game
			pointCursor(4, 0);
			printf("Score: %d", score);
			
			//Check for movement updates
			if(pressed & WPAD_BUTTON_UP){
				dx = 0;
				dy = -1;
			}
			if(pressed & WPAD_BUTTON_DOWN){
				dx = 0;
				dy = 1;
			}
			if(pressed & WPAD_BUTTON_RIGHT){
				dx = 1;
				dy = 0;
			}
			if(pressed & WPAD_BUTTON_LEFT){
				dx = -1;
				dy = 0;
			}
			
			// Only updating snake cell positions every 8 frames
			framerateDevideCounter++;
			if(framerateDevideCounter % 8 == 0){
				// Moving all snake cells into their next cells' positions
				for(unsigned int i = snakeLength-1; i >= 1; i--){
					snake[i][0] = snake[i-1][0];
					snake[i][1] = snake[i-1][1];
				}
				// Move snake cell positions
				snake[0][0] += dx;
				snake[0][1] += dy;
				// Keeping the snake in the grid
				if(snake[0][0] >= gridSize) snake[0][0] = 0;
				if(snake[0][1] >= gridSize) snake[0][1] = 0;
				if(snake[0][0] < 0)         snake[0][0] = gridSize-1;
				if(snake[0][1] < 0)         snake[0][1] = gridSize-1;
				
				// Checking for collisions within the snake
				for(unsigned int i = 1; i < snakeLength; i++){
					for(unsigned int j = i+1; j < snakeLength; j++){
						if((snake[i][0] == snake[j][0]) && (snake[i][1] == snake[j][1])){
							score = 0;
							snakeLength = initialSnakeLength;
							paused = false;
							inGame = false;
						}
					}
				}
				
				// Check for "collision" with apple
				for(unsigned int i = 0; i < snakeLength; i++){
					if((snake[i][0] == apple[0]) && (snake[i][1] == apple[1])){
						score++;
						newApplePosition();
					}
				}
				
				framerateDevideCounter = 0;
			}
			
			// Draw the snake's cells
			for(unsigned int i = 0; i < snakeLength; i++){
				drawSolidBox(convertGridToX(snake[i][0]),
							 convertGridToY(snake[i][1]),
							 convertGridToX(snake[i][0]) + gridTileSize,
							 convertGridToY(snake[i][1]) + gridTileSize,
							 COLOR_GREEN);
				if(i == 0){
					drawSolidBox(convertGridToX(snake[i][0]),
								convertGridToY(snake[i][1]),
								convertGridToX(snake[i][0]) + gridTileSize,
								convertGridToY(snake[i][1]) + gridTileSize,
								COLOR_YELLOW);
				}
			}
			
			// Draw the apple
			drawSolidBox(convertGridToX(apple[0]),
						 convertGridToY(apple[1]),
						 convertGridToX(apple[0]) + gridTileSize,
						 convertGridToY(apple[1]) + gridTileSize,
						 COLOR_RED);
			
			// Draw a Grid
			for(unsigned int i = 0; i < gridSize; i++){
				for(unsigned int j = 0; j < gridSize; j++){
					drawBox(convertGridToX(i), convertGridToY(j), convertGridToX(i+1), convertGridToY(j+1), COLOR_WHITE);
				}
			}
			
			// Exit the application when the HOME button is pressed
			if(pressed & WPAD_BUTTON_HOME){
				paused = true;
				inGame = false;
			}
			
		}else if(paused){
			// Clear Frame Buffer
			VIDEO_ClearFrameBuffer(rmode, xfb, COLOR_GREEN);
			
			// Display pause message
			pointCursor(6, 10);
			printf("PAUSED");
			pointCursor(8, 10);
			printf("Press the A button to resume.");
			pointCursor(10, 10);
			printf("Press the B button to return to the main menu.");
			pointCursor(12, 10);
			printf("Press the HOME button to exit this application.");
			
			// Exit the application when the HOME button is pressed
			if(pressed & WPAD_BUTTON_HOME) exit(0);
			
			// Checking to return to the game
			if(pressed & WPAD_BUTTON_A){
				paused = false;
				inGame = true;
			}
			
			//Checking to go back to the main menu
			if(pressed & WPAD_BUTTON_B){
				initialiseSnake();
				paused = false;
			}
			
		}else{
			// Clear Frame Buffer
			VIDEO_ClearFrameBuffer(rmode, xfb, COLOR_GREEN);
		
			// Display welcome message
			pointCursor(6, 10);
			printf("Hey, this is a snake ripoff.");
			pointCursor(8, 10);
			printf("Please press the A button to start the game.");
			pointCursor(10, 10);
			printf("Please press the HOME button to exit this application.");
			
			// Exit the application when the HOME button is pressed
			if(pressed & WPAD_BUTTON_HOME) exit(0);
			
			// Checking to start the Game
			if(pressed & WPAD_BUTTON_A){
				initialiseSnake();
				newApplePosition();
				inGame = true;
			}
		}
		
		// Draw small box at WiiMote's Cursor position => always nice to have
		drawSolidBox(ir.x, ir.y, ir.x+3, ir.y+3, COLOR_WHITE);

		// Wait for the next frame
		VIDEO_WaitVSync();
	}

	return 0;
}
