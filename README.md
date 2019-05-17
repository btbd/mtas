# Mirror's Edge TAS Tool (WIP)

A program for making tool-assisted Mirror's Edge speedruns.

## Setup

1. Download the latest release.
2. Extract into any directory.
2. Run the program before starting Mirror's Edge.

## Usage

- The topmost input is the command to be executed to start the demo. This should be a command that starts a level.
- The listview below displays the frames of the demo with the inputs to be done for each frame.
	- *MX* for mouse x.
	- *MY* for mouse y.
	- Green inputs signify a keydown event, and red signifies a keyup event.
	- Copying, pasting, cutting, and clearing the selected frames' inputs are available.
	- Doubleclicking on a frame will bring up a dialog for the frame.
		- Shows and allows editing of the mouse and keyboard inputs on the frame.
		- The *Goto* button brings you to the frame as fast as possible by disabling core rendering and animation functions.
- *Start* button
	- Starts the demo and records frames.
- || or ▶ button
	- Pauses/unpauses the demo.
- *Advance* button (or F1)
	- Advances one frame forward in the demo.
- State Breakpoints
	- `Change` - Pauses the demo when Faith's state changes.
	- `Ground`, `Air`, `Wallrun`, `Wallclimb` - Pauses the demo when Faith's state changes to this.
- Frame Goto
	- Options for the frame *Goto* button.
	- `Fast load` - Ignores the preserveration of bot actors during the goto.
	- `Ignore level streaming` - Halts level streaming during the goto.
- Record Live Input
	- If this is checked, live input will be enabled while the demo is playing and will be recorded.
- *Stop All* button
	- Stops playing the current demo and clears all flags allowing the game to function as if no demo is active.

### Tools

- Faith
	- Displays useful information about Faith's actor.
	- `x`, `y`, `z` - Position vector.
	- `vx`, `vy`, `vz` - Velocity vector.
	- `hs` - Horizontal speed (km/h).
	- `vs` - Vertical speed (km/h).
	- `s` - State.
	- `h` - Health.
	- `rx` - Camera rotation about the x-axis (degrees).
	- `ry` - Camera rotation about the y-axis (degrees).
- Commands
	- Shows the executed commands and can execute a command.
- Timescale
	- Changes the time before advancing to the next frame when playing a demo.
- Translator
	- Replaces all binds in the current demo with another.
	- Usage:
		- Each rule in the ruleset must be in this format: `originalbind > replacebind` (spacing is ignored).
		- Multiple rules are allowed by separating them with a newline.
			- Example ruleset:
			
			```
			LeftMouseButton > RightMouseButton
			W>D
```

		- Click *Replace* to replace the binds.
- Focus and Resume
	- Sets focus to Mirror's Edge and resumes the demo. Useful for live input recording.

## Contributing

### Setup
Both the client DLL and EXE are written in C++ and compiled using Visual Studio 2017. To contribute you can download Visual Studio Community 2017 or download a paid or trial version of Visual Studio, such as Visual Studio Professional 2017 or Visual Studio Enterprise 2017.

### Testing
When compiling either the DLL or EXE to test your contributions, be sure that the runtime library is `Multi-threaded (/MT)` because all libraries should be statically linked to the compiled output. You can check the runtime library by going to:

```Project -> Properties -> Configuration Properties -> C/C++ -> Code Generation -> Runtime Library```

In addition, be sure the compiler configuration is set to:
- `Release` instead of `Debug` for faster optimization.
	
- `x86` because Mirror's Edge is a 32-bit application.

- Unicode for the character set.

For the DLL, you will need to include the DirectX 9 Library. You can download the SDK from <a href="https://www.microsoft.com/en-us/download/confirmation.aspx?id=6812">here</a>.

Once downloaded and installed, open the DLL project in Visual Studio and go to `Project -> Project Properties -> Configuration Properties -> VC++ Directories` and add the SDK include and library directories accordingly.