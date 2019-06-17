
# Docker :

## build image
```
docker/build.sh  
```

## run image
```
docker exec -it rtc_gw bash
```
```
cd /git/webrtc-checkout/src
./out/Default/rtc_gw --listen 127.0.0.1
```


# Installation :

## Debian
```
apt-get install git build-essential
```

## WebRTC
#### depot_tools
```
git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
export PATH=$PATH:/path/to/depot_tools
```
#### WebRTC sources
```
mkdir webrtc-checkout
cd webrtc-checkout
fetch --nohooks webrtc
gclient sync
cd src
git checkout master
gn gen out/Default
```


## RTC Gateway

### checkout the source code
```
cd webrtc-checkout/src/examples
git clone https://github.com/jchavanton/rtc_gw.git
```

### Patch BUILD.gn to build rtc_gw

```
patch -p1 < examples/rtc_gw/build.patch
```

### Build everything
```
ninja -C out/Default
```
