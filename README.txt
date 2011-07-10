==========================================================
TeensyRawhid: a Python Raw HID interface for Teensy boards
==========================================================

Ian Howson (ian@mutexlabs.com)

http://mutexlabs.com/c/TeensyRawhid/

https://github.com/ihowson/Teensy-Raw-HID-in-Python

Introduction
------------

The Teensy boards (http://www.pjrc.com/teensy/index.html) are extremely useful AVR development boards with a USB interface. The website provides some great C sample code including a 'Raw HID' interface (http://www.pjrc.com/teensy/rawhid.html) -- essentially, providing the ability to read and write 64 byte packets to and from the device. 

This module allows Python to talk to the Raw HID interface. It provides roughly the same API as the C rawhid interface: ``open``, ``close``, ``send``, ``recv``. 

I've tested the module on a Teensy++ 2.0, but it should work on all Teensy types.

Usage
-----

Create a Rawhid object::

    import TeensyRawhid
    rh = TeensyRahwid.Rawhid()

Open a USB device that uses the Raw HID protocol. By default, it will try to open any connected Teensy++ devices::

    rh.open()

but you can also specify some other device::

    rh.open(vid=0x16c0, pid=0x0478) # open a HalfKay device

Send it a frame::

    frame = 'a' * 64
    rh.send(frame, 50) # 50ms timeout

Receive a frame::

    rh.recv(50) # 50ms timeout

Close the device when you're done::

    rh.close()

More information is available in the API reference.

Manifest
--------

teensyrawhid/

    The implementation. The C side of this is adapted from PJRC's rawhid implementation. I've made a couple of tweaks and added the Python interface code.

teensyrawhid/test/

    Test suite. The test builds a test stub to put on the Teensy, loads it on and talks to the board a bit. As a result, there are a lot of dependencies. So far, I've only run this on Linux.

teststub/

    ``make install`` will copy the hexfile to the device using ``teensy_loader_cli``. You must do this before running the unit tests.
    
teensy_loader_cli/

    Unmodified ``teensy_loader_cli`` utility from PJRC website (http://www.pjrc.com/teensy/loader_cli.html). This is included so that the unit tests can run with less user interaction.

udev rules
----------

The test script writes a test stub with the USB ID of 0x16c0/0x8000. With the udev rules supplied on the Teensy website, this device will only be accessible by root. I recommend that you add another udev rule that looks like::

    SUBSYSTEMS=="usb", ATTRS{idVendor}=="16c0", ATTRS{idProduct}=="8000", MODE:="0666"

For convenience, I've bundled a tweaked version of the Linux udev rules in the ``teensy_loader_cli`` directory.

Download
--------

For now, use the github link to download the source.

