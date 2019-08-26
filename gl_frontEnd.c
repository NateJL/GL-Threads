//
//  gl_frontEnd.c
//  GL threads
//
//  Nathan Larson 2017-05-02

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#include "gl_frontEnd.h"

//---------------------------------------------------------------------------
//	ink access functions.
//---------------------------------------------------------------------------
int acquireRedInk(unsigned int theRed);
int acquireGreenInk(unsigned int theGreen);
int acquireBlueInk(unsigned int theBlue);
int refillRedInk(unsigned int theRed);
int refillGreenInk(unsigned int theGreen);
int refillBlueInk(unsigned int theBlue);

//---------------------------------------------------------------------------
//  Private functions' prototypes
//---------------------------------------------------------------------------

void myResize(int w, int h);
void drawnTankFrame(unsigned int LEVEL_WIDTH, unsigned int LEVEL_HEIGHT);
void fillTank(unsigned int y, unsigned int LEVEL_WIDTH);
void displayTextualInfo(const char* infoStr, int x, int y, int isLarge);
void myMouse(int b, int s, int x, int y);
void myGridPaneMouse(int b, int s, int x, int y);
void myStatePaneMouse(int b, int s, int x, int y);
void myKeyboard(unsigned char c, int x, int y);
void myMenuHandler(int value);
void mySubmenuHandler(int colorIndex);
void myTimer(int value);

// Producer sleep time access functions
void speedupProducers(void);
void slowdownProducers(void);

//---------------------------------------------------------------------------
//  Interface constants
//---------------------------------------------------------------------------
enum MenuItemID {	SEPARATOR = -1,
					//
					QUIT_MENU = 0,
					OTHER_MENU_ITEM
};

const char* MAIN_MENU_ITEM_STR[] = {	"Quit",			//	QUIT_MENU
										"Something"		//	OTHER_MENU_ITEM
};

enum FirstSubmenuItemID {	FIRST_SUBMENU_ITEM = 11,
							SECOND_SUBMENU_ITEM
};

#define SMALL_DISPLAY_FONT    GLUT_BITMAP_HELVETICA_12
#define LARGE_DISPLAY_FONT    GLUT_BITMAP_HELVETICA_18
const int SMALL_FONT_HEIGHT = 12;
const int LARGE_FONT_HEIGHT = 18;
const int TEXT_PADDING = 0;
const float kTextColor[4] = {1.f, 1.f, 1.f, 1.f};

//            kTextBackgroundColor[3] = {0.0f, 0.2f, 0.0f};



const int   INIT_WIN_X = 100,
            INIT_WIN_Y = 40;

const unsigned int GRID_PANE_WIDTH = 600;
const unsigned int GRID_PANE_HEIGHT = 600;
const unsigned int STATE_PANE_WIDTH = 400;
const unsigned int STATE_PANE_HEIGHT = 600;
const unsigned int H_PADDING = 0;
const unsigned int WINDOW_WIDTH = 1000;
const unsigned int WINDOW_HEIGHT = 600;

extern const unsigned int MAX_LEVEL;
extern const unsigned int MAX_ADD_INK;
extern const unsigned int MAX_NUM_TRAVELER_THREADS;

//---------------------------------------------------------------------------
//  File-level global variables
//---------------------------------------------------------------------------

void (*gridDisplayFunc)(void);
void (*stateDisplayFunc)(void);

//	I use a window split into two panes/subwindows.  The subwindows
//	will be accessed by an index.
const int	GRID_PANE = 0,
			STATE_PANE = 1;
int	gMainWindow,
	gSubwindow[2];

// extern variable to reference the list of mutex locks for the traveler info list of structs
extern pthread_mutex_t* travelerLocks;

//---------------------------------------------------------------------------
//	Drawing functions
//---------------------------------------------------------------------------


