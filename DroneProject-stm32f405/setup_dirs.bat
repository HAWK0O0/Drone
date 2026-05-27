@echo off
REM Run this once to create the subdirectory structure for fc-stm32f405
REM Double-click or run from project root: setup_dirs.bat

set BASE=%~dp0

echo Creating directories...
mkdir "%BASE%src\drivers"    2>NUL
mkdir "%BASE%src\ahrs"       2>NUL
mkdir "%BASE%src\control"    2>NUL
mkdir "%BASE%src\telemetry"  2>NUL
mkdir "%BASE%include\drivers"    2>NUL
mkdir "%BASE%include\ahrs"       2>NUL
mkdir "%BASE%include\control"    2>NUL
mkdir "%BASE%include\telemetry"  2>NUL

echo Moving drivers...
move "%BASE%src\motor.c"      "%BASE%src\drivers\"
move "%BASE%src\imu.c"        "%BASE%src\drivers\"
move "%BASE%src\sbus.c"       "%BASE%src\drivers\"
move "%BASE%src\gps.c"        "%BASE%src\drivers\"
move "%BASE%src\sdcard.c"     "%BASE%src\drivers\"
move "%BASE%src\buzzer.c"     "%BASE%src\drivers\"
move "%BASE%src\led.c"        "%BASE%src\drivers\"
move "%BASE%include\motor.h"  "%BASE%include\drivers\"
move "%BASE%include\imu.h"    "%BASE%include\drivers\"
move "%BASE%include\sbus.h"   "%BASE%include\drivers\"
move "%BASE%include\gps.h"    "%BASE%include\drivers\"
move "%BASE%include\sdcard.h" "%BASE%include\drivers\"
move "%BASE%include\buzzer.h" "%BASE%include\drivers\"
move "%BASE%include\led.h"    "%BASE%include\drivers\"

echo Moving AHRS...
move "%BASE%src\madgwick.c"   "%BASE%src\ahrs\"
move "%BASE%src\ahrs.c"       "%BASE%src\ahrs\"
move "%BASE%include\madgwick.h" "%BASE%include\ahrs\"
move "%BASE%include\ahrs.h"     "%BASE%include\ahrs\"

echo Moving control...
move "%BASE%src\pid.c"        "%BASE%src\control\"
move "%BASE%src\mixer.c"      "%BASE%src\control\"
move "%BASE%include\pid.h"    "%BASE%include\control\"
move "%BASE%include\mixer.h"  "%BASE%include\control\"

echo Moving telemetry...
move "%BASE%src\telemetry.c"      "%BASE%src\telemetry\"
move "%BASE%include\telemetry.h"  "%BASE%include\telemetry\"

echo.
echo Done! Update #include paths in source files to use subdirectory prefixes:
echo   e.g.  #include "drivers/motor.h"
echo         #include "ahrs/madgwick.h"
echo         #include "control/pid.h"
echo         #include "telemetry/telemetry.h"
pause
