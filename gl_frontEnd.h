//
//  gl_frontEnd.h
//  GL threads
//
//  Nathan Larson 2017-05-02

#ifndef GL_FRONT_END_H
#define GL_FRONT_END_H


//------------------------------------------------------------------------------
//	Find out whether we are on Linux or macOS (sorry, Windows people)
//	and load the OpenGL & glut headers.
//	For the macOS, lets us choose which glut to use
//------------------------------------------------------------------------------
#if (defined(__dest_os) && (__dest_os == __mac_os )) || \
	defined(__APPLE_CPP__) || defined(__APPLE_CC__)
	//	Either use the Apple-provided---but deprecated---glut
	//	or the third-party freeglut install
	#if 0
		#include <GLUT/GLUT.h>
	#else
		#include <GL/freeglut.h>
		#include <GL/gl.h>
	#endif
#elif defined(linux)
	#include <GL/glut.h>
#else
	#error unknown OS
#endif


//-----------------------------------------------------------------------------
//	Data types
//-----------------------------------------------------------------------------

//	Travel direction data type
//	Note that if you define a variable
//	TravelDirection dir = whatever;
//	you get the opposite directions from dir as (NUM_TRAVEL_DIRECTIONS - dir)
//	you get left turn from dir as (dir + 1) % NUM_TRAVEL_DIRECTIONS
typedef enum TravelDirection {
								NORTH = 0,
								WEST,
								SOUTH,
								EAST,
								//
								NUM_TRAVEL_DIRECTIONS
} TravelDirection;

//	The 
typedef enum TravelerType {
								RED_TRAV = 0,
								GREEN_TRAV,
								BLUE_TRAV,
								//
								NUM_TRAV_TYPES
} TravelerType;

//	Traveler info data type
typedef struct TravelerInfo {
								TravelerType type;
								//	location of the traveler
								unsigned int row;
								unsigned int col;
								//	in which direciton is the traveler going
								TravelDirection dir;
								// initialized to 1, set to 0 if terminates
								unsigned char isLive;

								pthread_t threadID;

								unsigned int index;
} TravelerInfo;

//
typedef enum ProducerType {
								RED_INK = 0,
								GREEN_INK,
								BLUE_INK,

								NUM_PRODUCER_TYPES
} ProducerType;

// Producer info data type
typedef struct ProducerInfo {
								ProducerType type;

								pthread_t threadID;

} ProducerInfo;


//-----------------------------------------------------------------------------
//	Function prototypes
//-----------------------------------------------------------------------------

void drawGrid(int**grid, unsigned int numRows, unsigned int numCols);
void drawGridAndTravelers(int**grid, unsigned int numRows, unsigned int numCols, TravelerInfo* travelList);
void drawState(unsigned int numLiveThreads, unsigned int redLevel, unsigned int greenLevel, unsigned int blueLevel, unsigned int producerSleepTime);
void initializeFrontEnd(int argc, char** argv, void (*gridCB)(void), void (*stateCB)(void));

#endif // GL_FRONT_END_H

