#!/bin/sh
if ( [ ! -e kwindowsystem ] || [ kwindowsystem.cpp -nt kwindowsystem ] ); then
    g++ `pkg-config --libs --cflags QtGui` -lX11 \
        -I`kde4-config --path include | sed 's%:%KDE -I%g; s%$%KDE%g'` \
        -L`kde4-config --path lib | sed 's/:/ -L/g'` -lkdeui -o kwindowsystem kwindowsystem.cpp
fi
if [ "$1" = "install" ]; then
    install -v kwindowsystem "`kde4-config --prefix`/bin/"
fi
