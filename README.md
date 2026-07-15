# ENIAC Simulator

This is a pulse-level simulator of the ENIAC.
The current state of the simulator covers all
of the functionality as originally constructed
at the University of Pennsylvania.
Future plans include implementing the enhancements
that were made at the Ballistics Research Lab
during its operational life.

The simulator is discussed in some detail in
[Simulating the ENIAC](http://ieeexplore.ieee.org/document/8326772/).
Further information on the ENIAC can be found
[here](http://cs.drexel.edu/~bls96).

## Build Instructions

The source code is divided into two main parts: the simulator core and
the graphical visualization support.
Source code for the simulator proper is in the `src` directory and that
for the graphical support is in the `vis` directory.
The graphical support is implemented as separate programs that run as
child processes under the simulator core.
With or without graphical support, the simulator provides a command-line
interface for control.

### Building the Core Simulator

Since the core is written in Go, you'll need the Go compiler installed.
With that, go into the `src` directory and issue the command:

> `go build -o eniac`

This will result in an executable called `eniac` that you can use to invoke
the simulator
Usually, things are run from the root of the repo, so if we want to run the prime
number sieve in the programs directory, we could use the command:

> `src/eniac sieve.e`

(Note that in some environments, you might have to use `./src/eniac`.)
The effect is that you will power up the machine with the configuration described
in the file `programs/sieve.e`, and
you will be greeted with a `>` prompt.
To begin the computation, you need to press the *Initiating Pulse Switch* on the
initiating unit.
In the simulator you do this with the `b i` command.
Doing so will cause the ENIAC to puch a series of cards, each holding a prime
number in the range of 3 to 2801.

### Building Graphical Support

At the present time, there are five different graphical support programs.
Which one(s) will be useful to you depends on the details of your environment.
They are as follows:

1. `eniactk`: the original graphical interface.
It requires that you have TCL/Tk installed on your system and that the program `wish`
be in your path.
It uses the graphics in the images directory.
As it is written in Go, you can compile it with the command `go build -o eniactk eniactk.go`
or if you have `make` on your system, you can `make eniactk`.

2. `eniacfp`: provides a first-person video game type of interface.
It uses the 3d model in the `obj` directory.
To use this, you must have the Irrlicht library and a C++ compiler installed.
It is built with the command `make eniacfp`.

3. `eniac3d`: very similar to `eniacfp` except that it renders the view from both
eyes and presents them both.
It's only useful if you have a video display system that supports 3d display.
These displays usually require the use of LCD shutter glasses that alternate
which eye is open, and the display switches between which frame is displayed.
The command `make eniac3d` builds this one.

4. `eniact5`: another 3d experience.
This one works with the AR system from
[Tilt Five](http://www.tiltfive.com) and requires both the Irrlicht library
and the Tilt Five native SDK.
It is built with the command `make eniact5`.

5. `ledmat`: a driver for an LED matrix connected to a Raspberrhy Pi running
Plan 9.
Some discussion and a brief demonstration of this was presented at the
[11th International Workshop on Plan 9](http://iwp9.org).

6. `eniacweb`: a browser-based control panel, plus a walkable 3D view (a
web counterpart to `eniacfp`). See "Running the Web Interface" below -
unlike the other four, it isn't invoked with `-v` directly.

To run the simulator with a graphical display, add the `-v` option to the
command.
For example, if we wanted to run the prime number sieve with the TCL/Tk
graphical support, we'd use the command:

> `src/eniac -v vis/eniactk sieve.e`

(assuming all the executables are in their respective source directories).

### Running the Web Interface

The web interface is split into two pieces:

- `vis/eniacweb-adapter`: a minimal "vis" program that bridges the engine
  display protocol to a loopback TCP connection. You never invoke this one
  directly - `eniacweb-server` spawns it (via `eniac -v`) once per browser
  session.
- `vis/eniacweb-server`: the actual web server. Unlike the other visualizers,
  *it* spawns `eniac` (not the other way around), so that every browser tab
  gets its own independent, isolated machine.

Build everything from the repo root:

> `go build -o src/eniac ./src`
>
> `go build -o vis/eniacweb-adapter/eniacweb-adapter ./vis/eniacweb-adapter`
>
> `go build -o vis/eniacweb-server/eniacweb-server ./vis/eniacweb-server`

(append `.exe` to each output name on Windows). Then, from the repo root:

> `vis/eniacweb-server/eniacweb-server -addr :8080`

and open `http://localhost:8080` in a browser. Pick a program from the
dropdown and click "Start New Session" to power up a fresh machine; each
browser tab that does this gets its own `eniac` process, so multiple people
(or multiple tabs) can run independent simulations at the same time. Closing
a tab or clicking "Start New Session" again tears down that session's
process.

The panel images served by `eniacweb-server` are pre-converted from
`images/*.ppm` with `tools/ppm2img` (Go's standard library can't decode PPM).
If you ever need to regenerate them - e.g. after changing which section
images are used - run, from the repo root:

> `go run ./tools/ppm2img images/e1900s1.ppm vis/eniacweb-server/web/static/s1.jpg images/e1900s2.ppm vis/eniacweb-server/web/static/s2.jpg images/e1900s3.ppm vis/eniacweb-server/web/static/s3.jpg images/e1900s4.ppm vis/eniacweb-server/web/static/s4.jpg images/e1900s5.ppm vis/eniacweb-server/web/static/s5.jpg`

#### 3D Walkthrough

Once `eniacweb-server` is running, open `http://localhost:8080/3d.html` for a
walkable first-person view of the machine room - a browser-based counterpart
to `eniacfp`, using the same `obj/eniact.obj` model and the same live protocol.
Controls: drag the view with the mouse to look around (the mouse is never
captured, so the on-screen buttons stay clickable at any time - no `Esc`
needed), W/S or up/down arrows to walk forward/back, and A/D or left/right
arrows to turn - the same walk/turn split `eniacfp`'s own arrow-key controls
use, rather than a strafe. It reuses `eniacweb-server`'s existing session
machinery unchanged - each browser tab still gets its own isolated `eniac`
process - and serves the (~28MB, not `go:embed`'d) `obj/` directory straight
from disk via its own route.

The Three.js library and loaders it depends on are vendored under
`vis/eniacweb-server/web/vendor/three/` (pinned to a specific release,
fetched once from `unpkg.com` - not an npm dependency of this Go module, and
not fetched at runtime).

**Windows note:** `eniacweb-server` opens a network listener and spawns child
processes, a combination Windows Smart App Control's heuristics may flag and
block outright on some machines, even for a locally-built binary. If you hit
"An Application Control policy has blocked this file" when trying to run it,
either allow it via Windows Security's Protection History (if an entry
appears there), or build and run it under WSL instead - WSL2 forwards
`localhost` to Windows automatically, so `http://localhost:8080` still works
from your normal browser either way.
