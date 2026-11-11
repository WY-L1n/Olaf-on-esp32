# ESP32-S3 Embedded Acoustic Fingerprinting

An embedded audio recognition project based on the ESP32-S3, INMP441 MEMS microphone, and Olaf acoustic fingerprinting.

## 1. Project Overview

This project explores the implementation of real-time acoustic fingerprinting on a resource-constrained microcontroller.

The system uses an INMP441 digital MEMS microphone to capture audio through the I2S interface. The captured audio is processed by an embedded implementation of Olaf, an acoustic fingerprinting system, to identify audio against a locally stored reference fingerprint database.

The project also explores how the system can be extended from a single-track demonstration to a multi-track recognition setup using an offline-generated fingerprint database.

Eventually I failed to figure out why the recognition program loves white noise so much. The more the microphone got closer to my computer (with fan operation noise), the more frequently the matching occurs. I put everything I have here so someone interested in this may keep going.

## 2. Hardware

| Component | Description |
|---|---|
| Microcontroller | ESP32-S3 (N16R8) |
| Microphone | INMP441 digital MEMS microphone |
| Audio interface | I2S |
| Storage | MicroSD card module (SPI), used for audio recording and testing |
| Audio output | MAX98357 module (planned / not currently connected) |

### INMP441 Connections

| INMP441 | ESP32-S3 |
|---|---|
| SD | GPIO 4 |
| SCK / BCLK | GPIO 7 |
| WS | GPIO 6 |
| L/R | GND |

## 3. System Architecture

The recognition system follows this general workflow:

1. Reference audio tracks are processed offline (I used MSYS2 UCRT64) to generate acoustic fingerprints.
2. The generated fingerprints are stored in a reference header file.(originally named olaf_fp_ref_mem.h)
3. The reference database is included in the ESP32 firmware.
4. The INMP441 captures live audio through I2S.
5. Olaf processes the incoming audio and compares its fingerprints with the stored reference database.
6. A recognition callback is triggered when a match satisfies the configured matching criteria.

The embedded implementation uses a pre-generated fingerprint index rather than building the reference database on the microcontroller.

## 4. Development and Experiments

The project is organised into several development and investigation stages.

### 4.1 Initial Testing

Fingerprints extracted from 39 kanye west's songs and Olaf were uploaded to ESP32. 

Observation: A lot of matching from different songs all came once and occured consistently. No obvious pattern

### 4.2 Microphone Testing

A standalone microphone test program was developed to record audio from the INMP441 to a microSD card.

The recording configuration is:

- Sample rate: 16 kHz
- Channels: Mono
- Output format: 16-bit PCM WAV
- Recording duration: 10 seconds

Basic signal statistics, including average amplitude, minimum, maximum, and RMS, are collected to examine the recorded audio.

An audio was recorded under silent condition. Another audio was recorded near a phone playing music. 
Two files were imported to computer, then play the second audio and adjust system volume to a proper value. Then play the first one to check background noise.

Conclusion: Background noise from INMP441 was audiable, but it should not be the reason as Olaf should be resistive to noise.

### 4.3 Trouble Shooting

Replace olaf_39_all.h with olaf_nsma.h

Observation: Matching still occured even no music was playing.
The more the microphone got closer to my computer (with fan operation noise), the more frequently the matching occurs.

### 4.4 Conclusion

Possible reason: Ineffective peaks were extracted from audio picked, and make frequent collision to the fingerprint to cause false matches.

## 5. Repository Structure
Olaf-on-ESP32/
│
├── firmware/
│   │
│   ├── created/
│   │   │
│   │   ├── custom path/
│   │   │   ├── olaf_39_all.h
│   │   │   ├── olaf_530.h
│   │   │   └── olaf_nsma.h
│   │   │
│   │   ├── Olaf-master/
│   │   │   └── src/
│   │   │       ├── olaf_db_mem_pack.h
│   │   │       ├── olaf_db_mem.c
│   │   │       ├── olaf_fp_db_writer_mem.c
│   │   │       └── olaf_fp_ref_mem.h
│   │   │
│   │   ├── partitions.csv
│   │   └── recording.ino
│   │
│   └── edited/
│       ├── esp32_inmp441_olaf.ino
│       └── olaf_config.c
│
├── hardware/
│   └── 123.
│
├── README.md
│
└── research_log.md

## 6. Third-party Software and Licensing

This project incorporates Olaf, an open-source acoustic fingerprinting system developed by Joren Six.

Olaf is distributed under the GNU Affero General Public License v3.0 (AGPL-3.0).

## 7. Acknowledgements

Thanks to the original contributors of Olaf for making their acoustic fingerprinting implementation available as open-source software.
Original code: https://github.com/JorenSix/Olaf.git

