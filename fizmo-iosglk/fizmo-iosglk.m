
/* fizmo-iosglk.m
 *
 * This file is part of fizmo.
 *
 * Copyright (c) 2012 Andrew Plotkin.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "FizmoGlkDelegate.h"
#import "GlkLibrary.h"
#import "GlkStream.h"

#include "glk.h"
#include "glk_interface.h"
#include "glk_screen_if.h"
#include "glk_blorb_if.h"
#include "glk_filesys_if.h"
#include "glkstart.h" /* This comes with the Glk library. */
#include "ios-autosave.h"

#include <interpreter/fizmo.h>
#include <interpreter/config.h>
#include <tools/tracelog.h>
#include <tools/unused.h>

static char *init_err = NULL;

static strid_t gamefilestream = nil;
static NSString *gamepathname = nil;

/* Called in the VM thread before glk_main. This sets up variables which will be used to launch the Fizmo core.
 */
void iosglk_startup_code()
{
	GlkLibrary *library = [GlkLibrary singleton];
	FizmoGlkDelegate *libdelegate = library.glkdelegate;
	if (libdelegate) {
		NSString *path = [libdelegate gamePath];
		gamepathname = [path retain]; // retain forever
	}
	
	if (!gamepathname || ![library.filemanager isReadableFileAtPath:gamepathname])
		init_err = "Unable to locate game file";
}

/* This callback has the job of opening a Glk stream to the game file, and embedding it into a z_file object. If you pass in a z_file object, it uses that rather than allocating a new one.
 */
static z_file *iosglk_open_game_stream(z_file *current_stream)
{
	if (gamefilestream) {
		// This is the old stream object; it's just been closed. Discard it.
		[gamefilestream release];
		gamefilestream = nil;
	}
	
	// Open a new stream object.
	gamefilestream = [[GlkStreamFile alloc] initWithMode:filemode_Read rock:1 unicode:NO textmode:NO dirname:@"." pathname:gamepathname]; // retain forever, or until the next call of this function.
	if (!gamefilestream)
		return nil;
	
	// Create a z_file for the stream, or stick it into the existing z_file.
	if (!current_stream)
		current_stream = zfile_from_glk_strid(gamefilestream, "Game", FILETYPE_DATA, FILEACCESS_READ);
	else
		zfile_replace_glk_strid(current_stream, gamefilestream);
	
	return current_stream;
}

/* Called in the VM thread. This is the VM main function.
 */
void glk_main(void)
{
	z_file *story_stream;
	z_file *autosave_stream;
	
	if (init_err) {
		glkint_fatal_error_handler(init_err, NULL, NULL, FALSE, 0);
		return;
	}
	
	/* Set up all the configuration for Fizmo that we care about. */
	
	set_configuration_value("savegame-path", NULL);
	//set_configuration_value("transcript-filename", "transcript");
	set_configuration_value("savegame-default-filename", "");
	set_configuration_value("disable-stream-2-wrap", "true");
	set_configuration_value("disable-stream-2-hyphenation", "true");
	set_configuration_value("stream-2-left-margin", "0");
	
	NSString *approot = [NSBundle mainBundle].bundlePath;
	NSString *locales = [approot stringByAppendingPathComponent:@"FizmoLocales"];
	set_configuration_value("i18n-search-path", (char *)locales.UTF8String);

	/* Add the iOS-specific autosave functions to the screen interface. */
	glkint_screen_interface.do_autosave = iosglk_do_autosave;
	glkint_screen_interface.restore_autosave = iosglk_restore_autosave;
	
	/* Install the interface objects. */
	fizmo_register_filesys_interface(&glkint_filesys_interface);
	fizmo_register_screen_interface(&glkint_screen_interface);
	fizmo_register_blorb_interface(&glkint_blorb_interface);
	
	/* Begin. */
	story_stream = glkint_open_interface(&iosglk_open_game_stream);
	if (!story_stream) {
		return;
	}
	autosave_stream = iosglk_find_autosave();
	fizmo_start(story_stream, NULL, autosave_stream, -1, -1);
}

