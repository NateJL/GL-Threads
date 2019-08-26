//
//  main.c
//  GL threads
//
//  Nathan Larson 2017-05-02

 /*-------------------------------------------------------------------------+
 |	A graphic front end for a grid+state simulation.						|
 |																			|
 |	This application simply creates a glut window with a pane to display	|
 |	a colored grid and the other to display some state information.			|
 |	Sets up callback functions to handle menu, mouse and keyboard events.	|
 |	Normally, you shouldn't have to touch anything in this code, unless		|
 |	you want to change some of the things displayed, add menus, etc.		|
 |																			|
 |	Current GUI:															|
 |		- 'ESC' --> exit the application									|
 |		- 'r' --> add red ink												|
 |		- 'g' --> add green ink												|
 |		- 'b' --> add blue ink												|
 +-------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

#include "gl_frontEnd.h"

//==================================================================================
//	Function prototypes
//==================================================================================
void displayGridPane(void);
void displayStatePane(void);
void initializeApplication(void);

//==================================================================================
//	Thread Function prototypes & locks
//==================================================================================
void* travelerThread(void*);
void* producerThread(void*);

// function prototype for the moveTraveler function, used to handle traveler movement and coloring
void moveTraveler(TravelerInfo* info);

// mutex locks for access to red ink tank, green ink tank, and blue ink tank
pthread_mutex_t redInkLock;
pthread_mutex_t greenInkLock;
pthread_mutex_t blueInkLock;


//==================================================================================
//	Application-level global variables
//==================================================================================

//	Don't touch
//----------------
extern const int GRID_PANE, STATE_PANE;
extern int	gMainWindow, gSubwindow[2];

// Array of locks to control access to each of the travelerInfo structs
pthread_mutex_t* travelerLocks;

//	The state grid and its dimensions
int** grid;
const unsigned int NUM_ROWS = 32, NUM_COLS = 30;

// 2D array of locks to control access to each of the grid squares in the grid
pthread_mutex_t** gridLocks;

// the max number of traveler threads to initialize
const unsigned int MAX_NUM_TRAVELER_THREADS = 8;

//the number of live threads (that haven't terminated yet)
unsigned int numLiveThreads = 0;

//	the ink levels
const unsigned int MAX_LEVEL = 50;
const unsigned int MAX_ADD_INK = 10;
const unsigned int TOTAL_INK_PRODUCER_THREADS = 6;		// MUST BE MULTIPLE OF 3 OR PROGRAM WILL NOT RUN
unsigned int redLevel = 20, greenLevel = 10, blueLevel = 40;

//	ink producer sleep time (in microseconds)
const unsigned int MIN_SLEEP_TIME = 1000;
unsigned int producerSleepTime = 100000;

// Array of TravelerInfo structs to store traveler thread information
TravelerInfo *travelList;

// Array of producerInfo structs to store the producer thread information
ProducerInfo *producerList;

void displayGridPane(void)
{
	//	This is OpenGL/glut magic.
	glutSetWindow(gSubwindow[GRID_PANE]);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	//---------------------------------------------------------
	//	This is the call that makes OpenGL render the grid.
	//
	//	You *must* synchronize this call.
	//---------------------------------------------------------
	drawGridAndTravelers(grid, NUM_ROWS, NUM_COLS, travelList);
	
	//	This is OpenGL/glut magic.
	glutSwapBuffers();
	
	glutSetWindow(gMainWindow);
}

void displayStatePane(void)
{
	//	This is OpenGL/glut magic.
	glutSetWindow(gSubwindow[STATE_PANE]);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	//---------------------------------------------------------
	//	This is the call that makes OpenGL render information
	//	about the state of the simulation.
	//
	//	You *must* synchronize this call (probably inside the function)
	//---------------------------------------------------------
	drawState(numLiveThreads, redLevel, greenLevel, blueLevel, producerSleepTime);
		
	//	This is OpenGL/glut magic.
	glutSwapBuffers();
	
	glutSetWindow(gMainWindow);
}

//------------------------------------------------------------------------
//	These are the functions that would be called by a traveler thread in
//	order to acquire red/green/blue ink to trace its trail.
//	You *must* synchronized access to the ink levels
//------------------------------------------------------------------------
//
int acquireRedInk(unsigned int theRed)
{
	int ok = 0;
	if (redLevel >= theRed)
	{
		redLevel -= theRed;
		ok = 1;
	}
	return ok;
}

int acquireGreenInk(unsigned int theGreen)
{
	int ok = 0;
	if (greenLevel >= theGreen)
	{
		greenLevel -= theGreen;
		ok = 1;
	}
	return ok;
}

int acquireBlueInk(unsigned int theBlue)
{
	int ok = 0;
	if (blueLevel >= theBlue)
	{
		blueLevel -= theBlue;
		ok = 1;
	}
	return ok;
}

//------------------------------------------------------------------------
//	These are the functions that would be called by a producer thread in
//	order to refill the red/green/blue ink tanks.
//	You *must* synchronized access to the ink levels
//------------------------------------------------------------------------
//

/*
 * Refill the amount of "red ink"
 */
