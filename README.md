# Motion-Sensing-Using-WiFi
It's just a basic homemade thing, I have though whether I can use the WiFi as my motion sensing medium as I saw  on  online. So, I gave it a try. Use it as you please

# Manual Wi-Fi signal-change test

This firmware measures changes in the RSSI reported for one nearby Wi-Fi
access point. It cannot confirm that a person caused a change or identify
anyone.

## Setup

1. Use a stationary, powered Wi-Fi access point and place the ESP32-S3 where
   the radio path between the access point and board crosses the area to
   monitor.
2. For repeatable results, set `TARGET_SSID` in `src/main.cpp` to that
   access point's SSID. If left empty, the firmware chooses the strongest
   visible access point when it first scans.
3. Build and upload with PlatformIO using the `esp32-s3-n16r8` environment.
4. Open the serial monitor at 115200 baud and keep the monitored area clear
   while the 12-sample baseline is collected.

## Checks

- Confirm the serial output names the expected SSID and BSSID.
- Wait for `Baseline set ...`, then keep the area clear. Readings should
  generally remain in `monitoring`.
- Move through or obstruct the monitored radio path for several readings.
  A sustained RSSI change of at least 6 dB should produce
  `SIGNAL CHANGE DETECTED`.
- Clear the path and wait for readings to return within 3 dB of baseline;
  the output should report `signal returned near baseline`.
- Send `c` in the serial monitor to recalibrate with the area clear.

RSSI varies with multipath, people, doors, access-point traffic, and other
radio conditions. If the result is noisy, improve the placement or adjust
`RSSI_CHANGE_THRESHOLD_DB` and `RSSI_CLEAR_THRESHOLD_DB` in `src/main.cpp`.
<img width="3072" height="3072" alt="IMG_20261123_224554" src="https://github.com/user-attachments/assets/9ce1b034-6579-4a1a-a1bd-780bbbb7655d" />



https://github.com/user-attachments/assets/331cea4f-131c-417f-9bb7-078de2741791





https://github.com/user-attachments/assets/19b1086f-7345-4ca5-be68-2198dce43058


