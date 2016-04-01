
gui.json format

###Action functions
Launch specific function ("func" object)

|Key|Description|Parameters|
|---|---|---|
|RUN_RXMODE|Boot into rxMode|drive (1 - SysNAND, 2 - EmuNAND)|
|RUN_PASTA|Boot into Pasta mode||
|RUN_SHUTDOWN|Shutdown console|0 - turn off, 1 - reboot|
|RUN_CFG_NEXT|Toggle configuration option to the next value|configuration key|

###Status check functions
Check menu option state ("enabled" object)

|Key|Description|Parameters|
|---|---|---|
|CHK_CFG|Check rxTools configuration property availability|configuration key|
|CHK_EMUNAND|Check if EmuNAND exists||
|CHK_FILE|Check if file exist, have exact size and hash|full path name; size(optional); MD5 hash(optional)|
|CHK_TITLE|Check that .tmd and accompanied encrypted (.app) or decrypted (obtaied from NUS) exists|full path name of the .tmd|

###Value resolve functions
Get rxTools or system property value ("resolve" object)

|Key|Description|Parameters|
|---|---|---|
|VAL_CFG|rxTools configuration property value|configuration key|
|VAL_RXTOOLS_BUILD|rxTools build version (WIP)||
|VAL_REGION|NAND region (WIP)|drive (1 - SysNAND, 2 - EmuNAND)|
|VAL_TITLE_VERSION|Latest installed title version|full path to content directory|

###Configuration keys
Used to display and change rxTools configuration parameters

|Key|Description|Values|
|---|---|---|
|CFG_BOOT_DEFAULT|Default boot mode|0 - GUI, 1 - rxMode SysNAND, 2 - rxMode EmuNAND, 3 - Pasta mode|
|CFG_GUI_FORCE|Override default boot and boot into GUI|Button code|
|CFG_EMUNAND_FORCE|Override default boot and boot into rxMode EmuNAND|Button code|
|CFG_SYSNAND_FORCE|Override default boot and boot into rxMode SysNAND|Button code|
|CFG_PASTA_FORCE|Override default boot and boot into Pasta mode|Button code|
|CFG_THEME|Use this theme|Theme name|
|CFG_AGB_BIOS|Show AGB BIOS logo|true/false|
|CFG_LANGUAGE|Language name|ex. "en"|
