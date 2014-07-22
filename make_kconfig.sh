#!/bin/sh
if ( [ ! -e kconfig ] || [ kconfig.cpp -nt kconfig ] ); then
    for lib in $(ldd `which kde4-config` | sed '/\(libkdecore\.so\|libQtCore\.so\)/!d; s/^.* => \([^ ]*\) .*/\1/g'); do
        LIB_PATH="${LIB_PATH} -L`dirname $lib`"
    done
    g++ `pkg-config --libs --cflags QtCore` \
        -I`kde4-config --path include | sed 's%:%KDE -I%g; s%$%KDE%g'` $LIB_PATH  -lkdecore -o kconfig kconfig.cpp
fi
if [ "$1" = "install" ]; then
    install -v kconfig "`kde4-config --prefix`/bin/"
fi
