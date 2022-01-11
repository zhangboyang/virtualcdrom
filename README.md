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
2. [Disable SIP](https://developer.apple.com/library/archive/documentation/Security/Conceptual/System_Integrity_Protection_Guide/ConfiguringSystemIntegrityProtection/ConfiguringSystemIntegrityProtection.html) in order to load self-compiled kexts.
3. Download [ffmpeg for macOS](https://ffmpeg.org/download.html).

### Build and load kernel driver

1. Open `virtualcdrom.xcodeproj` with **Xcode**.
2. Press *Command-B* to build project.
3. From *Product* menu, click *Show Build Folder in Finder*.
4. `virtualcdrom.kext` should appear in `Products/Debug` in build folder.
5. Load kext with following commands:
    ```
    sudo cp -R [DRAG virtualcdrom.kext TO HERE] /Library/Extensions
    sudo kextutil /Library/Extensions/virtualcdrom.kext
    ```
6. Approve the kext in **System Preferences** and reboot.
7. Verify kext is loaded:
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
    cp /your/path/to/ffmpeg ffmpeg
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
      decoding '明日への扉.wav'
    ==================================================
    track 00 start  0:00.00 sector      0 crc ????????
    track 01 start  4:46.59 sector  21509 crc ????????
    track 02 start  9:34.44 sector  43094 crc ????????
    --------------------------------------------------
    total          17:23.02 sector  78227 crc 75ea388c
    ==================================================
    sending 184342704 bytes to kernel
      successfully loaded 184342704 bytes to kernel
    ```

### Or, Load a series of audio files as tracks

  * Command:
    ```
    ./loadcd.py [DRAG AUDIO FILES HERE]
    ```
  * Example output of `loadcd.py`:
    ```
    loading track 00
      decoding '01 明日への扉.wav'
    loading track 01
      decoding '02 明日への扉 (instrumental).wav'
    loading track 02
      decoding '03 エピローグミニドラマ「明日と加瀬さん。」.wav'
    ==================================================
    track 00 start  0:00.00 sector      0 crc a209b77f
    track 01 start  4:46.59 sector  21509 crc a5683510
    track 02 start  9:34.44 sector  43094 crc 0120bcf0
    --------------------------------------------------
    total          17:23.02 sector  78227 crc 75ea388c
    ==================================================
    sending 184342704 bytes to kernel
      successfully loaded 184342704 bytes to kernel
    ```