int refillRedInk(unsigned int theRed)
{
	int ok = 0;
	if (redLevel + theRed <= MAX_LEVEL)
	{
		redLevel += theRed;
		ok = 1;
	}
	return ok;
}

/*
 * Refill the amount of "green ink"
 */
int refillGreenInk(unsigned int theGreen)
{
	int ok = 0;
	if (greenLevel + theGreen <= MAX_LEVEL)
	{
		greenLevel += theGreen;
		ok = 1;
	}
	return ok;
}

/*
 * Refill the amount of "blue ink"
 */
int refillBlueInk(unsigned int theBlue)
{
	int ok = 0;
	if (blueLevel + theBlue <= MAX_LEVEL)
	{
		blueLevel += theBlue;
		ok = 1;
	}
	return ok;
}

/*
 * Speed up production of ink
 */
void speedupProducers(void)
{
	//	decrease sleep time by 20%, but don't get too small
	unsigned int newSleepTime = (8 * producerSleepTime) / 10;
	
	if (newSleepTime > MIN_SLEEP_TIME)
	{
		producerSleepTime = newSleepTime;
	}
}

/*
 * Slow down production of ink
 */
void slowdownProducers(void)
{
	//	increase sleep time by 20%
	producerSleepTime = (12 * producerSleepTime) / 10;
}

/*
 * This function acts as the main function for each of the traveler threads that control 
 * how the traveler acts and calculates various values.
 */
void* travelerThread(void* arg)
{
	TravelerInfo* info = (TravelerInfo *) arg;

	// when the traveler first spawns, acquire the current grid square lock
	pthread_mutex_lock(&gridLocks[info->row][info->col]);
	
	// main while loop, run while the traveler is still alive
	while(info->isLive)
	{
		// get a random direction perpendicular to current direction
		if(info->dir == NORTH || info->dir == SOUTH)	// if direction is north or south
		{
			int temp = rand() % 2;		// calculate random number out of 2
			if(temp)
				info->dir = EAST;		// if 1, face east
			else
				info->dir = WEST;		// else face west
		}
		else						// else if the direction is east or west
		{
			int temp = rand() % 2;		// calculate random number out of 2
			if(temp)
				info->dir = NORTH;		// if 1, face north
			else
				info->dir = SOUTH;		// else face south
		}

		// calculate distance from available grid elements
		int distance = 0;
		if(info->dir == NORTH)	// if facing north
		{
			distance = rand() % (NUM_ROWS - info->row);	// calculate random distance within current row to max row
			
		}
		else if(info->dir == SOUTH)		// else if facing south
		{
			distance = rand() % info->row;			// calculate random distance from 0 to current row
		}
		else if(info->dir == EAST)		// else if facing east
		{
			distance = rand() % (NUM_COLS - info->col);	// calculate random distance within current column to max column
		}
		else if(info->dir == WEST)		// else if facing west
		{
			distance = rand() % info->col;			// calculate random distance from 0 to current column
		}

		// check if the resources are available
		int hasResources = 0;
		if(info->type == RED_TRAV)					// if type is red
		{
			pthread_mutex_lock(&redInkLock);			// acquire red ink lock
			hasResources = acquireRedInk(distance);		// try to get enough ink to travel distance
			pthread_mutex_unlock(&redInkLock);			// release red ink lock
		}
		else if(info->type == GREEN_TRAV)			// else if the type is green
		{
			pthread_mutex_lock(&greenInkLock);			// acquire green ink lock
			hasResources = acquireGreenInk(distance);	// try to get enough ink to travel distance
			pthread_mutex_unlock(&greenInkLock);		// release green ink lock
		}
		else if(info->type == BLUE_TRAV)			// else if the type is blue
		{
			pthread_mutex_lock(&blueInkLock);			// aquire blue ink lock
			hasResources = acquireBlueInk(distance);	// try to get enough ink to travel distance
			pthread_mutex_unlock(&blueInkLock);			// release blue ink lock
		}

		// if resources are available, loop through grid and travel distance, leaving trail of color
		if(hasResources)
		{
			for(int i = 0; i < distance; i++)	// for loop, looping for each square in the distance
			{
				moveTraveler(info);		// call function to move the traveler
				
				usleep(100000);			// sleep for some amount of time (to make display easier to read)

				if(!info->isLive)	// if the traveler is not live (reached corner space)
				{
					break;		// then break from the main while loop
				}
			}
		}
	}
	numLiveThreads -= 1;	// after breaking from loop, decrement the number of live threads,
	return NULL;			// since this thread will be terminating.
}

