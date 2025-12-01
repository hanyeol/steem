FROM ubuntu:22.04

ARG STEEM_STATIC_BUILD=ON
ENV STEEM_STATIC_BUILD=${STEEM_STATIC_BUILD}

ENV LANG=en_US.UTF-8
ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies in a single layer
RUN apt-get update && \
    apt-get install -y \
        autoconf \
        automake \
        autotools-dev \
        bsdmainutils \
        build-essential \
        ca-certificates \
        cmake \
        curl \
        doxygen \
        fcgiwrap \
        gdb \
        git \
        jq \
        libboost-all-dev \
        libbz2-dev \
        libgflags-dev \
        liblz4-dev \
        libreadline-dev \
        libsnappy-dev \
        libssl-dev \
        libtool \
        libzstd-dev \
        ncurses-dev \
        nginx \
        pkg-config \
        python3-jinja2 \
        python3-pip \
        runit \
        virtualenv \
        wget \
        zlib1g-dev \
    && \
    pip3 install --no-cache-dir gcovr && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

ADD . /usr/local/src/steem

RUN \
    if [ "$BUILD_STEP" = "1" ] || [ ! "$BUILD_STEP" ] ; then \
    cd /usr/local/src/steem && \
    mkdir build && \
    cd build && \
    cmake \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_STEEM_TESTNET=ON \
        -DLOW_MEMORY_NODE=OFF \
        -DCLEAR_VOTES=ON \
        -DSKIP_BY_TX_ID=ON \
        .. && \
    make -j$(nproc) chain_test test_fixed_string plugin_test && \
    ./tests/chain_test && \
    ./tests/plugin_test && \
    ./programs/utils/test_fixed_string && \
    cd /usr/local/src/steem && \
    doxygen && \
    PYTHONPATH=programs/build_helpers \
    python3 -m steem_build_helpers.check_reflect && \
    programs/build_helpers/get_config_check.sh && \
    rm -rf /usr/local/src/steem/build ; \
    fi

RUN \
    if [ "$BUILD_STEP" = "2" ] || [ ! "$BUILD_STEP" ] ; then \
    cd /usr/local/src/steem && \
    mkdir build && \
    cd build && \
    cmake \
        -DCMAKE_INSTALL_PREFIX=/usr/local/steemd-testnet \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_STEEM_TESTNET=ON \
        -DLOW_MEMORY_NODE=OFF \
        -DCLEAR_VOTES=ON \
        -DSKIP_BY_TX_ID=ON \
        -DSTEEM_STATIC_BUILD=${STEEM_STATIC_BUILD} \
        .. && \
    make -j$(nproc) chain_test test_fixed_string plugin_test && \
    make install && \
    ./tests/chain_test && \
    ./tests/plugin_test && \
    ./programs/utils/test_fixed_string && \
    cd /usr/local/src/steem && \
    doxygen && \
    PYTHONPATH=programs/build_helpers \
    python3 -m steem_build_helpers.check_reflect && \
    programs/build_helpers/get_config_check.sh && \
    rm -rf /usr/local/src/steem/build ; \
    fi

RUN \
    if [ "$BUILD_STEP" = "1" ] || [ ! "$BUILD_STEP" ] ; then \
    cd /usr/local/src/steem && \
    mkdir build && \
    cd build && \
    cmake \
        -DCMAKE_BUILD_TYPE=Debug \
        -DENABLE_COVERAGE_TESTING=ON \
        -DBUILD_STEEM_TESTNET=ON \
        -DLOW_MEMORY_NODE=OFF \
        -DCLEAR_VOTES=ON \
        -DSKIP_BY_TX_ID=ON \
        -DCHAINBASE_CHECK_LOCKING=OFF \
        .. && \
    make -j$(nproc) chain_test plugin_test && \
    ./tests/chain_test && \
    ./tests/plugin_test && \
    mkdir -p /var/cobertura && \
    gcovr --object-directory="../" --root=../ --xml-pretty --gcov-exclude=".*tests.*" --gcov-exclude=".*fc.*" --gcov-exclude=".*app*" --gcov-exclude=".*net*" --gcov-exclude=".*plugins*" --gcov-exclude=".*schema*" --gcov-exclude=".*time*" --gcov-exclude=".*utilities*" --gcov-exclude=".*wallet*" --gcov-exclude=".*programs*" --output="/var/cobertura/coverage.xml" && \
    cd /usr/local/src/steem && \
    rm -rf /usr/local/src/steem/build ; \
    fi

