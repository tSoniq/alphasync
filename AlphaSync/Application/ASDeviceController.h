/** @file       ASDeviceController.h
 *  @brief      Top-level controller for devices. There is a single instance of the controller. It manages
 *              all device plug/unplug events and device enumeration and access.
 *
 *  Copyright (c) 2008-2013, tSoniq. http://tsoniq.com
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
 *
 * The device controller posts the following notifications to the default notification center:
 *
 *      kASDeviceControllerDeviceDetected       New HID device is visible; starting transition to COMMS device.
 *      kASDeviceControllerDeviceConnected      New device has appeared. Receiver should re-parse the object tree.
 *      kASDeviceControllerDeviceDisconnected   New device has appeared. Receiver should re-parse the object tree.
 */
#import <Cocoa/Cocoa.h>
#import "ASNode.h"
#import "ASDeviceNode.h"
#import "ASAppletNode.h"
#import "ASFileNode.h"
#import "ASDevice.h"
#import "ASDeviceFactory.h"

#define kASDeviceControllerDeviceDetected       @"ASDeviceDetected"     /**< Notification name for detect events. */
#define kASDeviceControllerDeviceConnected      @"ASDeviceConnected"    /**< Notification name for connect events. */
#define kASDeviceControllerDeviceDisconnected   @"ASDeviceDisconnected" /**< Notification name for disconnect events. */


@interface ASDeviceController : NSObject
{
    ts::ASDeviceFactory* deviceFactory;         /**< Single instance of the device factory. */
    ASNode* rootNode;                           /**< Contains the set of ASDeviceNodes corresponding to connected devices. */
    NSNotificationCenter* notificationCenter;
}

- (BOOL)enable;
- (void)disable;

- (ASNode*)systemRoot;

- (void)reloadAllDevices;

- (void)closeDevice:(ASDeviceNode*)deviceNode;
- (void)closeAllDevices;

@end
