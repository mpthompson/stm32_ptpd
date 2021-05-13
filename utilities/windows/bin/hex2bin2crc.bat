@echo off

REM Update path to include the directory where this batch file
REM is located. This is so the hex2bin and cksum utilities can
REM be found.
for %%i in ("%~dp0.") do SET "BATCH_PATH=%%~fi"
set PATH=%PATH%;%BATCH_PATH%

REM Get the server directory relative to this batchfile.
set SERVER_PATH=%BATCH_PATH:utilities\windows\bin=server%

REM First argument is the input hex filename.
set HEX_FILENAME=%1

REM Check that the hex filename exists.
if not exist %HEX_FILENAME% (
  echo File not found: %HEX_FILENAME%
  exit /b
)

REM Create the bin filename from the hex filename.
set BIN_FILENAME=%HEX_FILENAME:.hex=.bin%

REM create the crc filename from the bin filename.
set CRC_FILENAME=%BIN_FILENAME:.bin=.crc%

REM Create the bin file from the hex file.
hex2bin %HEX_FILENAME% > NUL

REM Perform the checksum calculation.
cksum < %BIN_FILENAME% > %CRC_FILENAME%
type %CRC_FILENAME%

REM Copy the bin file and crc file to the server directory.
copy /y %BIN_FILENAME% %SERVER_PATH%\ > NUL
copy /y %CRC_FILENAME% %SERVER_PATH%\ > NUL

