/** @file   ASDeviceFactory.cc
 *  @brief  Alphasmart device object factory.
 *
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
 */

#include <unistd.h>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/usb/IOUSBLib.h>
#include <mach/mach.h>
#include <assert.h>

#include "ASDeviceFactory.h"


#define kAQMaxDevices           (256)       /**< The maximum number of concurrently connected devices that can be handled. */
#define kAQHidUSBVendorID       (0x081e)    /**< USB Vendor ID for the Neo, operating as a keyboard. */
#define kAQHidUSBProductID      (0xbd04)    /**< USB Product ID for the Neo, operating as a keyboard. */
#define kAQComUSBVendorID       (0x081e)    /**< USB Vendor ID for the Neo, operating as a comms device. */
#define kAQComUSBProductID      (0xbd01)    /**< USB Product ID for the Neo, operating as a comms device. */


namespace ts
{
    #pragma mark    ---------------- Forward References ----------------

    static unsigned aq_deviceIdent(IOUSBDeviceInterface245** dev);
    static IOReturn aq_hidFlipToCommsMode(IOUSBDeviceInterface245** dev);
    void aq_hidDeviceAdded(void *refCon, io_iterator_t iterator);
    void aq_comDeviceAdded(void *refCon, io_iterator_t iterator);
    void aq_comDeviceRemoved(void *refCon, io_iterator_t iterator);




    #pragma mark    ---------------- ASDeviceUSB : MacOSX specific USB implementation ----------------



    /** Class definition.
     */
    class ASDeviceUSB : public ASDevice
    {
    public:

        ASDeviceUSB();
        ~ASDeviceUSB();

        bool init(io_service_t serviceHandle);

        /** Return the service handle passed at the call to init().
         */
        io_service_t service() const
        {
            return m_service;
        }

    protected:

        bool open();
        void close();

        virtual bool read(void* buffer, unsigned length, unsigned* actual, unsigned timeout);
        virtual bool write(const void* buffer, unsigned length, unsigned timeout);

    private:

        io_service_t m_service;                         /**< The service handle. */
        IOUSBDeviceInterface245** m_device;             /**< The device handle. */
        IOUSBInterfaceInterface245** m_interface;       /**< The interface handle. */
        unsigned m_pipeOut;                             /**< The output pipe. */
        unsigned m_pipeIn;                              /**< The input pipe. */
        unsigned m_timeout;                             /**< Timeout for IO operations, in ms. */
        FILE* m_debugRead;                              /**< Set to a non-zero handle to log all reads. */
        FILE* m_debugWrite;                             /**< Set to a non-zero handle to log all writes. */


        ASDeviceUSB(const ASDeviceUSB&);              /**< Prevent the use of the copy operator. */
        ASDeviceUSB& operator=(const ASDeviceUSB&);   /**< Prevent the use of the assignment operator. */
    };


    /** Constructor.
     */
    ASDeviceUSB::ASDeviceUSB()
        :
        ASDevice(),
        m_service(0),
        m_device(0),
        m_interface(0),
        m_pipeIn(0),
        m_pipeOut(0),
        m_timeout(20000),
        m_debugRead(0),
        m_debugWrite(0)
    {
        // Nothing
    }


    /** Destructor.
     */
    ASDeviceUSB::~ASDeviceUSB()
    {
        close();
    }


    /** Initialise the object.
     *
     *  @param  serviceHandle   The service handle.
     *  @return                 Logical true if the device could be initialised.
     */
    bool ASDeviceUSB::init(io_service_t serviceHandle)
    {
        m_service = serviceHandle;
        return open();
    }


