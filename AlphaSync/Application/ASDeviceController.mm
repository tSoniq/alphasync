/** @file   ASDeviceController.mm
 *  @brief  Top-level controller for devices. There is a single instance of the controller. It manages
 *          all device plug/unplug events and device enumeration and access.
 *
 *
 *  Copyright (c) 2008-2013, tSoniq.
 *
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 *  	*	Redistributions of source code must retain the above copyright notice, this list of
 *  	    conditions and the following disclaimer.
 *  	*	Redistributions in binary form must reproduce the above copyright notice, this list
 *  	    of conditions and the following disclaimer in the documentation and/or other materials
 *  	    provided with the distribution.
 *  	*	Neither the name of tSoniq nor the names of its contributors may be used to endorse
 *  	    or promote products derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 *  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 *  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#import "ASDeviceController.h"
#import "ASPreferenceController.h"
#import "ASAppletID.h"

using namespace ts;



#pragma mark Forward Delarations


@interface ASDeviceController(PrivateAPI)
- (void)deviceDetected;
- (void)deviceConnected:(ASDevice*)device;
- (void)deviceDisconnected:(ASDevice*)device;
@end





#pragma mark C++ callback bindings


static bool deviceDetect(class ASDeviceFactory* factory, void* context, unsigned ident)
{
    // Return true to switch the device to comms mode, or false to leave it as it is. This can be
    // used to prevent repeated capture of the same device (looping between HID and COMMS mode).
    (void)factory;
    (void)ident;
    ASDeviceController* controller = (ASDeviceController*)context;
    [controller deviceDetected];
    return true;
}


static void deviceConnect(class ASDeviceFactory* factory, void* context, unsigned ident, ASDevice* device)
{
    (void)factory;
    (void)ident;
    ASDeviceController* controller = (ASDeviceController*)context;
    [controller deviceConnected:device];
}


static void deviceDisconnect(class ASDeviceFactory* factory, void* context, unsigned ident, ASDevice* device)
{
    (void)factory;
    (void)ident;
    ASDeviceController* controller = (ASDeviceController*)context;
    [controller deviceDisconnected:device];
}




#pragma mark ASDeviceController(PrivateAPI)



@implementation ASDeviceController(PrivateAPI)


/** Callback on detection of a new device (in HID mode).
 */
- (void)deviceDetected
{
    [notificationCenter postNotificationName:kASDeviceControllerDeviceDetected object:nil];
}


/** Callback on connection of a new device (in comms mode).
 *
 *  @param  device      The device handle.
 */
- (void)deviceConnected:(ASDevice*)device
{
    ASDeviceNode* deviceNode = [[[ASDeviceNode alloc] initWithDevice:device] autorelease];
    if (deviceNode)
    {
        [rootNode addChild:deviceNode];
        [notificationCenter postNotificationName:kASDeviceControllerDeviceConnected object:deviceNode];
    }
}


/** Callback on disconnection of a device.
 *
 *  @param  device      The device handle.
 */
- (void)deviceDisconnected:(ASDevice*)device
{
    NSEnumerator* deviceEnumerator = [rootNode childEnumerator];
    while (ASDeviceNode* deviceNode = [deviceEnumerator nextObject])
    {
        if ([deviceNode device] == device)
        {
            [rootNode removeChild:deviceNode];
            [notificationCenter postNotificationName:kASDeviceControllerDeviceDisconnected object:deviceNode];
            break;
        }
    }
}


@end




#pragma mark    ASDeviceController



@implementation ASDeviceController

/** Designated initialiser. Only one instance of the class should be created at a time, and only when
 *  the low level driver functions should be enabled.
 */
- (id)init
{
    if ((self = [super init]))
    {
        notificationCenter = [NSNotificationCenter defaultCenter];
        rootNode = [[ASNode alloc] init];
        deviceFactory = new ASDeviceFactory;
        if (!rootNode || !deviceFactory)
        {
            // This is fatal to the entire application - there is no way to recover.
            NSLog(@"AlphaSync: unable to initialise device factory");
            self = nil;
        }
    }
    return self;
}


/** Destructor. As well as releasing resources, this will shutdown the driver and reset each connected
 *  device instance (putting it back in to HID mode).
 */
- (void)dealloc
{
    [rootNode release];     // will invoke the destructors for each contained device node, reseting the devices
    if (deviceFactory) delete deviceFactory;
    [super dealloc];
}


/** Enable the driver.
 */
- (BOOL)enable
{
    return deviceFactory->enable(self, deviceDetect, deviceConnect, deviceDisconnect) ? TRUE : FALSE;
}


/** Disable the driver.
 */
- (void)disable
{
    deviceFactory->disable();
}


/** Return the root node for the hardware tree.
 */
- (ASNode*)systemRoot
{
    return rootNode;
}


/** Request each attached device to reload its data.
 */
- (void)reloadAllDevices
{
    NSEnumerator* enumerator = [rootNode childEnumerator];
    ASDeviceNode* deviceNode;
    while ((deviceNode = [enumerator nextObject]))
    {
        [deviceNode reload];
    }
}


/** Close a device, returning it to HID mode.
 */
- (void)closeDevice:(ASDeviceNode*)deviceNode
{
    ASDevice* device = [deviceNode device];
    device->restart();
}


/** Close all connected devices.
 */
- (void)closeAllDevices
{
    NSEnumerator* enumerator = [rootNode childEnumerator];
    ASDeviceNode* deviceNode;
    while ((deviceNode = [enumerator nextObject]))
    {
        [self closeDevice:deviceNode];
    }
}



@end
