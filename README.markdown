### dbslayer ###

This is an unofficial github fork of [dbslayer's](http://code.nytimes.com/projects/dbslayer) [svn repo](http://www.dbslayer.org/svn/dbslayer/trunk/).

I added `dbslayer.command` for easy deployment on Mac OS X.

#### installation (Mac OS X) ####

	git clone git://github.com/rentzsch/dbslayer.git
	cd dbslayer
	./configure
	make
	open start_dbslayer.command

The dbslayer server process should now be running and listening for local connections on port 9090. Try [giles](http://github.com/rentzsch/giles) for a simple ajax frontend to dbslayer to verify it's working.

#### Cleaning ####

	make distclean; rm -rf aclocal.m4 autom4te.cache
