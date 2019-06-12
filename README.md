# MobiView

MobiView is a GUI for the model building system [Mobius](https://github.com/NIVANorge/Mobius)

MobiView can load and work with any Mobius model that has been compiled to a .dll using the Mobius dll interface. It understands the .dat format for parameters and inputs.

MobiView is made using the [Ultimate++](https://www.ultimatepp.org/) GUI library.

![Example of MobiView viewing the PERSiST model](img/MobiView.png)



Known issues:
Windows 10 may scale the layout of your programs, and for MobiView this may make the window too large for your screen.
If this happens:
- Right click your desktop, choose 'display settings', then go to 'Change the size of text, apps, and other items' and set it to 100%.
If this does not work right away, you could
- either restart windows
- or right click MobiView.exe, then go to Settings->Compatibility, click 'Change high DPI settings', then check the first box under 'Program DPI', then change 'Use the DPI that's set for my main display when' to 'I open this program'.
