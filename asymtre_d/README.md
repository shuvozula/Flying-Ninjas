This is tuned for the site clearing task, with task allocation added, and its
for physical robots.

navigation reasoning is included.
boxes of two different sizes are included (clear1 and clear2).
------------
The robot team is composed of:
Bliz  = Pioneer 56: laser, sonar, camera, comm
Cappy = Pioneer 57: laser, camera, comm
Arno  = Pioneer 55: sonar, camera, comm

Starting position:

Bliz  (26.5, -3)
Cappy (28, -3)
Arno  (29.5, -3)

I didn't use Arno (55) in my final experiments.

source code:
------------
The robot control code: clear.cc clear.h, comm-robot.h
The auction: auctioneer.c, robotrader.c, func.c, communicate.c
The reasoner: and_or_tree.c, parse.c, reason.c, util.c, communicate.c
