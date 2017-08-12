# About

Collection of High Altitude Balloon projects for Zinoo science center:

 * `zinoo-liepaja`: Custom built ATMega328p based payload tracker firmware.
 * `zinoo-lora`: LoRa-based payload tracker firmware. It uses Arduino Uno + Dragino LoRa/GPS shield.
 * `zinoo-lora-ground`: LoRa-based ground station firmware. It uses Arduino Uno + Dragino LoRa/GPS shield.

# Getting Started

Please install PlatformIO to build the firmware (see [Installation guide](http://docs.platformio.org/en/latest/installation.html)), e.g. by executing

```bash
pip install -U platformio
```

To use the automatic upload script, `pyserial` library is required. You can install it by executing

```bash
pip install -U pyserial
```

Note that you might want to add `sudo -H` in front of these commands, depending on your setup.

# Building and uploading

To build `zinoo-lora` and `zinoo-lora-ground`, change directory to the corresponding project and execute

```bash
pio run
```

The `zinoo-lora` project builds several variants of the firmware (see the project definition file `zinoo-lora/platformio.ini`). To upload the code to an Arduino board, connect it and execute 

```bash
pio run -t upload -e <variant>
```

Where `<variant>` is currently one of `z71`, `z72`, `z73` or `z74`. Variants are defined in the project definition file.

To upload the `zinoo-lora-ground` project, you can simply execute

```bash
pio run -t upload
```

# Upload to *habhub*

The script `habhub-upload.py` can be used to automatically upload received telemetry to the habhub platform. To do that, you need to specify the receiver callsign and the serial port device of the connected Arduino board, e.g.

```bash
python habhub-upload.py GROUND-1 /dev/ttyUSB0
```

The script will echo all strings received from the serial port, as well as automatically attempt to upload strings beginning with `$$`. All displayed information is also logged into a timestamped logfile, which is created in the current working directory.
