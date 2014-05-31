#!/bin/sh
if ( [ ! -e kconfig ] || [ kconfig.cpp -nt kconfig ] ); then
    g++ `pkg-config --libs --cflags QtCore` \
        -I`kde4-config --path include | sed 's%:%KDE -I%g; s%$%KDE%g'` \
        -L`kde4-config --path lib | sed 's/:/ -L/g'` -lkdecore -o kconfig kconfig.cpp
fi
if [ "$1" = "install" ]; then
    install -v kconfig "`kde4-config --prefix`/bin/"
fi
