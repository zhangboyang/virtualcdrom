# Simple Virtual CDROM for macOS

## Introduction

* Use kext (kernel extensions) to simulate a CD-ROM drive
* Tested on macOS Big Sur
* Code is extremely simple
* Only supports loading Audio CD
* Supports loading CUE sheets
* Supports loading a series of audio files as tracks

## Build

### Requirements

1. Install Xcode, make sure `clang` & `python3` accessible from command-line.
2. [Disable SIP](https://developer.apple.com/library/archive/documentation/Security/Conceptual/System_Integrity_Protection_Guide/ConfiguringSystemIntegrityProtection/ConfiguringSystemIntegrityProtection.html) in order to load kexts.
3. Download [ffmpeg for macOS](https://ffmpeg.zeranoe.com/builds/).

### Build and load kernel driver

1. Open `virtualcdrom.xcodeproj` with Xcode.
2. Press *Command-B* to build project.
3. `virtualcdrom.kext` should appear in `Products` folder in left sidebar.
4. Load kext with following commands:
    ```
    sudo cp -R [DRAG virtualcdrom.kext TO HERE] /Library/Extensions
    sudo kextutil /Library/Extensions/virtualcdrom.kext
    ```
5. Approve the kext in System Preferences and reboot.
6. Verify kext loaded:
    ```
    kextstat | grep virtualcdrom
    ```

### Build and prepare user-space client

1. Build `client`
    ```
    clang -o client client.c -framework IOKit
    ```
2. Copy `ffmpeg`, let it along with `client`
    ```
    cp /path/to/ffmpeg-X.X.X-macos64-static/bin/ffmpeg ffmpeg
    ```
3. Run `./loadcd.py` to verify everything is ok, it should output:
    ```
    sending 0 bytes to kernel
      successfully loaded 0 bytes to kernel
    ```

## Let's load CD!

### Load a CUE sheet (with one big audio file)

  * Command:
    ```
    ./loadcd.py [DRAG CUE FILE HERE]
    ```
  * Example output of `loadcd.py`:
    ```
    trying utf_8
    loading disk image
      decoding '此花亭の四季.wav'
    ==================================================
    track 00 start  0:00.00 sector      0 crc ????????
    track 01 start  3:51.11 sector  17336 crc ????????
    track 02 start  7:41.22 sector  34597 crc ????????
    track 03 start 12:21.69 sector  55644 crc ????????
    --------------------------------------------------
    total          16:34.09 sector  74559 crc 3f2a71d8
    ==================================================
    sending 175715568 bytes to kernel
      successfully loaded 175715568 bytes to kernel
    ```

### Or, Load a series of audio files as tracks

  * Command:
    ```
    ./loadcd.py [DRAG AUDIO FILES HERE]
    ```
  * Example output of `loadcd.py`:
    ```
    loading track 00
      decoding '01 春ウララ、君ト咲キ誇ル.wav'
    loading track 01
      decoding '02 夏咲き恋花火.wav'
    loading track 02
      decoding '03 茜空、君舞フ紅葉ノ散歩道.wav'
    loading track 03
      decoding '04 雪華煌めく家路にて.wav'
    ==================================================
    track 00 start  0:00.00 sector      0 crc 5d8d3313
    track 01 start  3:51.11 sector  17336 crc 6ca8c5b0
    track 02 start  7:41.22 sector  34597 crc 05469636
    track 03 start 12:21.69 sector  55644 crc 35827c20
    --------------------------------------------------
    total          16:34.09 sector  74559 crc 3f2a71d8
    ==================================================
    sending 175715568 bytes to kernel
      successfully loaded 175715568 bytes to kernel
    ```