/*
 * This function is used by the traveler threads to execute the movement of the traveler by:
 * 		1.) alter the color of the current grid square based off of the traveler type
 *		2.) Based off the orientation, attempt to acquire the next grid square lock before releasing the current/previous
 *		3.) if the traveler is located at one of the corner squares, set isLive to false (0)
 */
void moveTraveler(TravelerInfo* info)
{
	// amount to increment color by, 64 seemed to be the best choice for visual pleasure
	unsigned char newColor = 64;

	unsigned int red = ((grid[info->row][info->col]) & 0xFF); 			 // Extract the RR byte
    unsigned int green = ((grid[info->row][info->col] >> 8) & 0xFF);  	 // Extract the GG byte
  	unsigned int blue = ((grid[info->row][info->col] >> 16) & 0xFF);     // Extract the BB byte
  	
	if(info->type == RED_TRAV)			// if the traveler type is red
	{
		red += newColor;				// increment the red color value
		if(red > 255)					// if the new red value is greater than 255
			red = 255;					// set the red value to 255 (max)
	}
	else if(info->type == GREEN_TRAV)	// if the traveler type is green
	{
		green += newColor;				// increment the green color value
		if(green > 255)					// if the new green value is greater than 255
			green = 255;				// set the green value to 255 (max)
	}
	else if(info->type == BLUE_TRAV)	// else if the traveler type is blue
	{
		blue += newColor;				// increment the blue color value
		if(blue > 255)					// if the new blue value is greater than 255
			blue = 255;					// set the blue value to 255 (max)
	}

	// take the new calculated values and set them to the current grid value
	grid[info->row][info->col] = 0xFF000000 | (blue << 16) | (green << 8) | red;
	

	if(info->dir == NORTH)			// if the current orientation is north
	{
		pthread_mutex_lock(&gridLocks[info->row + 1][info->col]);	// try to acquire the next grid square lock
		pthread_mutex_unlock(&gridLocks[info->row][info->col]);		// release the current/previous grid square lock
		pthread_mutex_lock(&travelerLocks[info->index]);		// try to acquire the traveler info lock for the corresponding traveler
		info->row += 1;												// increment row by 1
		pthread_mutex_unlock(&travelerLocks[info->index]);			// release the traveler info lock
	}
	else if(info->dir == SOUTH)		// if the current orientation is south
	{
		pthread_mutex_lock(&gridLocks[info->row - 1][info->col]);	// try to acquire the next grid square lock
		pthread_mutex_unlock(&gridLocks[info->row][info->col]);		// release the current/previous grid square lock
		pthread_mutex_lock(&travelerLocks[info->index]);		// try to acquire the traveler info lock for the corresponding traveler
		info->row -= 1;												// increment row by 1
		pthread_mutex_unlock(&travelerLocks[info->index]);			// release the traveler info lock
	}
	else if(info->dir == EAST)		// if the current orientation is east
	{
		pthread_mutex_lock(&gridLocks[info->row][info->col + 1]);	// try to acquire the next grid square lock
		pthread_mutex_unlock(&gridLocks[info->row][info->col]);		// release the current/previous grid square lock
		pthread_mutex_lock(&travelerLocks[info->index]);		// try to acquire the traveler info lock for the corresponding traveler
		info->col += 1;												// increment row by 1
		pthread_mutex_unlock(&travelerLocks[info->index]);			// release the traveler info lock
	}
	else if(info->dir == WEST)		// if the current orientation is west
	{
		pthread_mutex_lock(&gridLocks[info->row][info->col - 1]);	// try to acquire the next grid square lock
		pthread_mutex_unlock(&gridLocks[info->row][info->col]);		// release the current/previous grid square lock
		pthread_mutex_lock(&travelerLocks[info->index]);		// try to acquire the traveler info lock for the corresponding traveler
		info->col -= 1;												// increment row by 1
		pthread_mutex_unlock(&travelerLocks[info->index]);			// release the traveler info lock
	}

	// if statement to check if the traveler is in one of the corner squares of the grid
	if(((info->row == 0) && ((info->col == 0) || (info->col == (NUM_COLS-1)))) ||
		((info->row == (NUM_ROWS-1) && ((info->col == 0) || (info->col == (NUM_COLS-1))))))
	{
		info->isLive = 0;		// if it is, then set isLive value to 0 (false)
	}
}

