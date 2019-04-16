# DWM1001 & Zephyr
This project contains examples on how to use the Ultra Wideband (UWB) and Bluetooth hardware based DWM1001 module together with Zephyr RTOS. It's an adaptation of Decawave's examples distributed along with their driver.

Note upfront: this readme isn't finished yet. It will be improved & extended the following days/weeks.

## Getting Started

## What's required?
### OS
Linux, Mac or Windows!

### Hardware
You will need at least one `DWM1001-dev` board and a `micro-USB` cable.

### Software
There's quite a lot to install if you haven't already. First we're going to build the firmware, after which we can flash it on the board.

#### Building
Make sure you have `CMake` (min 3.13.1) and `ninja` installed on your PC. If you don't have these tools yet, follow the instructions from zephyr [here](https://docs.zephyrproject.org/latest/getting_started/index.html#set-up-a-development-system).
Note that you don't really need 'west'.

Next up is the `toolchain`. Instructions can be found here [here](https://docs.zephyrproject.org/latest/getting_started/index.html#set-up-a-toolchain). Best is to install GNU ARM Embedded as described. Pay attention not to install the toolchain into a path with spaces, and don't install version 8-2018-q4-major (both as described in the warnings). After configuring the environment variables you're almost good to go.

#### Flashing
In order to flash the boards, you will need `nrfjprog`. This tool is also available on all 3 main OS's. You can find it [here](https://www.nordicsemi.com/Software-and-Tools/Development-Tools/nRF5-Command-Line-Tools). After installing, make sure that your system PATH contains the path to where it is installed.
Typically for Windows this is:
```
C:\Program Files\Nordic Semiconductor\nrf5x\bin
```

### Zephyr 
Now that the development system is set up, we just need Zephyr and the examples itself. They are spread over two github repositories. You can start by downloading the Zephyr distribution that contains the DWM1001 BSP:

Download or clone the zephyr distribution: 
```
git clone https://github.com/RT-LOC/zephyr
```

Now change your active directory:
```
cd zephyr
```

Now source the script zephyr-env.sh (linux & macOS) or run zephyr-env.cmd to make sure all the environment variables are set correctly.

### Build your first application
The second github repository is the one that contains the specific DWM1001 example code.
Download or clone this repository to your local computer:
```
git clone https://github.com/RT-LOC/zephyr-dwm1001
```

Now that we have installed zephyr, we can start building the real examples.
We will proceed here with building the first simple example. Note that the procedure to follow for all the other examples is identical.

First let's create a build directory and jump to it.
```
cd examples/ex_01a_simple_tx
```
```
mkdir build
```
```
cd build
```
We configure the build system with Ninja as follows:
```
cmake -GNinja -DBOARD=nrf52_dwm1001 ..
```
And we actually build or firmware with ninja:
```
ninja
```

### Flash
Now let's flash the binary file that we just built onto the board. Make sure you have nrfjprog properly installed and that it is in the system PATH.

#### Erase the flash:
```
nrfjprog --family nrf52 --eraseall
```

#### Program the binary file on the board:
```
nrfjprog --family nrf52 --program zephyr/zephyr.hex
```

#### Perform a soft reset of the MCU:
```
nrfjprog --family nrf52 --reset
```

## Examples
The following examples are provided:
 - Example 1 - transmission
    - ex_01a_simple_tx
    - ex_01b_tx_sleep
    - ex_01c_tx_sleep_auto
    - ex_01d_tx_timed_sleep
    - ex_01e_tx_with_cca
 - Example 2 - reception
    - ex_02a_simple_rx
    - ex_02b_rx_preamble_64
    - ex_02c_rx_diagnostics
    - ex_02d_rx_sniff
    - ex_02e_rx_dbl_buff
 - Example 3 - transmission + wait for response
    - ex_03a_tx_wait_resp
    - ex_03b_rx_send_resp
    - ex_03c_tx_wait_resp_leds
    - ex_03d_tx_wait_resp_interrupts
 - Example 4 - continuous transmission
    - ex_04a_cont_wave
    - ex_04b_cont_frame
 - Example 5 - double-sided two-way ranging
    - ex_05a_ds_twr_init
    - ex_05b_ds_twr_resp
 - Example 6 - single-sided two-way ranging
    - ex_06a_ss_twr_init
    - ex_06b_ss_twr_resp
 - Example 7 - acknownledgements
    - ex_07a_ack_data_tx
    - ex_07b_ack_data_rx
 - Example 8 - low power listen
    - ex_08a_low_power_listen_rx
    - ex_08b_low_power_listen_tx
 - Example 9 - bandwidth power
    - ex_09a_bandwidth_power_ref_meas
    - ex_09b_bandwidth_power_comp
 - Example 10 - GPIO
    - ex_10a_gpio
 - Example 11 - IO
    - ex_11a_button
    - ex_11b_leds (coming)
 - Example 12 - BLE (coming)
    - ex_12a_ble 

## What's next?
* Examples completion
* (Mobile) readout app
