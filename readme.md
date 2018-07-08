This repository hosts the firmware and source code for the RFIDuino Windows Login Project.
Code can be compled in Visual Studio 2012.

For instructions on installation go here
http://learn.robotgeek.com/getting-started/41-rfiduino/181-rfiduino-windows-login.html


See the 'releases' page for compiled binary .dll files to install on your windows machine.
https://github.com/robotgeek/RFIDuinoWindowsLogin/releases


This project is based on / uses code from the original Windows Login Project from the Redbee RFID Experimenters Kit, code written by Amal Graafstra
http://amal.net

Thank you to Eric Jastram for compilation and editing support.

=======

updated by haimiko:

Added basic file encryption so that the credentials aren't in clear text.  This isn't fool proof but will have to do for now until I have more time to work on it.  It will keep someone from casually stumbling on the password in the text file.  Use at your own risk.

The first time it detects that the file is not encrypted, it encrypts it.  Whenever you change your password, you will need to replace with the cleartext file with the updated password.  

