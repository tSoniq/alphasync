/** @file   ASPreferenceController.mm
 *  @brief
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

#import <Carbon/Carbon.h>           // Required for AHLookupAnchor
#import "ASPreferenceController.h"
#import "ASApplicationContoller.h"

@implementation ASPreferenceController


+ (NSStringEncoding)getDefaultTextEncoding
{
    NSStringEncoding encoding;
    switch ([[NSUserDefaults standardUserDefaults] integerForKey:kASPreferenceKeyTextEncoding])
    {
        case kASPreferenceTextEncodingMacASCII:
            encoding = NSMacOSRomanStringEncoding;
            break;

        case kASPreferenceTextEncodingPlainASCII:
            encoding = NSASCIIStringEncoding;
            break;

        case kASPreferenceTextEncodingUTF8:
            encoding = NSUTF8StringEncoding;
            break;

        case kASPreferenceTextEncodingUnicode:
            encoding = NSUnicodeStringEncoding;
            break;

        default:
            [[NSUserDefaults standardUserDefaults] setInteger:kASPreferenceTextEncodingUTF8 forKey:kASPreferenceKeyTextEncoding];
            encoding = NSUTF8StringEncoding;
            break;
    }

    return encoding;
}



/** Raise an error if the default constructor is used.
 */
- (id)init
{
    [self doesNotRecognizeSelector:_cmd];
    return nil;
}


- (id)initWithAppController:(ASApplicationController*)controller
{
    if ((self = [super initWithWindowNibName:@"Preferences"]))
    {
        applicationController = [controller retain];
    }
    return self;
}


- (void)dealloc
{
    [applicationController release];
    [super dealloc];
}


- (int)displayOption
{
    return [[NSUserDefaults standardUserDefaults] integerForKey:kASPreferenceKeyFilter];
}


- (NSString*)backupFolder
{
    return [[NSUserDefaults standardUserDefaults] stringForKey:kASPreferenceKeyBackupFolder];
}

- (int)textEncodingOption
{
    return [[NSUserDefaults standardUserDefaults] integerForKey:kASPreferenceKeyTextEncoding];
}

- (BOOL)autoRun
{
    return [[NSUserDefaults standardUserDefaults] boolForKey:kASPreferenceKeyAutoRun];
}



- (void)windowDidLoad
{
    [outletFilterOption selectItemWithTag:[self displayOption]];
    [outletBackupFolder setTitle:[self backupFolder]];
    [outletTextEncoding selectItemWithTag:[self textEncodingOption]];
    [outletAutoRun setState:[self autoRun] ? NSOnState : NSOffState];
}




- (IBAction)actionFilterOption:(id)sender
{
    NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
    int oldOption = [defaults integerForKey:kASPreferenceKeyFilter];

    int newOption;

    switch ([sender selectedTag])
    {
        case 0:     newOption = kASPreferenceOptionFilterAlphawordOnly;             break;
        case 1:     newOption = kASPreferenceOptionFilterAlphawordAndDictionary;    break;
        case 2:     newOption = kASPreferenceOptionFilterAll;                       break;
        default:
            assert(0);  // invalid UI tag
            newOption = kASPreferenceOptionFilterAlphawordOnly;
            break;
    }

    if (newOption != oldOption)
    {
        [defaults setInteger:newOption forKey:kASPreferenceKeyFilter];
        [defaults synchronize];
        [applicationController reloadDeviceData:self];
    }
}



- (void)backupFolderPanelDidEnd:(NSOpenPanel*)panel returnCode:(int)returnCode  contextInfo:(void*)contextInfo
{
    (void)contextInfo;
    if (NSOKButton == returnCode)
    {
        NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
        NSArray* filenames = [panel filenames];
        if ([filenames count] == 1)
        {
            [defaults setObject:[filenames objectAtIndex:0] forKey:kASPreferenceKeyBackupFolder];
            [outletBackupFolder setTitle:[self backupFolder]];
        }
    }
}

- (IBAction)actionBackupFolder:(id)sender
{
    (void)sender;
    NSOpenPanel* panel = [NSOpenPanel openPanel];
    [panel setCanChooseFiles:NO];
    [panel setCanChooseDirectories:YES];
    if ([panel respondsToSelector:@selector(setCanCreateDirectories:)]) [panel setCanCreateDirectories:YES];
    [panel setResolvesAliases:YES];
    [panel setAllowsMultipleSelection:NO];
    [panel setTitle:@"Select new backup folder"];
    [panel setPrompt:@"Select"];

    [panel beginSheetForDirectory:[self backupFolder]
           file:nil
           types:nil
           modalForWindow:[self window]
           modalDelegate:self
           didEndSelector:@selector(backupFolderPanelDidEnd:returnCode:contextInfo:)
           contextInfo:nil];
}

- (IBAction)actionAutoRun:(id)sender
{
    int state = [sender state];
    [[NSUserDefaults standardUserDefaults] setBool:((NSOnState == state) ? YES : NO) forKey:kASPreferenceKeyAutoRun];
    [outletAutoRun setState:[self autoRun] ? NSOnState : NSOffState];
    [applicationController setupAutoRun:0.0];
}

- (IBAction)actionTextEncoding:(id)sender
{
    (void)sender;
    int tag = [[outletTextEncoding selectedItem] tag];
    int encoding;
    switch (tag)
    {
        case kASPreferenceTextEncodingMacASCII:
        case kASPreferenceTextEncodingPlainASCII:
        case kASPreferenceTextEncodingUTF8:
        case kASPreferenceTextEncodingUnicode:
            encoding = tag;
            break;

        default:
            encoding = kASPreferenceTextEncodingMacASCII;
            break;
    }

    [[NSUserDefaults standardUserDefaults] setInteger:encoding forKey:kASPreferenceKeyTextEncoding];
}

- (IBAction)actionPreferenceHelp:(id)sender
{
    (void)sender;

    CFBundleRef applicationBundle = CFBundleGetMainBundle();
    if (applicationBundle)
    {
        CFTypeRef bookName = CFBundleGetValueForInfoDictionaryKey(applicationBundle, CFSTR("CFBundleHelpBookName"));
        if (bookName && CFGetTypeID(bookName) == CFStringGetTypeID())
        {
            // Display the help.
            AHLookupAnchor((CFStringRef)bookName, kASPreferenceHelpAnchor);
        }
    }
}


@end
