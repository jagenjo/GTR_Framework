To make the project work you need to install SDL2.
Download the latest version of SDL2 osx developer from:
https://www.libsdl.org/download-2.0.php

Install the SDL2.Framework inside the /Library/Framework

If the first time you run the application you see an strange crash with the error:
- EXC_BAD_ACCESS (Code Signature Invalid)

you must sign the library by going to the folder using the terminal and writing the command:

codesign -f -s - SDL2.Framework