/*
 * This function acts as the main function for each of the producer threads, handling the lock acquiring, releasing,
 * and refilling of the ink tanks between sleep cycles.
 */
void* producerThread(void* arg)
{
	ProducerInfo* info = (ProducerInfo *) arg;

	// main while loop, no break since it will keep filling as long as the program is running
	while(1)
	{
		if(info->type == RED_INK)			// if the ink type is red
		{
			pthread_mutex_lock(&redInkLock);	// acquire the red ink lock
			refillRedInk(MAX_ADD_INK);			// attempt to fill MAX_ADD_INK into the ink tank
			pthread_mutex_unlock(&redInkLock);	// release the red ink lock
		}
		else if(info->type == GREEN_INK)	// if the ink type is green
		{
			pthread_mutex_lock(&greenInkLock);	// acquire the green ink lock
			refillGreenInk(MAX_ADD_INK);		// attempt to fill MAX_ADD_INK into the ink tank
			pthread_mutex_unlock(&greenInkLock);// release the green ink lock
		}
		else if(info->type == BLUE_INK)
		{
			pthread_mutex_lock(&blueInkLock);	// acquire the blue ink lock
			refillBlueInk(MAX_ADD_INK);			// attempt to fill MAX_ADD_INK into the ink tank
			pthread_mutex_unlock(&blueInkLock);	// release the blue ink lock
		}
		usleep(producerSleepTime);	// sleep for the given value of producerSleepTime, altered by user input
	}
	return NULL;
}


/*
 * Main function
 */
int main(int argc, char** argv)
{
	// if statement to check and make sure that the number of ink producing threads is a multiple of 3.
	// this check is performed since for my implementation, the number of threads per color is equal across all colors.
	// ie. TOTAL_INK_PRODUCER_THREADS = 9 would result in 3 producer threads per color
	if(TOTAL_INK_PRODUCER_THREADS%3 != 0)
	{
		printf("Total Ink count must be multiple of 3!");
		exit(0);
	}

	initializeFrontEnd(argc, argv, displayGridPane, displayStatePane);
	
	//	Now we can do application-level
	initializeApplication();

	// declare mutex lock for the ink resources
	pthread_mutex_init(&redInkLock, NULL);
	pthread_mutex_init(&greenInkLock, NULL);
	pthread_mutex_init(&blueInkLock, NULL);

	// declare errCode value to store the return value of pthread_create
	int errCode;

	// for loop to run through the max number of traveler threads and create a thread for each one
	for(int i = 0; i < MAX_NUM_TRAVELER_THREADS; i++)
	{
		// create a pthread, sending the travelerThread function to run and corresponding travelList struct reference
		errCode = pthread_create(&travelList[i].threadID, NULL, travelerThread, &travelList[i]);

		// increment the number of live threads
		numLiveThreads++;

		// if the errCode is nonzero, then the pthread was not created. print error and exit
		if(errCode != 0)
		{
			printf ("could not pthread_create thread %d. %d\n",
					 i, errCode);
			exit(0);
		}
	}

	// foor loop to run through the total number of ink producer threads and create thread for each one
	for(int i = 0; i < TOTAL_INK_PRODUCER_THREADS; i++)
	{
		// create a pthread, sending producerThread function to run and corresponding producerList struct reference
		errCode = pthread_create(&producerList[i].threadID, NULL, producerThread, &producerList[i]);

		// I choose to not increment the number of live threads in order to use that variable to represent the number of live travelers
		//numLiveThreads++;

		// if the errCode is nonzero, then the pthread was not created. print error and exit
		if(errCode != 0)
		{
			printf ("could not pthread_create thread %d. %d\n",
					 i, errCode);
			exit(0);
		}
	}

	//	Now we enter the main loop of the program and to a large extend
	//	"lose control" over its execution.  The callback functions that 
	//	we set up earlier will be called when the corresponding event
	//	occurs
	glutMainLoop();
	
	//	Free allocated resource before leaving (not absolutely needed, but
	//	just nicer.  Also, if you crash there, you know something is wrong
	//	in your code.
	for (unsigned int i=0; i< NUM_ROWS; i++)
		free(grid[i]);
	free(grid);

	// free 2D array of gridlocks
	for (unsigned int i=0; i<NUM_ROWS; i++)
		free(gridLocks[i]);
	free(gridLocks);
	
	// free the travelerInfo array, producerInfo array, and array of traveler locks
	free(travelList);
	free(producerList);
	free(travelerLocks);
	
	//	This will never be executed (the exit point will be in one of the
	//	call back functions).
	return 0;
}