//	This is the function that does the actual grid drawing
void drawGrid(int**grid, unsigned int numRows, unsigned int numCols)
{	
	const float DH = (1.f* GRID_PANE_WIDTH) / numCols;
	const float DV = (1.f*GRID_PANE_HEIGHT) / numRows;

	//	Display the grid as a series of quad strips
	for (unsigned int i=0; i<numRows; i++)
	{
		glBegin(GL_QUAD_STRIP);
			for (unsigned int j=0; j<numCols; j++)
			{
				
				glColor4f((grid[i][j] & 0x000000FF)/255.f, ((grid[i][j] & 0x0000FF00) >> 8)/255.f,
						  ((grid[i][j] & 0x00FF0000) >> 16)/255.f, 1.f);

				glVertex2f(j*DH, i*DV);
				glVertex2f(j*DH, (i+1)*DV);
				glVertex2f((j+1)*DH, i*DV);
				glVertex2f((j+1)*DH, (i+1)*DV);
			}
		glEnd();
	}
	
	//	Then draw a grid of lines on top of the squares
	glColor4f(0.5f, 0.5f, 0.5f, 1.f);
	glBegin(GL_LINES);
		//	Horizontal
		for (unsigned int i=0; i<= numRows; i++)
		{
			glVertex2f(0, i*DV);
			glVertex2f(GRID_PANE_WIDTH, i*DV);
		}
		//	Vertical
		for (unsigned int j=0; j<= numCols; j++)
		{
			glVertex2f(j*DH, 0);
			glVertex2f(j*DH, GRID_PANE_HEIGHT);
		}
	glEnd();
}

void drawGridAndTravelers(int**grid, unsigned int numRows, unsigned int numCols, TravelerInfo* travelList)
{
	drawGrid(grid, numRows, numCols);
	
	const float DH = (1.f* GRID_PANE_WIDTH) / numCols;
	const float DV = (1.f*GRID_PANE_HEIGHT) / numRows;

	//	Draw the travelers
	for (unsigned int k=0; k< MAX_NUM_TRAVELER_THREADS; k++)
	{
		if (travelList[k].isLive)
		{
			pthread_mutex_lock(&travelerLocks[travelList[k].index]);		// acquire the corresponding traveler info lock
			glPushMatrix();
			glTranslatef((travelList[k].col + 0.5f)*DH, (travelList[k].row + 0.5f)*DV, 0.f);
			glRotatef(travelList[k].dir * 90.f, 0.f, 0.f, 1.f);
			glColor4f(0.f, 0.f, 0.f, 1.f);
			glBegin(GL_POLYGON);
				glVertex2f(DH/6.f, -DV/4.f);
				glVertex2f(0.f, DV/4.f);
				glVertex2f(-DH/6.f, -DV/4.f);
			glEnd();
			glColor4f(1.f, 1.f, 1.f, 1.f);
			glBegin(GL_LINE_LOOP);
				glVertex2f(DH/6.f, -DV/4.f);
				glVertex2f(0.f, DV/4.f);
				glVertex2f(-DH/6.f, -DV/4.f);
			glEnd();
			glPopMatrix();
			pthread_mutex_unlock(&travelerLocks[travelList[k].index]);		// release the corresponding traveler info lock
		}
	}
}


void drawnTankFrame(unsigned int LEVEL_WIDTH, unsigned int LEVEL_HEIGHT)
{
	glColor4f(0.5f, 0.5f, 0.5f, 1.f);
	glBegin(GL_LINE_LOOP);
		glVertex2i(0, 0);
		glVertex2i(LEVEL_WIDTH, 0);
		glVertex2i(LEVEL_WIDTH, LEVEL_HEIGHT);
		glVertex2i(0, LEVEL_HEIGHT);
	glEnd();
}

void fillTank(unsigned int y, unsigned int LEVEL_WIDTH)
{
	glBegin(GL_POLYGON);
		glVertex2i(0, 0);
		glVertex2i(LEVEL_WIDTH, 0);
		glVertex2i(LEVEL_WIDTH, y);
		glVertex2i(0, y);
	glEnd();
}


