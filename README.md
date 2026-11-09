# ESP32-S3 Embedded Acoustic Fingerprinting

An embedded audio recognition project based on the ESP32-S3, INMP441 MEMS microphone, and Olaf acoustic fingerprinting.

## 1. Project Overview

This project explores the implementation of real-time acoustic fingerprinting on a resource-constrained microcontroller.

The system uses an INMP441 digital MEMS microphone to capture audio through the I2S interface. The captured audio is processed by an embedded implementation of Olaf, an acoustic fingerprinting system, to identify audio against a locally stored reference fingerprint database.

The project also explores how the system can be extended from a single-track demonstration to a multi-track recognition setup using an offline-generated fingerprint database.

**Main research question:**

*How robust is lightweight acoustic fingerprinting on a resource-constrained microcontroller?*

## 2. Project Objectives

- Implement digital audio acquisition using the INMP441 microphone and ESP32-S3.
- Integrate the Olaf acoustic fingerprinting system into an embedded environment.
- Explore multi-track recognition using an offline-generated reference fingerprint database.
- Investigate the effects of audio input quality and algorithm parameters on recognition behaviour.
- Evaluate the limitations of real-time audio recognition under practical hardware and environmental constraints.

## 3. Hardware

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

## 4. System Architecture

The recognition system follows this general workflow:

1. Reference audio tracks are processed offline to generate acoustic fingerprints.
2. The generated fingerprints are stored in a reference header file.
3. The reference database is included in the ESP32 firmware.
4. The INMP441 captures live audio through I2S.
5. Olaf processes the incoming audio and compares its fingerprints with the stored reference database.
6. A recognition callback is triggered when a match satisfies the configured matching criteria.

The embedded implementation uses a pre-generated fingerprint index rather than building the reference database on the microcontroller.

## 5. Software and Signal Processing

The project involves the following technologies:

- **I2S:** Digital audio acquisition from the INMP441 microphone.
- **PCM audio:** Representation and processing of sampled audio.
- **FFT:** Frequency-domain analysis used in acoustic fingerprint extraction.
- **Event Points:** Salient time-frequency features used by Olaf.
- **Acoustic fingerprints:** Compact representations used for audio matching.
- **Embedded C/C++:** Firmware development for the ESP32-S3.
- **Arduino IDE:** Firmware development and deployment.

## 6. Development and Experiments

The project is organised into several development and investigation stages.

### 6.1 Microphone Acquisition

A standalone microphone test program was developed to record audio from the INMP441 to a microSD card.

The recording configuration is:

- Sample rate: 16 kHz
- Channels: Mono
- Output format: 16-bit PCM WAV
- Recording duration: 10 seconds

Basic signal statistics, including average amplitude, minimum, maximum, and RMS, are collected to examine the recorded audio.

### 6.2 Embedded Olaf Integration

The Olaf embedded example was integrated with the ESP32-S3 and INMP441 microphone.

The firmware captures overlapping audio blocks and passes them to the fingerprint matching system.

### 6.3 Multi-track Recognition

The reference database can be generated offline from multiple audio tracks and compiled into the embedded firmware.

This extends the original single-track demonstration by allowing the embedded system to search against a multi-track reference database.

### 6.4 Recognition Reliability

Further experiments will investigate the effects of:

- Background noise
- Microphone input quality
- Fingerprint extraction parameters
- Matching thresholds
- Reference database size

Recognition performance will be evaluated using positive and negative test samples.

## 7. Current Limitations and Future Work

This project is still under development. Further work is needed to establish reliable quantitative performance.

Planned investigations include:

- Measuring false positive and false negative rates.
- Evaluating recognition performance under different background noise conditions.
- Investigating the influence of fingerprint and matching parameters.
- Measuring memory usage and processing performance on the ESP32-S3.
- Evaluating the effect of reference database size on embedded recognition.
- Improving the reproducibility of experiments.

No general claim of noise robustness or recognition accuracy is made until supported by experimental results.

## 8. Repository Structure

```text
.
├── firmware/
│   ├── microphone_test/
│   └── olaf_esp32/
├── hardware/
├── experiments/
├── analysis/
├── results/
├── docs/
├── THIRD_PARTY_NOTICES.md
└── README.md
```

The repository will include firmware, hardware documentation, experimental procedures, analysis scripts, and results as the project develops.

## 9. Third-party Software and Licensing

This project incorporates Olaf, an open-source acoustic fingerprinting system developed by Joren Six.

Olaf is distributed under the GNU Affero General Public License v3.0 (AGPL-3.0).

Original copyright and license notices are retained in the relevant source files. See `THIRD_PARTY_NOTICES.md` and the applicable license files for further information.

## 10. Acknowledgements

Thanks to the original developers of Olaf for making their acoustic fingerprinting implementation available as open-source software.

