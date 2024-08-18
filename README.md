![screenshot](screenshot.png)

# Wii U Recovery Menu
A simple recovery menu running on the IOSU for unbricking, which can be booted using [udpih](https://github.com/GaryOderNichts/udpih). 

> :information_source: Some Wii U's don't show any TV output, if it hasn't been configured properly before.  
> If that's the case download the `recovery_menu_dc_init` file and rename it to `recovery_menu`.  
> This build does display controller initialization and might fix the issue.  
> Note that this build only outputs 480p and has no GamePad output!

> :information_source: The recovery menu updates the power LED for debugging. The following patterns are used:  
> **purple-off blinking**: kernel code running  
> **purple-orange blinking**: menu thread running  
> **purple**: menu is ready and running  

## Options
### Set Coldboot Title
Allows changing the current title the console boots to.  
Useful for unbricking CBHC bricks.  
Possible options are:  
- `Wii U Menu (JPN) - 00050010-10040000`
- `Wii U Menu (USA) - 00050010-10040100`
- `Wii U Menu (EUR) - 00050010-10040200`

On non-retail systems the following additional options are available:
- `System Config Tool - 00050010-1F700500`
- `DEVMENU (pre-2.09) - 00050010-1F7001FF`
- `Kiosk Menu         - 00050010-1FA81000`

### Dump Syslogs
Copies all system logs to a `logs` folder on the root of the SD Card.

### Dump OTP + SEEPROM
Dumps the OTP and SEEPROM to `otp.bin` and `seeprom.bin` on the root of the SD Card.

### Start wupserver
Starts wupserver which allows connecting to the console from a PC using [wupclient](https://gist.github.com/GaryOderNichts/409672b1bd5627b9dc506fe0f812ec9e).

### Load Network Configuration
Loads a network configuration from the SD, and temporarily applies it to use wupserver.  
The configurations will be loaded from a `network.cfg` file on the root of your SD.  
For using the ethernet adapter, the file should look like this:
```
type=eth
```
For using wifi:
```
type=wifi
ssid=ssidhere
key=wifikeyhere
key_type=WPA2_PSK_AES
```

### Pair Gamepad
Displays the Gamepad Pin and allows pairing a Gamepad to the system. Also bypasses any region checks while pairing.  
The numeric values represent the following symbols: `♠ = 0, ♥ = 1, ♦ = 2, ♣ = 3`.  
Note that rebooting the system might be required to use the newly paired gamepad.

### Install WUP
Installs a valid signed WUP from the `install` folder on the root of your SD Card.  
Don't place the WUP into any subfolders.

### Edit Parental Controls
Displays the current Parental Controls pin configuration.  
Allows disabling Parental Controls.

### Debug System Region
Fixes bricks caused by setting productArea and/or gameRegion to an invalid
value. Symptoms include being unable to launch System Settings or other
in-region titles.

### System Information
Displays info about several parts of the system.  
Including serial number, manufacturing date, console type, regions, memory devices...

### Submit System Data
Allows submitting system information to an online database to collect various statistics about Wii U consoles.  
This is entirely optional and personally identifying information will be kept confidential.  
[The database can be found here!](https://wiiu.gerbilsoft.com/)

### Load BOOT1 payload
Loads a payload from the root of the SD Card named `boot1.img` and executes it from within boot1.
If the file is named `boot1now.img` it gets loaded automatically when starting the recovery_menu after a 5 second timeout.

## Building
```bash
# build the docker container
docker build -t recoverybuilder .

# build the menu
docker run -it --rm -v ${PWD}:/project recoverybuilder make

# build the menu with display controller initialization
docker run -it --rm -v ${PWD}:/project recoverybuilder make DC_INIT=1
```

## Credits
- [@Maschell](https://github.com/Maschell) for the [network configuration types](https://github.com/devkitPro/wut/commit/159f578b34401cd4365efd7b54b536154c9dc576)
- [@dimok789](https://github.com/dimok789) for [mocha](https://github.com/dimok789/mocha)
- [@hexkyz](https://github.com/hexkyz) for [hexFW](https://github.com/hexkyz/hexFW)
- [@rw-r-r-0644](https://github.com/rw-r-r-0644) for the lolserial code and display configuration info
- [decaf-emu](https://github.com/decaf-emu/decaf-emu) for a lot of IOS documentation
- [@GerbilSoft](https://github.com/GerbilSoft) for adding the initial "System Information" screen, visual improvements, region unbricking, ...
