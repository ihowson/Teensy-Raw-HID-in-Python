#!/usr/bin/env python

'''
Copyright (c) 2011 Ian Howson (ian@mutexlabs.com)

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
'''

# Test script for TeensyRawhid
#
# Builds the module and runs tests. Tests that do not depend on a device run
# first.
# 
# I've only run this on Linux, so far.

import os
import sys
import time

INVALID_VID = 0x1111

STUB_VID = 0x16c0
STUB_PID = 0x8000

HALFKAY_VID = 0x16c0
HALFKAY_PID = 0x0478

# TODO: build the teensy loader and the test stub; add docs on the dependencies that this pulls in

# build the module
os.chdir('../..')
ret = os.system('./setup.py build')
if ret:
    print "Build failed with error %s" % ret
    sys.exit(1)

# install the module to the staging dir
ret = os.system('./setup.py install --home=test-staging')
if ret:
    print "Install failed with error %s" % ret
    sys.exit(1)

# add the staged module to our import path
sys.path.append('test-staging/lib/python/')

# try to import the new module
# TODO need something here to ensure that we do not import from the system path - it MUST be the staged copy
import TeensyRawhid

# run some offline tests
rh = TeensyRawhid.Rawhid()

rh.close() # there should be no exception thrown

try:
    rh.recv(50, 50) # 
    assert False, "object didn't barf when I tried to read when it wasn't open"
except IOError:
    # good!
    pass

try:
    rh.send("test", 50)
    assert False, "object didn't barf when I tried to send when it wasn't open"
except IOError:
    # good!
    pass

# that's about all we can do without talking to a real device
try:
    rh.open(vid=INVALID_VID)
    assert False, "object didn't barf when I tried to open a nonexistent device"
except IOError:
    # good!
    pass

# build the test stub
ret = os.system("make -C teststub")
assert ret == 0, "failed to build test stub"
# TODO: if the test stub has changed, force reloading it onto the device

# try to get the test stub loaded on the device

try:
    # NOTE: if you fail to open the device at this point even though you can
    # see the device listed (with 0x16c0/0x8000) in lsusb, check your udev
    # rules. You can also try re-running as root.
    rh.open(vid=STUB_VID, pid=STUB_PID) # this is the test stub
    # if we reach this point, it's running
except IOError, e:
    # Not running. Is there a HalfKay device there?
    try:
        rh.open(vid=HALFKAY_VID, pid=HALFKAY_PID, usage_page=-1, usage=-1)
        rh.close()
    except IOError:
        # Nope, no HalfKay. Ask the user to fix it.
        print "Please connect the Teensy board and press the reset button"
        while True:
            try:
                rh.open(vid=HALFKAY_VID, pid=HALFKAY_PID, usage_page=-1, usage=-1)
                rh.close()
                print
                break # found it!
            except IOError:
                print >>sys.stderr, '.',
                time.sleep(0.5)

    # program the test stub
    print "Writing test stub firmware..."
    ret = os.system("teensy_loader_cli/teensy_loader_cli -mmcu=at90usb1286 teststub/example.hex")
    assert ret == 0, "failed to write test stub to Teensy"

    # NOTE: if you get 'device not found', you might need to increase this delay. It gives the Teensy time to reboot and be redetected.
    time.sleep(2.0)
    #os.system('lsusb') # this might help with debugging

    # try opening it again
    rh.open(vid=STUB_VID, pid=STUB_PID)

# regular test case - open, send something, receive it back, close the device

# build input frame
inbuf = ''
for i in range(1, 65):
    inbuf += chr(i)
assert len(inbuf) == 64

rh.send(inbuf, 50)
outbuf = rh.recv(64, 50)
assert len(outbuf) == len(inbuf)

for index in range(len(outbuf)):
    assert ord(outbuf[index]) == (ord(inbuf[index]) + 1)

rh.close()
rh.close() # confirm that double-close is safe

print "All done!"

# TODO: more testcases that would be useful
# testcase: try open, recv (timeout), close
# testcase: try open, send (timeout), close. to induce timeout, use a magic value that the stub recognises
# testcase: confirm that double-opening a device will fail, then closing the first and opening again will work
# testcase: frames > 64 bytes
# testcase: controlling multiple open devices. Not really supported, yet.

