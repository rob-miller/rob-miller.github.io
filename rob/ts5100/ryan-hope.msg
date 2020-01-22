Hey I noticed you mentioned a needed update to your site so I figured I 
would send you some stuff you could include.

I attached a quick patch I made to drivers/pci/quirks.c in the 2.6 
kernel that unhides/enables the SMBus controller on some of our laptops. 
You might notice you do not have this in your lspci output.

0000:00:1f.3 SMBus: Intel Corp. 82801CA/CAM SMBus Controller (rev 02)
        Subsystem: Toshiba America Info Systems: Unknown device 0001
        Flags: medium devsel
        I/O ports at ef00 [size=32]

With this patch i2c support works with i2c-i801 lm90 eeprom.


The other thing I wanted to mention if you didnt already know, for some 
reason in the new nvidia drivers xorg/xfree cant display at 1600x1200 
even with the ignoreEDID and NVreg_Mobile. The best fix has been this 
mode line:

Modeline "1588x1200" 162 1588 1664 1856 2160 1200 1201 1204 1250

--Ryan
