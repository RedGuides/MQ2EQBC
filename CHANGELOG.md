Aug 4th, 2021 - Version 1.5
- Highlander edition - combined several different versions of EQBCS into one

Dec 15th, 2008 - Version 1.2.i2
- Added support for EQBC Interface by ieatacid 

Jan 16th, 2007 - Version 1.2.v1
- Added a 50 second ping.  The client does not need to be changed for this
  as it already accepts a ping message.  This should prevent the connection
  from crapping out.  It also should speed up detection of dead users.

Aug 7th, 2006 - Version 1.1.a7
- Fixed the local echo check

Jul 27th, 2006 - Version 1.1.a6
- Changed version numbering system. It's up to you to decide if the 'a' is
  "ascii" or "alpha" :)
- Channel list saved in INI file and restored when you log in.
- New command: /bccmd togglelocalecho.  When Local Echo is on, commands sent
  to a channel you are in will be sent back to you (as per toomanynames)

Jul 25th, 2006 - Version 1.0.5ascii-5
- Fixed bug that would crash server when any character other than the current
  oldest server connection left a channel.

Jul 15th, 2006 - Version 1.0.5ascii-4
- Added support for pseudo-channels.  A /bct to channel goes to everyone in
  the channel.
- New command: /bccmd channels channel_list
- Added ability to escape characters \<char> will be translated to just <char>

Jul 1st, 2006 - Version 1.0.5ascii-3
- Apply patch from Sorcerer for updated Netbots.
- Modified definition of two new Netbots functions to use char *
  rather than PCHAR in order to allow compilation on non-windows.

May 20th, 2006 - Version 1.0.5ascii-2
- Flush input buffer after processing tell to invalid username

Apr 16th, 2006 - Version 1.0.5ascii-1
- Added support for tells. (/bct)
- Added command line options -p, -i, -l, -u and -d.
-p <port> sets port number to listen on
-i <addr> IP Address of interface to listen on. Unspecified = ALL
-d (Unix only) Run as daemon
-l <file> log to file rather than STDOUT
-u <user> (UNIX only) setuid to named user

Nov 17th, 2005 - Version 1.0.5
- Fixed name problem bug reported by DKAA.

Oct 13th, 2005 - Version 1.0.4
- Fixed unix version not handling unclean disconnects properly.

Oct 5th, 2005 - Version 1.0.3
- Made more clear where to add login password (LOGIN_START_TOKEN).
- Changed select timeout to non debugging value, doh.
- Added support for msgall. (/bca)
- Added support for mq2netbots.
- Added ability for ping support - not used yet, but would give faster notification when someone does dirty drop from server (ie. crash, plugin unload)
- More cpu friendly.

Sep 26th, 2005 - Version 1.0.2
- Made compatible with VC6

Sep 25th, 2005 - Version 1.0.1
- Fixed hang on reading when closing. Now uses select to see if data exists.
