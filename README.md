# Mirror's Edge TAS Tool (WIP)

A program for making tool-assisted Mirror's Edge speedruns.

## Setup

1. Download the latest release.
2. Extract into any directory.
2. Run the program before starting Mirror's Edge.

## Usage

To be continued...

## Contributing

### Setup
Both the client DLL and EXE are written in C++ and compiled using Visual Studio 2017. To contribute you can download Visual Studio Community 2017 (or newer) or download a paid or trial version of Visual Studio, such as Visual Studio Professional 2017 (or newer) or Visual Studio Enterprise 2017 (or newer).

### Testing
When compiling either the DLL or EXE to test your contributions, be sure that the runtime library is `Multi-threaded (/MT)` because all libraries should be statically linked to the compiled output. You can check the runtime library by going to:

```Project -> Properties -> Configuration Properties -> C/C++ -> Code Generation -> Runtime Library```

In addition, be sure the compiler configuration is set to:
- `Release` instead of `Debug` for faster optimization.
	
- `x86` because Mirror's Edge is a 32-bit application.

- Unicode for the character set.

For the DLL, you will need to include the DirectX 9 Library. You can download the SDK from <a href="https://www.microsoft.com/en-us/download/confirmation.aspx?id=6812">here</a>.

Once downloaded and installed, open the DLL project in Visual Studio and go to `Project -> Project Properties -> Configuration Properties -> VC++ Directories` and add the SDK include and library directories accordingly.