# GL-Threads
C program involving multi-threading, mutex locks, and race conditions.

## The problem
In this assignment, you are going to implement “traveler” threads that move randomly (with some
constraints) on a rectangular grid. They roam randomly on the grid, but a traveler that reaches a
grid corner terminates. Travelers can be of three kinds, indicated by a color (red, green, or blue).
They will record a trail of their travel on the grid.

However, in this “nothing comes free” world, drawing a colorful trail requires acquiring ink
resources. A “red” traveler that plans to execute a 5-square long displacement on the grid needs
to acquired 5 units of red ink to draw its trail. There are limited-capacity tanks for each kind of
ink. When a tank is empty, the travelers of the corresponding color cannot move until the tank is
refilled again (maybe only partially). In the version of the code handed out, the tanks are refilled
when the user hits the appropriate key (‘r’, ‘g’, or ‘b’).

### Movement of the travelers
Travelers can only move vertically or horizontally on the grid. This means that in addition to a
position (row and column index) on the grid, there is also a current orientation of each traveler.
The initial position and orientation of the travelers are generated randomly on the grid.

At each iteration of its main loop, a traveler thread selects randomly a new displacement direction and displacement length. The new direction of displacement must be perpendicular to the
current direction. The length of the displacement must keep the traveler within the dimensions of
the grid (needless to say, being a length, it must be positive).

A traveler that reaches one of the corners of the grid terminates its execution. If you want to join
that thread in the main thread, for whatever reason, you can only do that after the glutMainLoop()
call at the end of the main function. The reason for this is that in a GLUT-driven program, the
handling of all events and interrupts is left over to GLUT. If you insert a pthread join call
(basically, a blocking call) anywhere, you basically block the graphic front end. Keyboard events
won’t be handled anymore and no more rendering will occur. There is no way around that as GLUT
must run on the main thread. It is a the price to pay for it being such a light-weight, portable, and
easy to use library.

### Color trails
A traveler going through a square will leave a trail of its designated color in that square. By now,
you are experts in the manipulation of color information, so it has not escaped to you that “red,”
“green,” and “blue” travelers don’t need to overwrite each other trails. They will simply write their
trail in the appropriate color channel of the grid square.

I defined the grid as a 2D array of int. You should have no problem accessing the color
channels of the grid square grid[i][j]. In fact, I give an example of how to do this, as a
certainly unnecessary reminder of the operation, in the initialization of the grid included in the
handout. Needless to say, this code should be replaced by the commented-out code that sets all
grid squares black.

### Traveler threads
You have to create at least eight traveler threads, assign them a random color among the three
available, an initial position on the grid, and an initial orientation. Use the data types defined in
gl frontEnd.h: TravelerType for the choice of ink color and TravelDirection for
the orientation.

Two travelers may occupy the same grid square. In other words, only the color information of
a grid square is considered a shared resource under race condition. The “space” within the grid
square is not under race condition for this simulation.

A single-threaded implementation (no traveler thread handling displacements and color trail)
will be penalized by 40 points. An implementation with a single thread of each type will be
penalized 20 points.

## Extra Credit

### Color levels
In the assignment as discussed so far, a traveler thread marked as “red” (resp. “blue” or “green”)
leaves a colorful trail by setting to 0xff its respective color channel. We can tell if a grid square
has been traversed by “red” and “green” travelers as opposed to “blue” travelers only because, in
the case of the former, the square will be colored yellow while, in the case of the latter, it we be
colored blue. There is, however, no way to tell how many of each type have gone through the
square.

In this enhanced version, you should increment the level of color rather than setting it directly
to 0xFF. Don’t increment by 1, since we won’t see any difference, rather by 16 or 32. Be careful
not to overflow the value of a color channel, as it cannot exceed 0xFF (that is, 255).

### Maintain and synchronize traveler info
In the main.c source file, some sections that have been commented out provide support for a data
type named TravelerInfo. An array of structs of this type can be passed to the graphic front
end so that the current position and orientation of the travelers is now shown on the grid.

You need to properly initialize and maintain the values of the different fields of this array.
Naturally, because there is the possibility of race condition on this data, you must synchronize
access.

### Add ink producing threads
In the version of the program handed out, the user can partially refill a ink tank by hitting the
appropriate key (‘r’, ‘g’, or ‘b’). We would like to automatize the process and instead have
threads in charge of refilling the tanks. Such threads would run an infinite loop of

• Sleep for a while (simulates time required to “produce” ink;

• Gain access to the appropriate ink tank;

• Add ink (don’t overfill);

If you look carefully at the code of main.c, there are variables and functions already defined that
control the size of a refill and the sleep time of a producer. Don’t rename any of these as this would
break code in the graphic front end. The functions to increase or decrease the sleep time are called
when the user presses the keys ‘,’ and ‘.’ (below ‘<’ and ‘>’). To get the full 15 points of extra
credit, you must implement multiple producer threads for each ink color. If you only implement
one of each type, then the maximum amount of extra credit points is 10 points, and only 7 points
if a single thread takes care of the three colors.

### Synchronize access to the grid squares
Add a synchronization mechanism so that two travelers may not anymore occupy the same grid
square. We understand that this may lead to a deadlock, but you are not asked to detect and resolve,
or to prevent deadlocks.
