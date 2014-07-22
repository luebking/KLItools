#!/bin/sh
if ( [ ! -e kwindowsystem ] || [ kwindowsystem.cpp -nt kwindowsystem ] ); then
    for lib in $(ldd `which kde4-config` | sed '/\(libkdecore\.so\|libQtCore\.so\)/!d; s/^.* => \([^ ]*\) .*/\1/g'); do
        LIB_PATH="${LIB_PATH} -L`dirname $lib`"
    done
    echo $LIB_PATH
    g++ `pkg-config --libs --cflags QtGui` -lX11 \
        -I`kde4-config --path include | sed 's%:%KDE -I%g; s%$%KDE%g'` $LIB_PATH -lkdeui -o kwindowsystem kwindowsystem.cpp
fi
if [ "$1" = "install" ]; then
    install -v kwindowsystem "`kde4-config --prefix`/bin/"
fi
