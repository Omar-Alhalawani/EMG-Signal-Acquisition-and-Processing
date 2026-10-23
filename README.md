# BioCARE — EMG Signal Acquisition, Visualization & Motor-Control Experiments

**Python · Arduino · MyoWare-style analog EMG input · Serial acquisition · Signal processing · Tkinter · Matplotlib**

An experimental electromyography (EMG) workflow that acquires analog muscle-signal readings from an Arduino, streams them to a Python desktop interface, and visualizes changes in muscle activity. The supplied code explores **baseline adjustment, signal rectification, envelope and RMS extraction, activation detection with hysteresis, frequency-domain plots, calibration, and CSV-based trial logging**. A separate application variant experiments with mapping EMG activation to motor-control commands.

> **Project status:** Research/engineering prototype, not a medical device or certified assistive product. Hardware and physiological performance have **not been independently validated** from this repository. In particular, the motor sketches are experimental and must be reviewed before powering any actuator.

## System overview

```text
Analog EMG sensor (A0)
       │
       ▼
Arduino firmware ── serial 115200 baud ──► Python acquisition thread
  "milliseconds,ADC"                         │
                                             ▼
                                Offset tracking / normalization
                                             │
                        ┌────────────────────┼────────────────────┐
                        ▼                    ▼                    ▼
                    Rectified             Moving RMS          Threshold +
                    envelope                                  hysteresis
                        │                    │                    │
                        └────────────┬───────┴────────────────────┘
                                     ▼
                       Tkinter / Matplotlib GUI
                       Time trace / FFT / status
                                     │
                            CSV snapshots and
                          labeled flex trial files
                                     │
                              [motor GUI only]
                           MOTOR:<0–100> commands
                                     │
                           Experimental L293D sketch
```

The simple serial-streaming firmware and the read-only plotter are the **recommended starting point**. No subject recordings or clinical data are included.

## Files

| Path | Contents |
| --- | --- |
| [`src/python/emg_serial_plotter.py`](src/python/emg_serial_plotter.py) | Live EMG visualization and analysis; control modes are simulated in the GUI |
| [`src/python/emg_motor_control.py`](src/python/emg_motor_control.py) | Similar GUI with serial `MOTOR:<percent>` output added |
| [`firmware/EMG_Servo_2TH/EMG_Servo_2TH.ino`](firmware/EMG_Servo_2TH/EMG_Servo_2TH.ino) | Minimal Arduino analog-reader sketch that emits timestamped EMG samples |
| [`firmware/experimental/LED_L293D_v3/LED_L293D_v3.ino`](firmware/experimental/LED_L293D_v3/LED_L293D_v3.ino) | Experimental LED-bar and dual-motor control based on ADC thresholds |
| [`firmware/experimental/To_share/To_share.ino`](firmware/experimental/To_share/To_share.ino) | Experimental motor command receiver and PWM mapping; **see critical motor warning below** |
| [`requirements.txt`](requirements.txt) | Host Python dependencies |
| [`.gitignore`](.gitignore) | Ignores local data outputs, virtual environments, and private config |

The source filenames were normalized for the repository; the underlying code was preserved except for replacing a personal hard-coded desktop path with a user-independent one and generalizing a personalized note in a help dialog.

## Quick start — safe read-only acquisition

1. Wire a suitable conditioned analog EMG output to the Arduino's **A0**, with the required reference and a hardware-safe power arrangement. Follow the sensor's manufacturer guidance. **Never attach electrodes directly to a microcontroller pin or exposed mains-connected circuitry.**
2. Open `firmware/EMG_Servo_2TH/EMG_Servo_2TH.ino` in the Arduino IDE and upload it to a compatible board.
3. Create a Python environment and install the host packages:

   ```bash
   python -m venv .venv
   # Windows: .venv\Scripts\activate
   # macOS/Linux: source .venv/bin/activate
   python -m pip install -r requirements.txt
   ```

4. Run the **read-only** application:

   ```bash
   python src/python/emg_serial_plotter.py
   ```