    /** Open the service.
     *
     *  @return                 Logical true if the device could be opened.
     */
    bool ASDeviceUSB::open()
    {
        IOCFPlugInInterface **plugInInterface = 0;
        IOUSBDeviceInterface245 **dev = 0;
        IOUSBInterfaceInterface245 **intf = 0;
        SInt32 score;
        UInt8 numPipes;
        unsigned pipeIn = 0;
        unsigned pipeOut = 0;
        UInt8 direction, number, transferType, interval;
        UInt16 maxPacketSize;
        HRESULT result;
        kern_return_t status;

        status = IOCreatePlugInInterfaceForService(m_service, kIOUSBDeviceUserClientTypeID, kIOCFPlugInInterfaceID, &plugInInterface, &score);
        if ((kIOReturnSuccess != status) || !plugInInterface)
        {
            fprintf(stderr, "ASDeviceUSB::init: unable to create a plugin: %08x\n", status);
            goto error;
        }

        result = (*plugInInterface)->QueryInterface(plugInInterface, CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID245), (LPVOID*)&dev);
        IODestroyPlugInInterface(plugInInterface);
        if (result || !dev)
        {
            fprintf(stderr, "ASDeviceUSB::init: unable to query interface: %08x, %p\n", status, dev);
            status = kIOReturnError;
            goto error;
        }

        m_identity = aq_deviceIdent(dev);


        /* The following tries to open the device and configure it.
         * Under MacOSX 10.4, nothing grabs the comms device, and it will remain unconfigured. In this case, the
         * open and subsequent configuration attempt should succeed. Under 10.5, however, the USB class drivers
         * grab the interface and the open will fail (but subsequent IO will work because the device is configured...)
         */
        status = (*dev)->USBDeviceOpen(dev);
        if (kIOReturnSuccess == status)
        {
            IOUSBConfigurationDescriptorPtr	confDesc;
            status = (*dev)->GetConfigurationDescriptorPtr(dev, 0, &confDesc);
            if (kIOReturnSuccess == status) status = (*dev)->SetConfiguration(dev, confDesc->bConfigurationValue);
            if (status) fprintf(stderr, "ASDeviceUSB::init: USB device open but configure completed with: %08x\n", status);
        }
        else
        {
            fprintf(stderr, "ASDeviceUSB::init: Unable to open USB device: %08x\n", status);
        }


        /* Open the interface.
         */
        IOUSBFindInterfaceRequest interfaceRequest;
        interfaceRequest.bInterfaceClass = kIOUSBFindInterfaceDontCare;         // requested class
        interfaceRequest.bInterfaceSubClass = kIOUSBFindInterfaceDontCare;      // requested subclass
        interfaceRequest.bInterfaceProtocol = kIOUSBFindInterfaceDontCare;      // requested protocol
        interfaceRequest.bAlternateSetting = kIOUSBFindInterfaceDontCare;       // requested alt setting

        io_service_t usbInterface;
        IOCFPlugInInterface **iodev;

        io_iterator_t interfaceIterator;
        status = (*dev)->CreateInterfaceIterator(dev, &interfaceRequest, &interfaceIterator);
        if (kIOReturnSuccess != status)
        {
            fprintf(stderr, "unable to create interface iterator: %08x\n", status);
            goto error;
        }

        usbInterface = IOIteratorNext(interfaceIterator);
        IOObjectRelease(interfaceIterator);
        status = (usbInterface) ? kIOReturnSuccess : kIOReturnError;
        if (status)
        {
            fprintf(stderr, "ASDeviceUSB::init: error at line %d: status %08x\n", __LINE__, status);
            goto error;
        }

        status = IOCreatePlugInInterfaceForService(usbInterface, kIOUSBInterfaceUserClientTypeID, kIOCFPlugInInterfaceID, &iodev, &score);
        if (!status) status = (*iodev)->QueryInterface(iodev, CFUUIDGetUUIDBytes(kIOUSBInterfaceInterfaceID245), (LPVOID*)&intf);
        IODestroyPlugInInterface(iodev);
        if (status)
        {
            fprintf(stderr, "ASDeviceUSB::init: error at line %d: status %08x\n", __LINE__, status);
            goto error;
        }

        status = (*intf)->USBInterfaceOpen(intf);
        if (kIOReturnSuccess != status)
        {
            fprintf(stderr, "ASDeviceUSB::init: unable to open interface: %08x\n", status);
            goto error;
        }

        status = (*intf)->GetNumEndpoints(intf, &numPipes);
        if (status)
        {
            fprintf(stderr, "ASDeviceUSB::init: error at line %d: status %08x\n", __LINE__, status);
            goto error;
        }

