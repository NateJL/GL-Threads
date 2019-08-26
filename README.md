# GL-Threads
C program involving multi-threading, mutex locks, and race conditions.

<b>The problem</b><br>
In this assignment, you are going to implement “traveler” threads that move randomly (with some
constraints) on a rectangular grid. They roam randomly on the grid, but a traveler that reaches a
grid corner terminates. Travelers can be of three kinds, indicated by a color (red, green, or blue).
They will record a trail of their travel on the grid.<br>
However, in this “nothing comes free” world, drawing a colorful trail requires acquiring ink
resources. A “red” traveler that plans to execute a 5-square long displacement on the grid needs
to acquired 5 units of red ink to draw its trail. There are limited-capacity tanks for each kind of
ink. When a tank is empty, the travelers of the corresponding color cannot move until the tank is
refilled again (maybe only partially). In the version of the code handed out, the tanks are refilled
when the user hits the appropriate key (‘r’, ‘g’, or ‘b’).
