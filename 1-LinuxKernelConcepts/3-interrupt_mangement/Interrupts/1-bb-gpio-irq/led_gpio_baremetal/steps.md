# building

```bash
. ../../../../../prepareEnv/bb-setup/setup_bb.sh
make
# deploy
sudo cp baremetal.bin /srv/tftp/
```

# running on ECU


```bash
tftpboot 0x80008000 baremetal.bin
go 0x80008000
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
