#!/bin/bash
PATH="/usr/local/bin:/usr/bin:/bin:/usr/local/sbin:/usr/sbin:/sbin"


function mount_private()
{
    echo "try to mount $1 and make private"
    if [ `cat /proc/mounts | grep $1 | wc -l` -eq 0 ]; then
            echo "$1 is not mounted, mount first"
            echo "mount --bind $1 $1"
            mount --bind $1 $1
    fi
    echo "mount --make-private $1"
    mount --make-private $1
    echo "mount $1 ok"
}

function umount_private()
{
    umount $1
}


service_stop()
{
	killall -10 nest_agent
	sleep 1
	killall -9 nest_agent

	killall -10 nest_clone
	sleep 1
	killall -9 nest_clone
}

service_start()
{
	mount_private /data/release
	cd ../bin/;
	./nest_clone ../conf/nest_agent.xml
	./nest_agent ../conf/nest_agent.xml
}


case $1 in
    stop)
	echo "Service STOPING.........."
        service_stop
        ;;
    start)
	echo "Service STARTING........."
	service_start
        ;;
    restart)
	echo "Service RESTARTING......."
	service_stop
	sleep 1
	service_start
        ;;
    *)
        echo " Usage: $0 { stop | start | restart }"
esac

exit 0
