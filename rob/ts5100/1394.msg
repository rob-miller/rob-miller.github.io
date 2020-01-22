Hi Rob,

I am a very confident user of debian (actually the simplyMEPIS distro) on  
my
Toshiba Satellite 5100-603. But until this day one thing really annoyed me:
Firewire simply didn't work. Since this laptop only supports USB 1.1  
devices,
accessing an 200GB external HD via USB (~400KB/s) was nothing but a  
challenge
for my patience.
But today I stumbled over a thread on sourceforge.net of someone who  
claims to
have been able to get firewire up and running on a 5100 series device:

http://sourceforge.net/mailarchive/forum.php?thread_id=6291404&forum_id=5389

I followed his instructions and couldn't believe my eyes, when I copied a  
video
file to my external disc with more than 20MB/s.

Here is what I've done:

The thread contains a patch for the 2.6.9 kernel:

--- drivers/ieee1394/ohci1394.c.orig	2003-04-05 17:28:40.000000000 +0900
+++ drivers/ieee1394/ohci1394.c	2003-04-08 20:16:48.000000000 +0900
@@ -2027,12 +2027,22 @@
  	unsigned long ohci_base, ohci_len;
  	int i;
  	
+	u16	data;
+
  	if (version_printed++ == 0)
  		PRINT_G(KERN_INFO, "%s", version);

+	pci_read_config_word(dev,PCI_CACHE_LINE_SIZE,&data);
          if (pci_enable_device(dev))
  		FAIL(-ENXIO, "Failed to enable OHCI hardware %d",
  		        card_id_counter++);
+
+	mdelay(10);
+	pci_write_config_word(dev,PCI_CACHE_LINE_SIZE,data);
+	pci_write_config_word(dev,PCI_INTERRUPT_LINE,dev->irq);
+	pci_write_config_dword(dev,PCI_BASE_ADDRESS_0,pci_resource_start(dev,  
0));
+	pci_write_config_dword(dev,PCI_BASE_ADDRESS_1,pci_resource_start(dev,  
1));
+
          pci_set_master(dev);

  	host = hpsb_alloc_host(ohci1394_driver, sizeof(struct ti_ohci));

But since I'm running a 2.6.8 this patch did not work for me automatically  
(patch -p1).
So I decided to edit the file drivers/ieee1394/ohci1394.c manually by  
filling in the
lines after the '+'es below line 3189 in ieee1394.c.
After recompiling and rebooting it did work flawlessly.
"modprobe ohci1394" loads the driver and when I connect the external HD it  
is
recognized and added to the fstab automatically.

I am writing this to you because there are lots of 5100series users all  
over the
world waiting for a solution for the firewire problem and your site is  
imho the
best and probably most visited linux on 5100 site.

If you have any further questions about this issue feel free to write  
back. I'll
try to answer as good as possible.

Kind regards,
              Oliver Koenig

-- 
Oliver Koenig, student of telematics, TUGraz, Austria

"In a world without walls and fences,
  who needs windows and gates?"
