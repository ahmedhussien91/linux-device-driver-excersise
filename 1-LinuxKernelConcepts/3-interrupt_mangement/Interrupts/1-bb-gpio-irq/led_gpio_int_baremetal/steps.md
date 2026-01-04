# building

```bash
. ../../../../../prepareEnv/bb-setup/setup_bb.sh
make
# deploy
sudo cp baremetal.bin /srv/tftp/
```

# running on ECU

on uboot

```bash
tftpboot 0x80000000 baremetal.bin
go 0x80000000
```


# problems
program reset and starts linux after a while

### what is really happening

U-Boot
  ↓ go 0x80008000
Bare-metal app runs
  ↓ (لا تعود لـ U-Boot)
بعد فترة
  ↓
Reset / Watchdog
  ↓
U-Boot يبدأ من جديد
  ↓
Linux boots

### solution

from uboot turn off watch dog
```c
// Watchdog base (AM335x):
#define WDT_BASE   0x44E35000

// Sequence التعطيل (من TRM):
#define WDT_WSPR   (*(volatile unsigned int *)(WDT_BASE + 0x48))
#define WDT_WWPS   (*(volatile unsigned int *)(WDT_BASE + 0x34))

static void wdt_disable(void)
{
    WDT_WSPR = 0xAAAA;
    while (WDT_WWPS & (1 << 4));

    WDT_WSPR = 0x5555;
    while (WDT_WWPS & (1 << 4));
}

int main(void)
{
    wdt_disable();
    ...
}
```

# BeagleBone Black – Bare-Metal GPIO Interrupt (Teaching Lab)

## Overview
This lab demonstrates **bare-metal GPIO interrupts** on the **BeagleBone Black (AM335x / Cortex-A8)** without Linux.

The goal is to teach the **complete interrupt path**:

GPIO (edge detect) → AINTC → ARM CPU → Vector Table → ISR → Acknowledge

The project is intentionally **verbose** and includes **debug UART prints** to make every step observable and teachable.

---

## Hardware Setup

### Board
- BeagleBone Black (AM335x)

### Connections
| Signal | Header Pin | SoC Signal | Description |
|------|-----------|-----------|-------------|
| Button | P8.12 | GPIO1_12 | Push button to GND (uses internal pull-up) |
| LED | P8.11 | GPIO1_13 | External LED or onboard test LED |
| UART | J1 / UART0 | UART0 | Debug output |

> Button wiring:
> **P8.12 → Button → GND**
> (Internal pull-up enabled via pinmux)

---

## Software Architecture

### Files
