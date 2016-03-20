
gui.json format

###Functions

|Key|Description|Parameters|
|---|---|---|
|RUN_RXMODE|Boot into rxMode|0 - SysNAND, 1 - EmuNAND|
|RUN_PASTA|Boot into Pasta mode||
|RUN_SHUTDOWN|Shutdown console|0 - reboot, 1 - turn off|
|CHK_EMUNAND|Check if EmuNAND exists||
|CHK_FILE|Check if file exist, have exact size and hash|full path name; size(optional); MD5 hash(optional)|
|CHK_TITLE|Check that .tmd and accompanied encrypted (.app) or decrypted (obtaied from NUS) exists|full path name of the .tmd|
|VAL_CHECK|Check configutation option availability|('value' parameter used)|
|VAL_TOGGLE|Toggle configutation option to the next value|('value' parameter used)|
