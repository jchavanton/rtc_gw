FROM debian

ENV DEB_PKGS="git build-essential python curl wget pkg-config"
ENV WEBRTC_COMMIT=36e3147b219de57325e9b009fcf672a872083566

RUN apt-get update && apt-get install -y ${DEB_PKGS}

RUN echo "build depot_tools" \
	&& mkdir -p /git && cd /git \
	&& git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git

RUN echo "fetching webrtc and syncing workspace using gclient" \
	&& export PATH=$PATH:/git/depot_tools  \
	&& mkdir -p /git/webrtc-checkout \
	&& cd /git/webrtc-checkout \
	&& fetch --nohooks webrtc \
	&& gclient sync

# consider setting some metrics settings after checkout
# WARNING: Your metrics.cfg file was invalid or nonexistent. A new one will be created.
RUN echo "checkout and generate Ninja files" \
	&& export PATH=$PATH:/git/depot_tools  \
	&& cd /git/webrtc-checkout/src \
	&& git checkout ${WEBRTC_COMMIT} \
	&& sed -i 's/rtc_use_x11 = use_x11/rtc_use_x11 = false/g' webrtc.gni \
	&& gn gen out/Default

RUN echo "build RTC GW" \
	&& export PATH=$PATH:/git/depot_tools  \
	&& cd /git/webrtc-checkout/src/examples \
	&& git clone https://github.com/jchavanton/rtc_gw.git \
	&& cd rtc_gw && git checkout update \
	&& cd /git/webrtc-checkout/src \
        && patch -p1 < examples/rtc_gw/build.patch \
	&& ninja -C out/Default