void displayTextualInfo(const char* infoStr, int xPos, int yPos, int isLarge)
{
    //-----------------------------------------------
    //  0.  get current material properties
    //-----------------------------------------------
    float oldAmb[4], oldDif[4], oldSpec[4], oldShiny;
    glGetMaterialfv(GL_FRONT, GL_AMBIENT, oldAmb);
    glGetMaterialfv(GL_FRONT, GL_DIFFUSE, oldDif);
    glGetMaterialfv(GL_FRONT, GL_SPECULAR, oldSpec);
    glGetMaterialfv(GL_FRONT, GL_SHININESS, &oldShiny);

    glPushMatrix();

    //-----------------------------------------------
    //  1.  Build the string to display <-- parameter
    //-----------------------------------------------
    unsigned long infoLn = strlen(infoStr);

    //-----------------------------------------------
    //  2.  Determine the string's length (in pixels)
    //-----------------------------------------------
    int textWidth = 0;
    for (unsigned long k=0; k<infoLn; k++)
	{
		if (isLarge)
			textWidth += glutBitmapWidth(LARGE_DISPLAY_FONT, infoStr[k]);
		else
			textWidth += glutBitmapWidth(SMALL_DISPLAY_FONT, infoStr[k]);
		
    }
	//  add a few pixels of padding
    textWidth += 2*TEXT_PADDING;
	
    //-----------------------------------------------
    //  4.  Draw the string
    //-----------------------------------------------    
    glColor4fv(kTextColor);
    int x = xPos;
    for (unsigned long k=0; k<infoLn; k++)
    {
        glRasterPos2i(x, yPos);
		if (isLarge)
		{
			glutBitmapCharacter(LARGE_DISPLAY_FONT, infoStr[k]);
			x += glutBitmapWidth(LARGE_DISPLAY_FONT, infoStr[k]);
		}
		else
		{
			glutBitmapCharacter(SMALL_DISPLAY_FONT, infoStr[k]);
			x += glutBitmapWidth(SMALL_DISPLAY_FONT, infoStr[k]);
		}
	}

    //-----------------------------------------------
    //  5.  Restore old material properties
    //-----------------------------------------------
	glMaterialfv(GL_FRONT, GL_AMBIENT, oldAmb);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, oldDif);
	glMaterialfv(GL_FRONT, GL_SPECULAR, oldSpec);
	glMaterialf(GL_FRONT, GL_SHININESS, oldShiny);  
    
    //-----------------------------------------------
    //  6.  Restore reference frame
    //-----------------------------------------------
    glPopMatrix();
}



