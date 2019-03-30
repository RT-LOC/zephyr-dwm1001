# DWM1001 & Zephyr
This project contains examples that show how to use the DWM1001 module together with Zephyr RTOS.
The examples that decawave distributes along with their driver are adapted to work on the DWM1001-board.

## Getting Started

### What's required?
#### Hardware
You will need at least one `DWM1001-dev` board and a `micro-USB` cable.

#### Software
Make sure you have `CMake` and `ninja` installed on your PC. Some installation guidelines are added below.
In order to flash the boards, you will need `nrfjprog`. This program is added to this repository so that you don't need to install it explicitely (but you can of course).

#### CMake
TODO

#### Ninja
TODO

### Zephyr 
First you need to make sure you have downloaded & installed Zephyr with the DWM1001 BSP.

Clone the zephyr distribution: 
```
git clone https://github.com/RT-LOC/zephyr
```

Now change your active directory:
```
cd zephyr
```

The BSP is in the branch `origin/DWM1001`. We need to checkout this branch in order to be able to use the BSP.
```
git checkout DWM1001
```

Now source the script zephyr-env.sh to make sure all the environment variables are set correctly.
```
source zephyr-env.sh
```

### Build your first application
Make sure you have downloaded or cloned this repository to your local computer:
```
git clone https://github.com/RT-LOC/zephyr-dwm1001
```

Now that we have installed zephyr, we can start building the real examples.
We will proceed here with building the first simple example. Note that the procedure to follow for all the other examples is identical.

```
cd examples/ex_01a_simple_tx
mkdir build
cd build
```

```
cmake -GNinja -DBOARD=nrf52_dwm1001 ..
```

```
ninja
```


### Flash
Now let's flash the binary file that we just built onto the board. 
Note that you should change the path to `nrfjprog` according to the operating system you're using. This is the example for linux.
#### Erase the flash:
```
 ../../tools/windows/nrfjprog/nrfjprog --family nrf52 --eraseall
```

#### Program the binary file on the board:
```
../../tools/windows/nrfjprog/nrfjprog --family nrf52 --program build/zephyr/zephyr.hex
```

#### Perform a soft reset of the MCU:
```
../../tools/windows/nrfjprog/nrfjprog --family nrf52 --reset
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

## What's next?
