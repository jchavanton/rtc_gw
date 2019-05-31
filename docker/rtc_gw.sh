#!/bin/sh
DIR_PREFIX=`pwd`
CONTAINER=rtc_gw
IMAGE=rtc_gw:latest
docker stop ${CONTAINER}
docker rm ${CONTAINER}
docker run -d --net=host --name=${CONTAINER} ${IMAGE} /bin/sh -c "tail -f /dev/null"


echo "docker exec -it rtc_gw bash"

