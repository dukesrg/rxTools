###Feature compatibility check chart
||3DS|New 3DS|2DS|
|---|---|---|---|---|
|RX-E (fw9.2/EmuNAND 10.7)|OK||||
|RX-E (fw9.2/EmuNAND 11.0)|||||
|RX-S (fw9.2)|OK||||
|Pasta (fw9.2)|OK||||
|SysNAND dump|OK||||
|SysNAND inject|||||
|SysNAND partition decrypt&dump|OK (except TWL)||||
|SysNAND partition encrypt&inject|||||
|EmuNAND dump|OK||||
|EmuNAND inject|OK||||
|EmuNAND partition decrypt&dump|OK (except TWL)||||
|EmuNAND partition encrypt&inject|OK (except TWL)||||
|SDinfo.bin XORpad|OK||||
|ncchinfo.bin XORpad|OK||||
|NAND partition XORpad|OK (except TWL)||||

#gui.json format

###Action functions
Launch specific function ("func" object)

|Key|Description|Parameters|
|---|---|---|
|RUN_RXMODE|Boot into rxMode|drive|
|RUN_PASTA|Boot into Pasta mode||
|RUN_SHUTDOWN|Shutdown console|0 - turn off, 1 - reboot|
|RUN_CFG_NEXT|Toggle configuration option to the next value|configuration key|
|GEN_XORPAD|Generate XORpad|partition or name of a source file in ncchinfo.bin or SDinfo.bin format|
|DMP_PARTITION|Dump decrypted NAND partition|drive; partition; full path name|
|DMP_NAND|Dump NAND (backup to file as is)|drive; full path name|
|INJ_PARTITION|Inject unencrypted NAND partition|drive; partition; full path name|
|INJ_NAND|Inject NAND (restore from file as is)|drive; full path name|

###Status check functions
Check menu option state ("enabled" object)

|Key|Description|Parameters|
|---|---|---|
|CHK_CFG|Check rxTools configuration property availability|configuration key|
|CHK_NAND|Check if NAND exists and compatible with file|drive;full path name(optional)|
|CHK_PARTITION|Check NAND partition exists and compatible with file|drive;partition;full path name(optional)|
|CHK_FILE|Check if file exists, have exact size and hash|full path name; size(optional); SSHA-1/SHA-224/SHA-256 hash(optional)|
|CHK_TITLE|Check that .tmd and accompanied encrypted (.app) or decrypted (obtaied from NUS) exists|full path name of the .tmd|

###Value resolve functions
Get rxTools or system property value ("resolve" object)

|Key|Description|Parameters|
|---|---|---|
|VAL_CFG|rxTools configuration property value|configuration key|
|VAL_RXTOOLS_BUILD|rxTools build version (WIP)||
|VAL_REGION|NAND region (WIP)|drive|
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

###Drive parameter values
Indicates used NAND

|Value|Description|
|---|---|
|1|SysNAND|
|2|EmuNAND|
|3|EmuNAND1|
|4|EmuNAND2|

###Partition parameter values
Indicates used NAND partition

|Value|Description|
|---|---|
|0|TWL|
|1|AGB_SAVE|
|2|FIRM0|
|3|FIRM1|
|4|CTR region|
|5|NCSD partiton 5 (currently unused)|
|6|NCSD partiton 6 (currently unused)|
|7|NCSD partiton 7 (currently unused)|
|8|TWLN FAT 16 partition|
|9|TWLP FAT12 photo partition|
|10|TLW logical partition 2 (currently unused)|
|11|TWL logical partition 3 (currently unused)|
|12|CTR FAT16 partition|
|13|CTR logical partition 1 (currently unused)|
|14|CTR logical partition 2 (currently unused)|
|15|CTR logical partition 3 (currently unused)|
