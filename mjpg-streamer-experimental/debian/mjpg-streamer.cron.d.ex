#
# Regular cron jobs for the mjpg-streamer package
#
0 4	* * *	root	[ -x /usr/bin/mjpg-streamer_maintenance ] && /usr/bin/mjpg-streamer_maintenance
