Wizd is a server application for use with the I-O Data LinkPlayer 2
and similar devices such as the Buffalo LinkStation and the MediaWiz.
It works as a replacement for the Link Server software supplied by 
I-O Data, but can be installed and run along side that server software
(and the Advanced Server software) if you desire.

To install wizd on your computer:

1) Edit the "wizd.conf" file with a text editor (such as Notepad)
   In particular you will want to change the "alias" settings to
   point to your media files.  Save the changes when you are done.

2) Double-click the wizd.exe program.  It should pop up a console
   window with your configuration information.

3) The "Wizd" server should now appear on your LinkPlayer.  Select
   it to browse and play your media files.

Tips:

You can test out the server using Internet Explorer by accessing
the page http://localhost:8004

Beginning a line with "#" in the wizd.conf file will disable that line
and the setting will revert to its default.  Removing the "#" will
enable a previously disabled line.

Any time you make changes to the wizd.conf file you will need to stop
and restart the wizd.exe program for the changes to take effect.

If the window disappears immediately, or things don't appear to be
working correctly, then edit the "wizd.conf" file, and set the
flag_debug_log_output setting to "true".
Then run wizd.exe again, and check the wizd_debug.log file for errors.

If you want to change your configuration, press Ctrl+C in the console
window to close the program, then modify wizd.conf and then restart
wizd.exe

For DVD navigation by chapters, the .vob files must be in a VIDEO_TS
subfolder.

When resuming from a bookmark, press the "left" arrow on your remote
to "go to 0%" and it will start playback from the beginning of the
video.

Once things are working well, edit the wizd.conf file, and put a comment
mark "#" at the beginning of the "flag_daemon false" line, or set this
parameter to "true".  Then restart the wizd.exe program, and it will 
run as a hidden application with no console window.  Once you make this
change you will have to use Task Manager to stop the program.

Features of this version:

1) Saves bookmarks for MPEG video files, so you resume playback where
   it was stopped
2) DVD information is displayed by title set, instead of by VOB file
3) DVD playback allows skipping forward by chapter
4) Delete mode allows deleting files from the server (disabled by default)
5) Internet Explorer .url files recognized, and displayed as links
6) Playlists are generated recursively using current folder and all subfolders