void drawState(unsigned int numLiveThreads, unsigned int redLevel, unsigned int greenLevel, 
			   unsigned int blueLevel, unsigned int producerSleepTime)
{
	//	I compute once the dimensions for all the rendering of my state info
	unsigned int LEVEL_WIDTH = STATE_PANE_WIDTH / 4;
	unsigned int LEVEL_HEIGHT = STATE_PANE_HEIGHT / 3;
	unsigned int LEVEL_BOTTOM = STATE_PANE_HEIGHT / 8;
	unsigned int H_PAD = LEVEL_WIDTH / 4;
	unsigned int RED_LEFT = H_PAD;
	unsigned int GREEN_LEFT = RED_LEFT + LEVEL_WIDTH + H_PAD;
	unsigned int BLUE_LEFT = STATE_PANE_WIDTH - LEVEL_WIDTH - H_PAD;
	unsigned int LEVEL_TXT_Y = LEVEL_BOTTOM / 2;
	unsigned int MAX_LEVEL_TXT_Y = LEVEL_BOTTOM + LEVEL_HEIGHT +  LEVEL_BOTTOM / 4;
	unsigned int TOP_LEVEL_TXT_Y = 4*STATE_PANE_HEIGHT / 5;

	
	//	Draw the red level tank
	glPushMatrix();
	glTranslatef(RED_LEFT, LEVEL_BOTTOM, 0);
	glColor4f(1.f, 0.f, 0.f, 1.f);
	const unsigned int yRed = (redLevel * LEVEL_HEIGHT) / MAX_LEVEL;
	fillTank(yRed, LEVEL_WIDTH);
	drawnTankFrame(LEVEL_WIDTH, LEVEL_HEIGHT);
	glPopMatrix();

	//	Draw the green level tank
	glPushMatrix();
	glTranslatef(GREEN_LEFT, LEVEL_BOTTOM, 0);
	glColor4f(0.f, 1.f, 0.f, 1.f);
	const unsigned int yGreen = (greenLevel * LEVEL_HEIGHT) / MAX_LEVEL;
	fillTank(yGreen, LEVEL_WIDTH);
	drawnTankFrame(LEVEL_WIDTH, LEVEL_HEIGHT);
	glPopMatrix();

	//	Draw the blue level tank
	glPushMatrix();
	glTranslatef(BLUE_LEFT, LEVEL_BOTTOM, 0);
	glColor4f(0.f, 0.f, 1.f, 1.f);
	const unsigned int yBlue = (blueLevel * LEVEL_HEIGHT) / MAX_LEVEL;
	fillTank(yBlue, LEVEL_WIDTH);
	drawnTankFrame(LEVEL_WIDTH, LEVEL_HEIGHT);
	glPopMatrix();
	
	//	Build, then display text info for the red, green, and blue tanks
	char infoStr[256];
	sprintf(infoStr, "Red level: %d", redLevel);
	displayTextualInfo(infoStr, RED_LEFT, LEVEL_TXT_Y, 0);
	sprintf(infoStr, "Green level: %d", greenLevel);
	displayTextualInfo(infoStr, GREEN_LEFT, LEVEL_TXT_Y, 0);
	sprintf(infoStr, "Blue level: %d", blueLevel);
	displayTextualInfo(infoStr, BLUE_LEFT, LEVEL_TXT_Y, 0);
	sprintf(infoStr, "Max level: %d", MAX_LEVEL);
	displayTextualInfo(infoStr, RED_LEFT, MAX_LEVEL_TXT_Y, 0);
	displayTextualInfo(infoStr, GREEN_LEFT, MAX_LEVEL_TXT_Y, 0);
	displayTextualInfo(infoStr, BLUE_LEFT, MAX_LEVEL_TXT_Y, 0);
	
	//	display info about number of live traveler threads
	sprintf(infoStr, "Traveler Threads: %d", numLiveThreads);
	displayTextualInfo(infoStr, RED_LEFT, TOP_LEVEL_TXT_Y, 1);

	// display info about the producer sleep time
	// I added these lines to help visualize the users input modifying the producer sleep time
	sprintf(infoStr, "Producer Sleep Time: %d mu", producerSleepTime);
	displayTextualInfo(infoStr, RED_LEFT, TOP_LEVEL_TXT_Y - 50, 1);
}


//	This callback function is called when the window is resized
//	(generally by the user of the application).
//	It is also called when the window is created, why I placed there the
//	code to set up the virtual camera for this application.
//
void myResize(int w, int h)
{
	if ((w != WINDOW_WIDTH) || (h != WINDOW_HEIGHT))
	{
		glutReshapeWindow(WINDOW_WIDTH, WINDOW_HEIGHT);
	}
	else
	{
		glutPostRedisplay();
	}
}


void myDisplay(void)
{
    glutSetWindow(gMainWindow);

    glMatrixMode(GL_MODELVIEW);
    glClear(GL_COLOR_BUFFER_BIT);
    glutSwapBuffers();

	gridDisplayFunc();
	stateDisplayFunc();
	
    glutSetWindow(gMainWindow);	
}

//	This function is called when a mouse event occurs just in the tiny
//	space between the two subwindows.
//
void myMouse(int button, int state, int x, int y)
{
	glutSetWindow(gMainWindow);
	glutPostRedisplay();
}

//	This function is called when a mouse event occurs in the grid pane
//
void myGridPaneMouse(int button, int state, int x, int y)
{
	switch (button)
	{
		case GLUT_LEFT_BUTTON:
			if (state == GLUT_DOWN)
			{
				//	do something
			}
			else if (state == GLUT_UP)
			{
				//	exit(0);
			}
			break;
			
		default:
			break;
	}

	glutSetWindow(gMainWindow);
	glutPostRedisplay();
}

