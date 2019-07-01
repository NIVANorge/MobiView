# MobiView

MobiView is a GUI for the model building system [Mobius](https://github.com/NIVANorge/Mobius)

MobiView can load and work with any Mobius model that has been compiled to a .dll using the Mobius dll interface. It understands the .dat format for parameters and inputs that is specified in the [Mobius file format documentation](https://github.com/NIVANorge/Mobius/blob/master/Documentation/file_format_documentation.pdf).

MobiView is made using the [Ultimate++](https://www.ultimatepp.org/) GUI library.

![Example of MobiView viewing the PERSiST model](img/MobiView.png)


### Needed improvements (TODO list):
- It should be possible to resize the various subviews (and the resizing should preferably be remembered in the settings file).
- Allow opening separate windows with multiple plots side by side (with locked x-axises)
- Possibly organize equations and inputs in a way that makes it faster to look up the one you want.
- Equation search similar to parameter search?
- Allow sorting equations by name.
- More goodness of fit stats.
- Q-Q plot
- Better alignment of grid lines in plot (try to lock to whole months or whole years for instance).
- Fix text alignment issues.
- Parameter search should select the actual parameter, not just the group.
- Reload / load different files after the first load.
- Port to and test on Linux.

Optional:
- Have model-specific plot setups similar to the old INCAs, where for instance for INCA-P there is a simple button to open a new window with a bar plot summary of e.g. all the phosphorous processes in land. This would save a few clicks for people who are always going to be looking at very specific plot setups, and it can be useful to look at multiple plots at once.

### Known issues:
Windows 10 may scale the layout of your programs, and for MobiView this can make the window too large for your screen if the screen resolution is not very high.
If this happens:
- Right click your desktop, choose 'display settings', then go to 'Change the size of text, apps, and other items' and set it to 100%.

If this does not work right away, you could
- either restart windows
- or right click MobiView.exe, then go to Settings->Compatibility, click 'Change high DPI settings', then check the first box under 'Program DPI', then change 'Use the DPI that's set for my main display when' to 'I open this program'.

This is not an ideal solution since it also changes the scaling of all your other programs and may make text hard to read. We will try to see if there is a better solution.
