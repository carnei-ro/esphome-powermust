# esphome-powermust

![GitHub actions](https://github.com/syssi/esphome-powermust/actions/workflows/ci.yaml/badge.svg)
![GitHub stars](https://img.shields.io/github/stars/syssi/esphome-powermust)
![GitHub forks](https://img.shields.io/github/forks/syssi/esphome-powermust)
![GitHub watchers](https://img.shields.io/github/watchers/syssi/esphome-powermust)
[!["Buy Me A Coffee"](https://img.shields.io/badge/buy%20me%20a%20coffee-donate-yellow.svg)](https://www.buymeacoffee.com/syssi)

ESPHome component to monitor and control a Mustek PowerMust 800 UPS via RS232

## Supported devices

* Mustek PowerMust 800 USB
* Intelbras XNB 720 BI (Board **BX500-1500 SMT**. Megatec Q1 over the internal UPS UART)

## Untested devices

* TBD.

## Requirements

* [ESPHome 2024.6.0 or higher](https://github.com/esphome/esphome/releases).
* Generic ESP32/ESP8266 board

## Schematics

```
                 RS232                     UART-TTL
┌───────────┐              ┌──────────┐                ┌─────────┐
│           │              │          │<----- RX ----->│         │
│           │<---- TX ---->│  RS232   │<----- TX ----->│ ESP32/  │
│ Powermust │<---- RX ---->│  to TTL  │<----- GND ---->│ ESP8266 │
│           │<---- GND --->│  module  │<-- 3.3V VCC -->│         │<--- VCC
│           │              │          │                │         │<--- GND
└───────────┘              └──────────┘                └─────────┘
```

### D-SUB 9P connector

| Pin | Purpose      | MAX3232 pin       |
|:---:| :----------- | :---------------- |
|  1  |              |                   |
|  2  | TX           | P13 (RIN1)        |
|  3  | RX           | P14 (DOUT1)       |
|  4  |              |                   |
|  5  | GND          | P15 (GND)         |
|  6  |              |                   |
|  7  |              |                   |
|  8  |              |                   |
|  9  |              |                   |

### MAX3232

| Pin          | Label        | ESPHome     | ESP8266 example  | ESP32 example |
| :----------- | :----------- | :---------- | :--------------- | :------------ |
| P11 (DIN1)   | TXD          | `tx_pin`    | `GPIO4`          | `GPIO16`      |
| P12 (ROUT1)  | RXD          | `rx_pin`    | `GPIO5`          | `GPIO17`      |
| P16 (VCC)    | VCC          |             |                  |               |
| P15 (GND)    | GND          |             |                  |               |

## Installation

You can install this component with [ESPHome external components feature](https://esphome.io/components/external_components.html) like this:
```yaml
external_components:
  - source: github://syssi/esphome-powermust@main
```

or just use the `esp32-example.yaml` as proof of concept:

```bash
# Install esphome
pip3 install esphome

# Clone this external component
git clone https://github.com/syssi/esphome-powermust.git
cd esphome-powermust

# Create a secrets.yaml containing some setup specific secrets
cat > secrets.yaml <<EOF
wifi_ssid: MY_WIFI_SSID
wifi_password: MY_WIFI_PASSWORD

mqtt_host: MY_MQTT_HOST
mqtt_username: MY_MQTT_USERNAME
mqtt_password: MY_MQTT_PASSWORD
EOF

# Validate the configuration, create a binary, upload it, and start logs
# If you use a esp8266 run the esp8266-examle.yaml
esphome run esp32-example.yaml

```

## Local usage with an Intelbras XNB 720 BI / NodeMCU

This checkout can also talk to the UPS UART directly (5 V TTL through a
TXS0102 level shifter), without an RS232 transceiver. The XNB 720 BI ignores
the first command sent with an idle line: it needs a blank CR before each
command. After that warm-up it answers to both CR and CRLF:

```yaml
external_components:
  - source:
      type: local
      path: esphome-powermust/components

uart:
  id: ups_uart
  tx_pin: GPIO14   # D5
  rx_pin: GPIO12   # D6
  baud_rate: 2400

powermust:
  - id: ups
    uart_id: ups_uart
    update_interval: 5s
    # Blank CR sent before every polling/control command.
    command_prefix: "\r"
    command_terminator: "\r\n"
```

`command_prefix` defaults to `""` (no prefix) and `command_terminator`
defaults to `"\r"` (Megatec standard). The component accepts responses
terminated by CR, LF or CRLF and ignores noise before the `(` marker of a Q1
response.

The F response of the XNB 720 BI uses a decimal current (`#120.0 3.0 12.00
60.0`), which the component handles since the rating current sensor is a
float.

GPIO14/GPIO12 use SoftwareSerial on the ESP8266, so UART0 stays available
for the USB log:

```yaml
logger:
  baud_rate: 115200
```

If UART0 (GPIO1/GPIO3) is used for the UPS instead, set `baud_rate: 0` on
the logger because both would share the same serial pins.

## Example response all sensors enabled

```
TBD.
```

## Protocol

See [https://networkupstools.org/protocols/megatec.html](networkupstools.org/protocols/megatec.html).

## Known issues

None.

## Debugging

If this component doesn't work out of the box for your device please update your configuration to increase the log level to see details about the UART traffic:

```
logger:
  level: DEBUG

uart:
  - id: uart_0
    baud_rate: 2400
    tx_pin: ${tx_pin}
    rx_pin: ${rx_pin}
    debug:
      direction: BOTH
      dummy_receiver: false
      after:
        delimiter: "\r"
      sequence:
        - lambda: UARTDebug::log_string(direction, bytes);
```

## References

* https://networkupstools.org/protocols/megatec.html
