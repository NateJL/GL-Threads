# GL-Threads
C program involving multi-threading, mutex locks, and race conditions.

### The problem
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

