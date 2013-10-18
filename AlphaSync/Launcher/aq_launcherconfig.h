/** @file       aq_launcherconfig.h
 *  @brief      Configuration of the launcher.
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
 *
 *  The launcher utility can be used generically with any application. Customise these definitions
 *  to use a particular name or application path in order to avoid conflicts with other launcher
 *  instances that might be in use.
 */
#ifndef COM_TSONIQ_aq_launcherconfig_h
#define COM_TSONIQ_aq_launcherconfig_h    (1)


#define kAQLauncherBundleID         CFSTR("com.tsoniq.AlphaSyncLauncher")               // this must match the value in the launcher's info.plist file
#define kAQLauncherPreferenceID     CFSTR("com.tsoniq.AlphaSyncLauncher")               // names the preference file
#define kAQLauncherControlPort      CFSTR("com.tsoniq.AlphaSyncLauncher.control")       // name the mach port used to communicate with the launcher

#define kAQLauncherEnableLog        (1)     // REVIEW: ensure that this is implemented
#define kAQLauncherEnableLogError   (1)     // REVIEW: ensure that this is implemented


#endif  //COM_TSONIQ_aq_launcherconfig_h