5. Select **Start** in the desktop window; the program scans available serial ports for lines matching `<milliseconds>,<ADC integer>` at **115200 baud**. Review the raw, rectified/envelope and RMS plots, and use **Calibrate Rest (3s)** to establish a threshold estimate.

This GUI requires a graphical Tk/TkAgg environment, a connected serial source, and a writable **Desktop** folder. The code does not include a standalone signal simulator. The Arduino sketch uses a fixed delay, so **actual sample throughput is hardware-dependent**; the GUI's `SAMPLE_RATE_HZ = 1000` is a nominal FFT assumption, not a verified measured sampling rate. The serial-only firmware comment mentions approximately 2 kHz; this should not be taken as a measured rate either.

## Signal-processing pipeline

- **Offset tracking:** moving raw ADC baseline to recenter the signal; normalized around an assumed 10-bit ADC span.
- **Rectification and envelope:** absolute amplitude followed by an approximately 30-sample moving average.
- **RMS:** approximately 100-sample rolling root-mean-square amplitude.
- **Activation detection:** rising threshold set by the user or from a three-second rest sample (`mean + 3 × std`); falling threshold at 70% of the upper threshold to reduce rapid toggling.
- **Visualization:** sliding-window plots of raw amplitude, envelope, RMS and binary activity, plus an FFT option using an assumed sample rate.
- **Experiments:** CSV exports with `time`, `emg_raw`, `emg_rect`, `emg_env`, `emg_rms`, and `binary_active`; an additional one-second `flex` trial exporter.
- **Interaction modes:** pulse (single/double/long event logic), angle, speed, and strength modes. In the plotter-only GUI, these update software indicators only; the separate motor-control GUI sends serial motor output.

The displayed quality score and motion-artifact indicator are **simple heuristics**, not validated measures of EMG signal quality or clinical signal-to-noise ratio.

## Motor-control variant — important warning

The application in `src/python/emg_motor_control.py` emits `MOTOR:<0–100>\n` commands over the serial connection. The prototype in `firmware/experimental/To_share/To_share.ino` parses this command for an L293D motor-driver circuit and maps it to PWM.

**Do not load the experimental motor sketches onto a connected actuator without a code and wiring review.** In particular:

- `To_share.ino` contains an **unconditional `analogWrite(ENA_PIN, 100)` in `loop()`**; this can energize the motor even with no user command, and interfere with commanded zero output. It is not a safe command-controlled implementation as provided.
- The experimental motor firmware has **no verified command-loss watchdog, emergency-stop interlock, current limiting, or full fail-safe behavior**.
- `LED_L293D_v3.ino` switches motor directions directly from raw thresholds and has hardware-dependent outputs; it is **not** a drop-in replacement for the serial-command firmware.
- The desktop **Stop** function attempts to send zero output, but this cannot substitute for a hardware-enforced motor disable.

Only test these sketches in a controlled, physically isolated setup after removing the unconditional drive behavior, adding appropriate fail-safes, and verifying all circuit connections. They are included **for transparent technical review**, not as ready-to-run motor firmware.

## Limitations and reproducibility

- Analog sensor wiring, board model, acquisition precision, and detailed calibration hardware are not specified in the archive.
- Input framing is a simple text serial stream rather than a fault-tolerant data protocol; invalid lines are skipped.
- Tkinter updates and serial access share state across threads with limited synchronization.
- No unit/integration test suite, sample recordings, oscilloscope validation, or measured latency/error results are included.
- The current applications assume a desktop environment, and data collection creates local time-stamped CSV files. These **measurements should not be committed to a public repository without consent and de-identification**.
- The available code is experimental; do not use it for clinical diagnosis or as a production prosthetic/assistive control system.

## Background

This repository documents embedded signal acquisition and exploratory biomedical-control software. It demonstrates integration across **microcontroller ADC acquisition, serial interfaces, Python signal processing, real-time plotting, and command-driven actuator experiments**, while keeping hardware verification status explicit.

**Licensing:** no license was supplied with the archive. Do not assume permission for commercial redistribution or modification by third parties without authorization.
