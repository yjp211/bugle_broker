#!/bin/bash

clean(){
    make distclean
    rm -rf autom4te.cache/
    unlink compile
    unlink depcomp
    unlink missing
    unlink install-sh
    rm -f ./broker/bin/broker
    rm -f aclocal.m4 autoscan.log stamp-h1
    rm -f configure configure.scan config.h.in config.h.in~ config.h config.log config.status
    find ./ -type d -name .deps |xargs rm -rf
    find ./ -type f -name Makefile.in |xargs rm -rf
    exit 0
}

init(){
    if [ "$(uname)" == "Darwin" ]; then
        #it's mac use dynamic library
        sed -i -e s/"^broker_LDADD.*$"/"broker_LDADD = -lpthread -ljson-c -levent  -lcurl -lnettle -lwslay "/ src/Makefile.am    
    else
        #it's linux use static library
        sed -i -e s/"^broker_LDADD.*$"/"broker_LDADD = -lpthread -lrt ..\/static-lib\/libjson-c.a ..\/static-lib\/libevent.a  ..\/static-lib\/libcurl.a ..\/static-lib\/libnettle.a ..\/static-lib\/libwslay.a "/ src/Makefile.am
    fi
}

build(){
    aclocal && autoheader && automake --add-missing && autoconf
    if [ $? = 0 ]; then
        ./configure && make
        if [ $? = 0 ]; then
            mv -f ./src/broker ./broker/bin/broker
        fi
    else
        echo "automake: some thing it's wrong\n"
    fi
}

if [ "$1" == "clean" ]; then
    echo  "\n=======make distclean and clean automake files=====\n\n"
    clean
else
    echo  "\n=====automake======\n\n"
    init
    build
fi
exit 0



