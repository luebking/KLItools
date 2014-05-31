KLItools
========

Kommand Line Interface tools ;-)

activities
----------
shellscript to manage KDE activities (list, add, remove - try "activities help")

blurwindow
----------
simple shellscript to set a window completely blurring. requires "xprop"

kconfig.cpp
-----------
replacement for kread/writeconfig - run "kconfig help"
./make_kconfig.sh [install]

kwin
----
kwin launcher shellscript. allows to set various environment variables for kwin and implies "--replace &"
place it somewhere up in $PATH, eg. /usr/local/bin or ~/bin

mwminfo
-------
shellscript that interprets the_MOTIF_WM_HINTS property of a window. requires "xprop"


playOrStop
----------
simple shellsctipt that plays a file with mplayer. on second invocation it stops playback (and
starts playback of new file if invoked on a different file) - usable as audio"preview" for eg.
dolphin or other FMs