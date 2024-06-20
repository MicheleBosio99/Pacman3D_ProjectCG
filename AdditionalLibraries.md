# Additional Libraries

Here are shown the passages to include libraries used in the project other than Vulkan, GLFW and the usual headers.

## irrKlang

Used to handle in-game sounds, here's how it is included:

- Go to the irrKlang website and download the correct package for your system architecture. Put it somewhere external from the project folder;
- As for Vulkan and GLFW:
	- put the include folder in the project properties under C/C++ -> General -> Additional Include Directories;
	- put the lib folder in properties under Linker -> General -> Additional Library Directories;
	- under the Linker -> Input -> Additional Dependencies add to the Vulkan and GLFW also the line "irrKlang.lib" (without the "");
- irrKlang library should be working now, check that on execution on the screen there's the output of its initialization;

- Problem: if, upon execution, an error citing "irrKlang.dll" is shown, go to the irrKlang/bin/<your architecture> and copy the file 	irrKlang.dll, which needs to be pasted inside the project folder where the executable is created (generally project-folder/x64/Debug 	and/or project-folder/x64/Release) and the problem should be solved.

## 
