/** @file   ASDeviceFactory.h
 *  @brief  Alphasmart device object factory.
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

#ifndef COM_TSONIQ_ASDeviceFactory_H
#define COM_TSONIQ_ASDeviceFactory_H   (1)

#include "ASDevice.h"


namespace ts
{
    /** Notification callback for the detection of a new device (initially in HID mode). The client
     *  can choose to either ignore the device or to request that it be switched to comms mode.
     *
     *  The client will receive enumeration callbacks either during calls to enable() or disable()
     *  or via a distinct run-loop callback event.
     *
     *  @param  context     The client context (registered with the factory at enable()).
     *  @param  ident       The device identity (has that uniquely identifies the device).
     *  @return             Logical true to open the device, or false to ignore it.  If true is returned
     *                      a subsequent callback will be issued to supply the device handle once the
     *                      device is ready.
     */
    typedef bool (*ASDeviceFactoryDetect)(class ASDeviceFactory* factory, void* context, unsigned ident);


    /** Callback to indicate that a device has been opened and is ready for communication handling.
     *
     *  The client will receive enumeration callbacks either during calls to enable() or disable()
     *  or via a distinct run-loop callback event.
     *
     *  @param  context     The client context (registered with the factory at enable()).
     *  @param  ident       The device identity (as for ASDeviceFactoryDetection)
     *  @param  device      The device handle.
     */
    typedef void (*ASDeviceFactoryConnect)(class ASDeviceFactory* factory, void* context, unsigned ident, ASDevice* device);


    /** Callback to indicate that a device has been closed (unplugged) and may no longer be used.
     *
     *  The client will receive enumeration callbacks either during calls to enable() or disable()
     *  or via a distinct run-loop callback event.
     *
     *  @param  context     The client context (registered with the factory at enable()).
     *  @param  ident       The device identity (as for ASDeviceFactoryDetection)
     *  @param  device      The device handle.
     */
    typedef void (*ASDeviceFactoryDisconnect)(class ASDeviceFactory* factory, void* context, unsigned ident, ASDevice* device);


    /** The device factory. This manages USB plug-and-play operation and the transformation from
     *  a HID device to a comms mode device.
     */
    class ASDeviceFactory
    {
    public:

        ASDeviceFactory();
        ~ASDeviceFactory();

        bool enable(void* context, ASDeviceFactoryDetect detect, ASDeviceFactoryConnect connect, ASDeviceFactoryDisconnect disconnect);
        void disable();

        /** Return logical true if the factory is currently enabled.
         */
        bool isEnabled() const
        {
            return 0 != m_usb;
        }

    private:

        class ASDeviceFactoryUSB* m_usb;       /**< USB context (separated to isolate this header from OS dependencies). */

        friend class ASDeviceFactoryUSB;

        ASDeviceFactory(const ASDeviceFactory&);              /**< Prevent the use of the copy constructor. */
        ASDeviceFactory& operator=(const ASDeviceFactory&);   /**< Prevent the use of the assignment operator. */
    };

}   /* namespace */

#endif      /* COM_TSONIQ_ASDeviceFactory_H */