//	This function is called when a mouse event occurs in the state pane
void myStatePaneMouse(int button, int state, int x, int y)
{
	switch (button)
	{
		case GLUT_LEFT_BUTTON:
			if (state == GLUT_DOWN)
			{
				//	do something
			}
			else if (state == GLUT_UP)
			{
				//	exit(0);
			}
			break;
			
		default:
			break;
	}

	glutSetWindow(gMainWindow);
	glutPostRedisplay();
}


//	This callback function is called when a keyboard event occurs
//
void myKeyboard(unsigned char c, int x, int y)
{
	int ok = 0;
	
	switch (c)
	{
		//	'esc' to quit
		case 27:
			exit(0);
			break;

		//	Test red ink up/down
		case 'r':
			ok = refillRedInk(MAX_ADD_INK);
			break;

		//	Test green ink up/down
		case 'g':
			ok = refillGreenInk(MAX_ADD_INK);
			break;

		//	Test blue ink up/down
		case 'b':
			ok = refillBlueInk(MAX_ADD_INK);
			break;

		case ',':
			slowdownProducers();
			break;

		case '.':
			speedupProducers();
			break;

		default:
			ok = 1;
			break;
	}
	if (!ok)
	{
		//	do something?
	}
	
	glutSetWindow(gMainWindow);
	glutPostRedisplay();
}


void myTimer(int value)
{
	//	value not used.  Warning suppression
	(void) value;

    myDisplay();
    
	//	And finally I perform the rendering
	glutTimerFunc(20, myTimer, 0);
}

void myMenuHandler(int choice)
{
	switch (choice)
	{
		//	Exit/Quit
		case QUIT_MENU:
			exit(0);
			break;
		
		//	Do something
		case OTHER_MENU_ITEM:
			break;
		
		default:	//	this should not happen
			break;
	
	}

    glutPostRedisplay();
}


void initializeFrontEnd(int argc, char** argv, void (*gridDisplayCB)(void),
						void (*stateDisplayCB)(void))
{
	//	Initialize glut and create a new window
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);


	glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
	glutInitWindowPosition(INIT_WIN_X, INIT_WIN_Y);
	gMainWindow = glutCreateWindow("Colorful Trails -- CSC 412 - Spring 2017");
	glClearColor(0.2f, 0.2f, 0.2f, 1.f);
	
	//	set up the callbacks for the main window
	glutDisplayFunc(myDisplay);
	glutReshapeFunc(myResize);
	glutKeyboardFunc(myKeyboard);
	glutMouseFunc(myMouse);
	glutTimerFunc(20, myTimer, 0);
	
	gridDisplayFunc = gridDisplayCB;
	stateDisplayFunc = stateDisplayCB;
	
	//	create the two panes as glut subwindows
	gSubwindow[GRID_PANE] = glutCreateSubWindow(gMainWindow,
												0, 0,							//	origin
												GRID_PANE_WIDTH, GRID_PANE_HEIGHT);
    glViewport(0, 0, GRID_PANE_WIDTH, GRID_PANE_HEIGHT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
	glOrtho(0.0f, GRID_PANE_WIDTH, 0.0f, GRID_PANE_HEIGHT, -1, 1);
	glClearColor(0.f, 0.f, 0.f, 1.f);
	glutKeyboardFunc(myKeyboard);
	glutMouseFunc(myGridPaneMouse);
	glutDisplayFunc(gridDisplayCB);
	
	
	glutSetWindow(gMainWindow);
	gSubwindow[STATE_PANE] = glutCreateSubWindow(gMainWindow,
												GRID_PANE_WIDTH + H_PADDING, 0,	//	origin
												STATE_PANE_WIDTH, STATE_PANE_HEIGHT);
    glViewport(0, 0, STATE_PANE_WIDTH, STATE_PANE_WIDTH);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
	glOrtho(0.0f, STATE_PANE_WIDTH, 0.0f, STATE_PANE_HEIGHT, -1, 1);
	glClearColor(0.f, 0.f, 0.f, 1.f);
	glutKeyboardFunc(myKeyboard);
	glutMouseFunc(myGridPaneMouse);
	glutDisplayFunc(stateDisplayCB);
}
