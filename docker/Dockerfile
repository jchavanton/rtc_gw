FROM debian

ENV DEB_PKGS="git build-essential python"

RUN apt-get update && apt-get install -y ${DEB_PKGS}

RUN echo "build depot_tools" \
	&& mkdir -p /git && cd /git \
	&& git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git \
	&& export PATH=$PATH:/git/depot_tools \
	&& mkdir -p /git/webrtc-checkout \
	&& cd /git/webrtc-checkout \
	&& fetch --nohooks webrtc \
	&& gclient sync \
	&& cd src \
	&& git checkout master \
	&& gn gen out/Default

RUN echo "build RTC GW" \
#	&& cd /git/webrtc-checkout/src/examples \
#	&& git clone https://github.com/jchavanton/rtc_gw.git \
#	&& patch -p1 < examples/rtc_gw/build.patch \
#	&& ninja -C out/Default