/*
 * Function to initialize the program,
 *		-Allocating required memory
 *		-Seeding "random" number generator
 *		-Initializing mutex locks
 */
void initializeApplication(void)
{
	//	Allocate the grid
	grid = (int**) malloc(NUM_ROWS * sizeof(int*));
	for (unsigned int i=0; i<NUM_ROWS; i++)
		grid[i] = (int*) malloc(NUM_COLS * sizeof(int));

	// Allocate the grid locks
	gridLocks = (pthread_mutex_t**) malloc(NUM_ROWS * sizeof(pthread_mutex_t*));
	for (unsigned int i=0; i<NUM_ROWS; i++)
		gridLocks[i] = (pthread_mutex_t*) malloc(NUM_COLS * sizeof(pthread_mutex_t));

	// Allocate the traveler info locks
	travelerLocks = (pthread_mutex_t*) malloc(MAX_NUM_TRAVELER_THREADS * sizeof(pthread_mutex_t));

	
	//	seed the pseudo-random generator
	srand((unsigned int) time(NULL));
	
	//	create RGB values (and alpha  = 255) for each pixel
	//	A color is stored on 4 bytes ARGB.  However, because Intel (and compatible)
	//	CPUs are small-endian, the order of bytes for int, float, double, etc. is
	//	inverted.  So if we look at the int (4 bytes) storing 
	for (unsigned int i=0; i<NUM_ROWS; i++)
	{
		for (unsigned int j=0; j<NUM_COLS; j++)
		{
			grid[i][j] = 0xFF000000;
		}
	}

	// initialize the grid locks
	for(unsigned int i=0; i<NUM_ROWS; i++)
	{
		for(unsigned int j=0; j<NUM_COLS; j++)
		{
			pthread_mutex_init(&gridLocks[i][j], NULL);
		}
	}

	// initialize the traveler info locks
	for(unsigned int i=0; i<MAX_NUM_TRAVELER_THREADS; i++)
	{
		pthread_mutex_init(&travelerLocks[i], NULL);
	}

	
	// Allocate space for the array of travelerInfo structs
	travelList = (TravelerInfo*) malloc(MAX_NUM_TRAVELER_THREADS * sizeof(TravelerInfo));

	// Loop through each of the travelerInfo structs in the list and initialize the values
	for (unsigned int k=0; k< MAX_NUM_TRAVELER_THREADS; k++)
	{
		travelList[k].type = rand() % NUM_TRAV_TYPES;
		travelList[k].row = (rand() % (NUM_ROWS-1)) + 1;
		travelList[k].col = (rand() % (NUM_COLS-1)) + 1;
		travelList[k].dir = rand() % NUM_TRAVEL_DIRECTIONS;
		travelList[k].isLive = (unsigned  char) 1;
		travelList[k].index = k;
	}

	// Allocate space for the array of producerInfo structs
	producerList = (ProducerInfo*) malloc(TOTAL_INK_PRODUCER_THREADS * sizeof(ProducerInfo));

	// Loop through each of the producerInfo structs in the list and initialize the values
	for(unsigned int k=0; k< TOTAL_INK_PRODUCER_THREADS; k++)
	{
		// set the type of the (regardless of number of producers, will either be 0, 1, or 2)
		producerList[k].type = k%NUM_PRODUCER_TYPES;
	}
}