RUN \
    if [ "$BUILD_STEP" = "2" ] || [ ! "$BUILD_STEP" ] ; then \
    cd /usr/local/src/steem && \
    mkdir build && \
    cd build && \
    cmake \
        -DCMAKE_INSTALL_PREFIX=/usr/local/steemd-low \
        -DCMAKE_BUILD_TYPE=Release \
        -DLOW_MEMORY_NODE=ON \
        -DCLEAR_VOTES=ON \
        -DSKIP_BY_TX_ID=OFF \
        -DBUILD_STEEM_TESTNET=OFF \
        -DSTEEM_STATIC_BUILD=${STEEM_STATIC_BUILD} \
        .. \
    && \
    make -j$(nproc) && \
    make install && \
    cd .. && \
    ( /usr/local/steemd-low/bin/steemd --version \
      | grep -o '[0-9]*\.[0-9]*\.[0-9]*' \
      && echo '_' \
      && git rev-parse --short HEAD ) \
      | sed -e ':a' -e 'N' -e '$!ba' -e 's/\n//g' \
      > /etc/steemd-version && \
    cat /etc/steemd-version && \
    rm -rfv build && \
    mkdir build && \
    cd build && \
    cmake \
        -DCMAKE_INSTALL_PREFIX=/usr/local/steemd-high \
        -DCMAKE_BUILD_TYPE=Release \
        -DLOW_MEMORY_NODE=OFF \
        -DCLEAR_VOTES=OFF \
        -DSKIP_BY_TX_ID=ON \
        -DBUILD_STEEM_TESTNET=OFF \
        -DSTEEM_STATIC_BUILD=${STEEM_STATIC_BUILD} \
        .. \
    && \
    make -j$(nproc) && \
    make install && \
    rm -rf /usr/local/src/steem ; \
    fi

RUN \
    apt-get remove -y \
        automake \
        autotools-dev \
        bsdmainutils \
        build-essential \
        cmake \
        doxygen \
        dpkg-dev \
        libboost-all-dev \
        libc6-dev \
        libexpat1-dev \
        libhwloc-dev \
        libibverbs-dev \
        libicu-dev \
        libltdl-dev \
        libncurses-dev \
        libnuma-dev \
        libopenmpi-dev \
        libreadline-dev \
        libssl-dev \
        libtinfo-dev \
        libtool \
        linux-libc-dev \
        m4 \
        make \
        manpages \
        manpages-dev \
        mpi-default-dev \
        python3-dev \
    && \
    apt-get autoremove -y && \
    rm -rf \
        /var/lib/apt/lists/* \
        /tmp/* \
        /var/tmp/* \
        /var/cache/* \
        /usr/include \
        /usr/local/include

RUN useradd -s /bin/bash -m -d /var/lib/steemd steemd

RUN mkdir /var/cache/steemd && \
    chown steemd:steemd -R /var/cache/steemd

ENV HOME=/var/lib/steemd
RUN chown steemd:steemd -R /var/lib/steemd

VOLUME ["/var/lib/steemd"]

# rpc service:
EXPOSE 8090
# p2p service:
EXPOSE 2001

# add seednodes from documentation to image
# ADD docs/seednodes.txt /etc/steemd/seednodes.txt

# the following adds lots of logging info to stdout
ADD configs/witness.config.ini /etc/steemd/witness.config.ini
ADD configs/fullnode.config.ini /etc/steemd/fullnode.config.ini
ADD configs/broadcast.config.ini /etc/steemd/broadcast.config.ini
ADD configs/ahnode.config.ini /etc/steemd/ahnode.config.ini
ADD configs/fastgen.config.ini /etc/steemd/fastgen.config.ini
ADD configs/testnet.config.ini /etc/steemd/testnet.config.ini

# add normal startup script that starts via sv
ADD contrib/steemd.run /usr/local/bin/steem-sv-run.sh
RUN chmod +x /usr/local/bin/steem-sv-run.sh

# add nginx templates
ADD contrib/steemd.nginx.conf /etc/nginx/steemd.nginx.conf
ADD contrib/healthcheck.conf.template /etc/nginx/healthcheck.conf.template

# add PaaS startup script and service script
ADD contrib/start-paas-steemd.sh /usr/local/bin/start-paas-steemd.sh
ADD contrib/testnet-init.sh /usr/local/bin/testnet-init.sh
ADD contrib/paas-sv-run.sh /usr/local/bin/paas-sv-run.sh
ADD contrib/sync-sv-run.sh /usr/local/bin/sync-sv-run.sh
ADD contrib/healthcheck.sh /usr/local/bin/healthcheck.sh
RUN chmod +x /usr/local/bin/start-paas-steemd.sh
RUN chmod +x /usr/local/bin/testnet-init.sh
RUN chmod +x /usr/local/bin/paas-sv-run.sh
RUN chmod +x /usr/local/bin/sync-sv-run.sh
RUN chmod +x /usr/local/bin/healthcheck.sh

# new entrypoint for all instances
# this enables exitting of the container when the writer node dies
# for PaaS mode (elasticbeanstalk, etc)
# AWS EB Docker requires a non-daemonized entrypoint
ADD contrib/steemd-entrypoint.sh /usr/local/bin/steemd-entrypoint.sh
RUN chmod +x /usr/local/bin/steemd-entrypoint.sh
CMD /usr/local/bin/steemd-entrypoint.sh
