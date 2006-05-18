all: initng-usplash initng-usplash-shutdown

initng-usplash: initng-usplash.c
	gcc initng-usplash.c -Os -Werror -Wall -lngeclient -I/usr/include/initng -o initng-usplash
initng-usplash-shutdown: initng-usplash-shutdown.c
	gcc initng-usplash-shutdown.c -Os -Werror -Wall -lngeclient -I/usr/include/initng -o initng-usplash-shutdown
initng-usplash-static: initng-usplash.c
	gcc initng-usplash.c -Os -static -Werror -Wall -lsngeclient -I/usr/include/initng -o initng-usplash-static

install: initng-usplash initng-usplash-shutdown
	cp initng-usplash initng-usplash-shutdown /sbin
	cp usplash.i /etc/initng/system