        for (unsigned i = 1; i <= numPipes; i++)
        {
            status = (*intf)->GetPipeProperties(intf, i, &direction, &number, &transferType, &maxPacketSize, &interval);
            if (status)
            {
                //fprintf(stderr, "unable to get pipe properties for pipe %d: %08x\n", i, status);
                continue;
            }
            if (transferType != kUSBBulk)
            {
                //fprintf(stderr, "ignoring pipe %d because it is not a bulk pipe\n", i);
                continue;
            }
            if ((direction == kUSBIn) && !pipeIn)
            {
                //fprintf(stderr, "matched BULK IN pipe index %d, number %d\n",i, number);
                pipeIn = i;
            }
            if ((direction == kUSBOut) && !pipeOut)
            {
                //fprintf(stderr, "matched BULK OUT pipe index %d, number %d\n", i, number);
                pipeOut = i;
            }
        }





        /* It worked.
         */
        m_device = dev;
        m_interface = intf;
        m_pipeOut = pipeOut;
        m_pipeIn = pipeIn;

        initialise();       // Initialise the parent class, now that the transport is operational

        return true;


        /* Jump here if there is an error.
         */
    error:
        fprintf(stderr, "ASDeviceUSB::init: exit with error %08x\n", status);
        assert(kIOReturnSuccess != status);
        if (intf)
        {
            (*intf)->USBInterfaceClose(intf);
            (*intf)->Release(intf);
        }
        if (dev)
        {
            (*dev)->Release(dev);
        }
        return false;
    }


    /** Close the device.
     */
    void ASDeviceUSB::close()
    {
        if (m_interface)
        {
            (*m_interface)->USBInterfaceClose(m_interface);
            (*m_interface)->Release(m_interface);
            m_interface = 0;
        }

        if (m_device)
        {
            (*m_device)->USBDeviceClose(m_device);
            (*m_device)->Release(m_device);
            m_device = 0;
        }
    }


    /** Read data from the device.
     *
     *  @param  buffer      Buffer memory to receive the data.
     *  @param  length      Specifies the number of bytes to read.
     *  @param  actual      Returns the actual number of bytes read.
     *  @param  timeout     Specifies the timeout, in ms. If zero, a default is applied.
     *  @return             Logical true if the request succeeded, or false if it failed.
     */
    bool ASDeviceUSB::read(void* buffer, unsigned length, unsigned* actual, unsigned timeout)
    {
        assert(0 != buffer);
        assert(0 != m_pipeIn);
        assert(0 != m_pipeOut);
        assert(0 != m_interface);

        if (0 == timeout) timeout = m_timeout;

        uint8_t* ptr = (uint8_t*)buffer;
        unsigned remaining = length;
        IOReturn status = 0;
        while (remaining != 0)
        {
            UInt32 blocksize = (remaining > 8) ? 8 : remaining;
            status = (*m_interface)->ReadPipeTO(m_interface, m_pipeIn, ptr, &blocksize, timeout, timeout);
            if (m_debugRead)
            {
                fprintf(m_debugRead, " <--   %8p : %08x : %u  =  ", (void*)ptr, status, (unsigned)blocksize);
                for (unsigned i = 0; i < 8; i++)
                {
                    if (i < blocksize) fprintf(m_debugRead, " %02x", ptr[i]);
                    else fprintf(m_debugRead, "   ");
                }
                fprintf(m_debugRead, "   ");
                for (unsigned i = 0; i < blocksize; i++)
                {
                    fprintf(m_debugRead, "%c", (isprint(ptr[i]) ? ptr[i] : '.'));
                }
                fprintf(m_debugRead, "\n");
                fflush(m_debugRead);
            }
            if (status)
            {
                fprintf(stderr, "%s: error %08x from ReadPipeTO\n", __FUNCTION__, status);
                // The failure was a stall, clear it so that subsequent communication isn't broken.
                (*m_interface)->ClearPipeStallBothEnds(m_interface, m_pipeIn);
                break;
            }
            assert(blocksize <= remaining);
            remaining -= blocksize;
            ptr += blocksize;

            if (blocksize != 8) break;      // terminate loop on a short read
        }

        /* Return the total number of bytes read, if requested.
         * If not requested, signal a short read as an error.
         */
        unsigned totalBytesRead = (unsigned)(ptr - (uint8_t*)buffer);
        if (0 != actual) *actual = totalBytesRead;
        if (0 == actual && totalBytesRead != length) status = kIOReturnError;
        return (status) ? false : true;
    }





    /** Write data to the device.
     *
     *  @param  buffer      Buffer memory containing the data to write.
     *  @param  length      Specifies the number of bytes to write.
     *  @param  timeout     Specifies the timeout, in ms. If zero, a default is applied.
     *  @return             Logical true if the request succeeded, or false if it failed.
     */
    bool ASDeviceUSB::write(const void* buffer, unsigned length, unsigned timeout)
    {
        assert(0 != buffer);
        assert(0 != m_pipeIn);
        assert(0 != m_pipeOut);
        assert(0 != m_interface);

        if (0 == timeout) timeout = m_timeout;

        IOReturn status = 0;
        unsigned remaining = length;
        const uint8_t* ptr = (const uint8_t*) buffer;
        while (remaining != 0 && !status)
        {
            unsigned blocksize = (remaining > 8) ? 8 : remaining;
            status = (*m_interface)->WritePipeTO(m_interface, m_pipeOut, (void*) ptr, blocksize, timeout, timeout);
            if (m_debugWrite)
            {
                fprintf(m_debugWrite, "  -->  %8p : %08x : %u  =  ", ptr, status, (unsigned)blocksize);
                for (unsigned i = 0; i < 8; i++)
                {
                    if (i < blocksize) fprintf(m_debugWrite, " %02x", ptr[i]);
                    else fprintf(m_debugWrite, "   ");
                }
                fprintf(m_debugWrite, "   ");
                for (unsigned i = 0; i < blocksize; i++)
                {
                    fprintf(m_debugWrite, "%c", (isprint(ptr[i]) ? ptr[i] : '.'));
                }
                fprintf(m_debugWrite, "\n");
                fflush(m_debugWrite);
            }
            ptr += blocksize;
            remaining -= blocksize;
        }

        if (status)
        {
            fprintf(stderr, "%s: error %08x from WritePipeTO\n", __FUNCTION__, status);
            // Clear any stall so that subsequent communication isn't broken.
            (*m_interface)->ClearPipeStallBothEnds(m_interface, m_pipeOut);
        }

        return (status) ? false : true;
    }





    #pragma mark    ---------------- ASDeviceFactoryUSB : MacOSX specific USB implementation ----------------




    /** MacOSX specific implementation.
     */
    class ASDeviceFactoryUSB
    {
    public:

        ASDeviceFactoryUSB();
        ~ASDeviceFactoryUSB();

        bool init(
            void* context,
            ASDeviceFactoryDetect detect,
            ASDeviceFactoryConnect connect,
            ASDeviceFactoryDisconnect disconnect,
            ASDeviceFactory* factory);

        int findFreeDeviceSlot() const;
        int findDeviceSlotByService(io_service_t serviceHandle) const;

        void hidDeviceAdded(io_service_t serviceHandle);
        void comDeviceAdded(io_service_t serviceHandle);
        void comDeviceRemoved(io_service_t serviceHandle);

    private:

        ASDeviceFactory* m_factory;                         /** The factory owning this object. */
        void* m_callbackContext;                            /**< Context for client callbacks. */
        ASDeviceFactoryDetect m_callbackDetect;             /**< Client callback for detect operations. */
        ASDeviceFactoryConnect m_callbackConnect;           /**< Client callback for connect operations. */
        ASDeviceFactoryDisconnect m_callbackDisconnect;     /**< Client callback for disconnect operations. */
        IONotificationPortRef m_notifyPort;                 /**< Notification port for IO operations. */
        io_iterator_t m_hidDeviceAddedIter;                 /**< Iterator for HID device added. */
        io_iterator_t m_comDeviceAddedIter;                 /**< Iterator for COM device added. */
        io_iterator_t m_comDeviceRemovedIter;               /**< Iterator for COM device removed. */
        ASDeviceUSB* m_deviceList[kAQMaxDevices];           /**< Array of active device references. */

        friend void aq_hidDeviceAdded(void *refCon, io_iterator_t iterator);
        friend void aq_comDeviceAdded(void *refCon, io_iterator_t iterator);
        friend void aq_comDeviceRemoved(void *refCon, io_iterator_t iterator);

        ASDeviceFactoryUSB(const ASDeviceFactoryUSB&);              /**< Prevent the use of the copy operator. */
        ASDeviceFactoryUSB& operator=(const ASDeviceFactoryUSB&);   /**< Prevent the use of the assignment operator. */
};



    /** Class constructor.
     */
    ASDeviceFactoryUSB::ASDeviceFactoryUSB()
            :
            m_factory(0),
            m_callbackContext(0),
            m_callbackDetect(0),
            m_callbackConnect(0),
            m_callbackDisconnect(0),
            m_notifyPort(0),
            m_hidDeviceAddedIter(0),
            m_comDeviceAddedIter(0),
            m_comDeviceRemovedIter(0),
            m_deviceList()
    {
        for (unsigned i = 0; i < kAQMaxDevices; i++)
        {
            m_deviceList[i] = 0;
        }
    }


    /** Class destructor.
     */
    ASDeviceFactoryUSB::~ASDeviceFactoryUSB()
    {
        // Terminate notification handling.
        // IOObjectRelease(m_comDeviceRemovedIter);
        // IOObjectRelease(m_comDeviceAddedIter);
        // IOObjectRelease(m_hidDeviceAddedIter);
        IONotificationPortDestroy(m_notifyPort);

        // Clear out any active devices.
        for (int i = 0; i < kAQMaxDevices; i++)
        {
            ASDeviceUSB* device = m_deviceList[i];
            if (device)
            {
                m_callbackDisconnect(m_factory, m_callbackContext, device->identity(), device);
                m_deviceList[i] = 0;
                delete device;
            }
        }
    }


    /** Initialisation.
     *
     *  @param  context     The client context, to pass in callbacks.
     *  @param  detect      The detect callback method.
     *  @param  connect     The connect callback method.
     *  @param  factory     The factory object to pass in callbacks.
     *  @return             Logical true if the enable succeeded.
     */
    bool ASDeviceFactoryUSB::init(
        void* context,
        ASDeviceFactoryDetect detect,
        ASDeviceFactoryConnect connect,
        ASDeviceFactoryDisconnect disconnect,
        ASDeviceFactory* factory)
    {
        mach_port_t masterPort = MACH_PORT_NULL;
        CFMutableDictionaryRef hidMatchingDict = 0;
        CFMutableDictionaryRef comMatchingDict = 0;
        IONotificationPortRef notifyPort = 0;
        CFRunLoopSourceRef runLoopSource = 0;
        io_iterator_t hidDeviceAddedIter = 0;
        io_iterator_t comDeviceAddedIter = 0;
        io_iterator_t comDeviceRemovedIter = 0;
        CFNumberRef hidVendorID = 0;
        CFNumberRef hidProductID = 0;
        CFNumberRef comVendorID = 0;
        CFNumberRef comProductID = 0;
        kern_return_t status;

        const SInt32 hidUSBVendorID = kAQHidUSBVendorID;
        const SInt32 hidUSBProductID = kAQHidUSBProductID;
        const SInt32 comUSBVendorID = kAQComUSBVendorID;
        const SInt32 comUSBProductID = kAQComUSBProductID;


        m_factory = factory;
        m_callbackContext = context;
        m_callbackDetect = detect;
        m_callbackConnect = connect;
        m_callbackDisconnect = disconnect;

        /* Create a master_port for the task.
         */
        status = IOMasterPort(MACH_PORT_NULL, &masterPort);
        if (status || MACH_PORT_NULL == masterPort) goto error;

        hidMatchingDict = IOServiceMatching(kIOUSBDeviceClassName);
        comMatchingDict = IOServiceMatching(kIOUSBDeviceClassName);
        if (!hidMatchingDict || !comMatchingDict) goto error;

        hidVendorID = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &hidUSBVendorID);
        hidProductID = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &hidUSBProductID);
        comVendorID = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &comUSBVendorID);
        comProductID = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &comUSBProductID);

        CFDictionaryAddValue(hidMatchingDict, CFSTR(kUSBVendorID), hidVendorID);
        CFDictionaryAddValue(hidMatchingDict, CFSTR(kUSBProductID), hidProductID);
        CFDictionaryAddValue(comMatchingDict, CFSTR(kUSBVendorID), comVendorID);
        CFDictionaryAddValue(comMatchingDict, CFSTR(kUSBProductID), comProductID);

        CFRelease(hidVendorID);
        CFRelease(hidProductID);
        CFRelease(comVendorID);
        CFRelease(comProductID);

        notifyPort = IONotificationPortCreate(masterPort);
        runLoopSource = IONotificationPortGetRunLoopSource(notifyPort);
        CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopDefaultMode);    // notification callbacks will be run here

        comMatchingDict = (CFMutableDictionaryRef) CFRetain(comMatchingDict);

        status = IOServiceAddMatchingNotification(
            notifyPort,
            kIOFirstMatchNotification,
            hidMatchingDict, aq_hidDeviceAdded, this, &hidDeviceAddedIter);
        assert(!status);
        if (status) goto error;

        status = IOServiceAddMatchingNotification(
            notifyPort,
            kIOFirstMatchNotification,
            comMatchingDict, aq_comDeviceAdded, this, &comDeviceAddedIter);
        assert(!status);
        if (status) goto error;

        status = IOServiceAddMatchingNotification(
            notifyPort,
            kIOTerminatedNotification,
            comMatchingDict, aq_comDeviceRemoved, this, &comDeviceRemovedIter);
        assert(!status);
        if (status) goto error;



        /* All ok.
         */
        m_notifyPort = notifyPort;
        m_hidDeviceAddedIter = hidDeviceAddedIter;
        m_comDeviceAddedIter = comDeviceAddedIter;
        m_comDeviceRemovedIter = comDeviceRemovedIter;


        /* Run an initial enumeration. Note that this means that the client may see
         * notification callbacks before it has seen the successful return of this method.
         */
        aq_hidDeviceAdded(this, m_hidDeviceAddedIter);
        aq_comDeviceAdded(this, m_comDeviceAddedIter);
        aq_comDeviceRemoved(this, m_comDeviceRemovedIter);

        // Now done with the master_port
        mach_port_deallocate(mach_task_self(), masterPort);
        masterPort = MACH_PORT_NULL;
        return true;


        /* Jump here if there is a failure in setup.
         */
    error:
        // if (0 != comDeviceRemovedIter) IOObjectRelease(comDeviceRemovedIter);
        // if (0 != comDeviceAddedIter) IOObjectRelease(comDeviceAddedIter);
        // if (0 != hidDeviceAddedIter) IOObjectRelease(hidDeviceAddedIter);
        if (0 != notifyPort) IONotificationPortDestroy(notifyPort);
        if (MACH_PORT_NULL != masterPort) mach_port_deallocate(mach_task_self(), masterPort);
        if (0 != comMatchingDict) CFRelease(comMatchingDict);
        if (0 != hidMatchingDict) CFRelease(hidMatchingDict);
        return false;
    }



    /** Find a free (unused) device slot.
     *
     *  @return     The slot index, or negative if no slots are free.
     */
    int ASDeviceFactoryUSB::findFreeDeviceSlot() const
    {
        for (int index = 0; index < kAQMaxDevices; index++)
        {
            if (0 == m_deviceList[index]) return index;
        }
        return -1;
    }


    /** Find a device slot corresponding to a specific identity.
     *
     *  @param  serviceHandle   The service handle.
     *  @return                 The slot index or negative if the identity was not found.
     */
    int ASDeviceFactoryUSB::findDeviceSlotByService(io_service_t serviceHandle) const
    {
        for (int index = 0; index < kAQMaxDevices; index++)
        {
            ASDeviceUSB* device = m_deviceList[index];
            if (0 != device && device->service() == serviceHandle) return index;
        }
        return -1;
    }



    /** Callback on addition of a HID device.
     *
     *  @param  serviceHandle   The service handle.
     */
    void ASDeviceFactoryUSB::hidDeviceAdded(io_service_t serviceHandle)
    {
        IOCFPlugInInterface** plugInInterface = NULL;
        IOUSBDeviceInterface245** dev = NULL;
        kern_return_t status;
        HRESULT res;
        SInt32 score;

        status = IOCreatePlugInInterfaceForService(serviceHandle, kIOUSBDeviceUserClientTypeID, kIOCFPlugInInterfaceID, &plugInInterface, &score);
        IOObjectRelease(serviceHandle);
        if ((kIOReturnSuccess != status) || !plugInInterface)
        {
            return;
        }

        res = (*plugInInterface)->QueryInterface(plugInInterface, CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID245), (LPVOID*)&dev);
        IODestroyPlugInInterface(plugInInterface);
        if (res || !dev)
        {
            return;
        }

        status = (*dev)->USBDeviceOpen(dev);
        if (kIOReturnSuccess != status)
        {
            (void) (*dev)->Release(dev);
            return;
        }

        /* If we made it this far, we have a HID device that we can control. Notify the client
         * and if requested start the transition to comms mode.
         */
        const unsigned ident = aq_deviceIdent(dev);
        if (0 == m_callbackDetect || m_callbackDetect(m_factory, m_callbackContext, ident))
        {
            aq_hidFlipToCommsMode(dev);
        }

        (void) (*dev)->USBDeviceClose(dev);
        (void) (*dev)->Release(dev);
    }





    /** Call-back invoked when a matching communications device is detected.
     *
     *  @param  serviceHandle   The service handle.
     */
    void ASDeviceFactoryUSB::comDeviceAdded(io_service_t serviceHandle)
    {
        /* Find a slot to hold the device.
         */
        int index = findFreeDeviceSlot();
        if (index < 0)
        {
            fprintf(stderr, "%s: Too many devices connected\n", __FUNCTION__);
        }
        else
        {
            ASDeviceUSB* device = new ASDeviceUSB;
            if (device)
            {
                if (!device->init(serviceHandle))
                {
                    fprintf(stderr, "%s: ASDeviceUSB init failed\n", __FUNCTION__);
                    delete device;
                }
                else
                {
                    m_deviceList[index] = device;
                    if (m_callbackConnect) m_callbackConnect(m_factory, m_callbackContext, device->identity(), device);
                }
            }
        }
    }



    /** Call-back invoked when a matching communications device is removed.
     *
     *  @param  serviceHandle   The service handle.
     */
    void ASDeviceFactoryUSB::comDeviceRemoved(io_service_t serviceHandle)
    {
        int index = findDeviceSlotByService(serviceHandle);
        if (index < 0)
        {
            printf("%s: Unknown device removed\n", __FUNCTION__);
        }
        else
        {
            ASDeviceUSB* device = m_deviceList[index];
            if (m_callbackDisconnect) m_callbackDisconnect(m_factory, m_callbackContext, device->identity(), device);
            m_deviceList[index] = 0;
            delete device;
        }
    }





    #pragma mark    ---------------- ASDeviceFactory : System independent factory object ----------------







    /** Class constructor.
     */
    ASDeviceFactory::ASDeviceFactory()
        :
        m_usb(0)
    {
        // Nothing.
    }


    /** Class destructor.
     */
    ASDeviceFactory::~ASDeviceFactory()
    {
        disable();
    }


    /** Enable handling.
     *
     *  @param  context     The client context, to pass in callbacks.
     *  @param  detect      The detect callback method.
     *  @param  connect     The connect callback method.
     *  @return             Logical true if the enable succeeded.
     */
    bool ASDeviceFactory::enable(
        void* context,
        ASDeviceFactoryDetect detect,
        ASDeviceFactoryConnect connect,
        ASDeviceFactoryDisconnect disconnect)
    {
        if (isEnabled()) disable();

        assert(!isEnabled());
        assert(0 == m_usb);

        ASDeviceFactoryUSB* usb = new ASDeviceFactoryUSB();
        if (usb)
        {
            if (!usb->init(context, detect, connect, disconnect, this))
            {
                delete usb;
            }
            else
            {
                m_usb = usb;
            }
        }

        return isEnabled();
    }


    /** Disable handling.
     */
    void ASDeviceFactory::disable()
    {
        if (isEnabled())
        {
            assert(m_usb);
            delete m_usb;
            m_usb = 0;
        }

        assert(!isEnabled());
    }





    #pragma mark    ---------------- C-Callback Bindings ----------------



    /** Obtain a device identifier. This is based on the physical port to which it is connected (ideally
     *  it would be a MAC address or similar, but no such number appears to be assigned to a Neo).
     *
     *  @param  dev         The USB device interface handle.
     *  @return             The device identifier.
     */
    static unsigned aq_deviceIdent(IOUSBDeviceInterface245** dev)
    {
        UInt32 ident = 0;
        assert(sizeof (unsigned) >= sizeof (UInt32));
        (*dev)->GetLocationID(dev, &ident);
        return (unsigned)ident;
    }



    /** Flip the specified device to comms mode.
     *  There is black magic here - the sequences used are not documented, but determined from a bus trace.
     *
     *  @param  dev         The device handle.
     *  @return             Completion status.
     */
    static IOReturn aq_hidFlipToCommsMode(IOUSBDeviceInterface245** dev)
    {
        UInt8 numConf;
        IOReturn status;
        IOUSBConfigurationDescriptorPtr	confDesc;

        status = (*dev)->GetNumberOfConfigurations(dev, &numConf);
        if (kIOReturnSuccess != status) return status;
        if (0 == numConf) return kIOReturnError;
        status = (*dev)->GetConfigurationDescriptorPtr(dev, 0, &confDesc);
        if (kIOReturnSuccess != status) return status;
        status = (*dev)->SetConfiguration(dev, confDesc->bConfigurationValue);
        if (kIOReturnSuccess != status) return status;

        /* The following causes the Neo to switch to communication mode.
         */
        for (UInt8 i = 0xe0; i <= 0xe4; i++)
        {
            IOUSBDevRequest req;
            req.bmRequestType = USBmakebmRequestType(kUSBOut, kUSBClass, kUSBInterface);
            req.bRequest = 9;
            req.wValue = (0x02 << 8) | 0;       // report type and ID
            req.wIndex = 1;                     // interface
            req.wLength = 1;                    // one byte of data
            req.pData = &i;                     // report value
            status = (*dev)->DeviceRequest(dev, &req);
            if (status) return status;
        }

        return kIOReturnSuccess;
    }



    /** Call-back invoked when a matching HID device is detected.
     *
     *  @param  context         The reference context registered with the callback (reference to the factory object).
     *  @param  iterator        The device iterator.
     */
    void aq_hidDeviceAdded(void* refCon, io_iterator_t iterator)
    {
        ASDeviceFactoryUSB* usb = (ASDeviceFactoryUSB*)refCon;
        io_service_t serviceHandle;

        while ((serviceHandle = IOIteratorNext(iterator)))
        {
            usb->hidDeviceAdded(serviceHandle);
            IOObjectRelease(serviceHandle);
        }
    }



    /** Call-back invoked when a matching comms device is connected.
     *
     *  @param  context         The reference context registered with the callback (reference to the factory object).
     *  @param  iterator        The device iterator.
     */
    void aq_comDeviceAdded(void* refCon, io_iterator_t iterator)
    {
        ASDeviceFactoryUSB* usb = (ASDeviceFactoryUSB*)refCon;
        io_service_t serviceHandle;

        while ((serviceHandle = IOIteratorNext(iterator)))
        {
            usb->comDeviceAdded(serviceHandle);
            IOObjectRelease(serviceHandle);
        }
    }



    /** Call-back invoked when a matching comms device is disconnected.
     *
     *  @param  context         The reference context registered with the callback (reference to the factory object).
     *  @param  iterator        The device iterator.
     */
    void aq_comDeviceRemoved(void *refCon, io_iterator_t iterator)
    {
        ASDeviceFactoryUSB* usb = (ASDeviceFactoryUSB*)refCon;
        io_service_t serviceHandle;

        while ((serviceHandle = IOIteratorNext(iterator)))
        {
            usb->comDeviceRemoved(serviceHandle);
            IOObjectRelease(serviceHandle);
        }
    }

}   // namespace


