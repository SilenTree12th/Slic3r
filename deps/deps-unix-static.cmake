
ExternalProject_Add(dep_boost
    EXCLUDE_FROM_ALL 1
    URL "https://dl.bintray.com/boostorg/release/1.66.0/source/boost_1_66_0.tar.gz"
    URL_HASH SHA256=bd0df411efd9a585e5a2212275f8762079fed8842264954675a4fddc46cfcf60
    BUILD_IN_SOURCE 1
    CONFIGURE_COMMAND ./bootstrap.sh
        --with-libraries=system,filesystem,thread,log,locale,regex
        "--prefix=${DESTDIR}/usr/local"
    BUILD_COMMAND ./b2
        -j ${NPROC}
        link=static
        variant=release
        threading=multi
        boost.locale.icu=off
        cxxflags=-fPIC cflags=-fPIC
        install
    INSTALL_COMMAND ""   # b2 does that already
)

ExternalProject_Add(dep_tbb
    EXCLUDE_FROM_ALL 1
    URL "https://github.com/wjakob/tbb/archive/a0dc9bf76d0120f917b641ed095360448cabc85b.tar.gz"
    URL_HASH SHA256=0545cb6033bd1873fcae3ea304def720a380a88292726943ae3b9b207f322efe
    CMAKE_ARGS -DTBB_BUILD_SHARED=OFF
        -DTBB_BUILD_TESTS=OFF
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON
    INSTALL_COMMAND make install "DESTDIR=${DESTDIR}"
)

ExternalProject_Add(dep_gtest
    EXCLUDE_FROM_ALL 1
    URL "https://github.com/google/googletest/archive/release-1.8.1.tar.gz"
    URL_HASH SHA256=9bf1fe5182a604b4135edc1a425ae356c9ad15e9b23f9f12a02e80184c3a249c
    CMAKE_ARGS -DBUILD_GMOCK=OFF
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON
    INSTALL_COMMAND make install "DESTDIR=${DESTDIR}"
)

ExternalProject_Add(dep_nlopt
    EXCLUDE_FROM_ALL 1
    URL "https://github.com/stevengj/nlopt/archive/v2.5.0.tar.gz"
    URL_HASH SHA256=c6dd7a5701fff8ad5ebb45a3dc8e757e61d52658de3918e38bab233e7fd3b4ae
    CMAKE_GENERATOR "${DEP_MSVC_GEN}"
    CMAKE_ARGS
        -DNLOPT_PYTHON=OFF
        -DNLOPT_OCTAVE=OFF
        -DNLOPT_MATLAB=OFF
        -DNLOPT_GUILE=OFF
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON
    INSTALL_COMMAND make install "DESTDIR=${DESTDIR}"
    INSTALL_COMMAND ""
)

ExternalProject_Add(dep_zlib
    EXCLUDE_FROM_ALL 1
    URL "https://zlib.net/zlib-1.2.11.tar.xz"
    URL_HASH SHA256=4ff941449631ace0d4d203e3483be9dbc9da454084111f97ea0a2114e19bf066
    CMAKE_GENERATOR "${DEP_MSVC_GEN}"
    CMAKE_ARGS
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON
    INSTALL_COMMAND make install "DESTDIR=${DESTDIR}"
    INSTALL_COMMAND ""
)

ExternalProject_Add(dep_libpng
    DEPENDS dep_zlib
    EXCLUDE_FROM_ALL 1
    URL "http://prdownloads.sourceforge.net/libpng/libpng-1.6.35.tar.xz?download"
    URL_HASH SHA256=23912ec8c9584917ed9b09c5023465d71709dce089be503c7867fec68a93bcd7
    CMAKE_GENERATOR "${DEP_MSVC_GEN}"
    CMAKE_ARGS
        -DPNG_SHARED=OFF
        -DPNG_TESTS=OFF
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON
    INSTALL_COMMAND make install "DESTDIR=${DESTDIR}"
    INSTALL_COMMAND ""
)

ExternalProject_Add(dep_libopenssl
    EXCLUDE_FROM_ALL 1
    URL "https://github.com/openssl/openssl/archive/OpenSSL_1_1_0g.tar.gz"
    URL_HASH SHA256=8e9516b8635bb9113c51a7b5b27f9027692a56b104e75b709e588c3ffd6a0422
    BUILD_IN_SOURCE 1
    CONFIGURE_COMMAND ./config
        "--prefix=${DESTDIR}/usr/local"
        no-shared
        no-ssl3-method
        no-dynamic-engine
        -Wa,--noexecstack
    BUILD_COMMAND make depend && make "-j${NPROC}"
    INSTALL_COMMAND make install_sw
)

ExternalProject_Add(dep_libcurl
    EXCLUDE_FROM_ALL 1
    DEPENDS dep_libopenssl
    URL "https://curl.haxx.se/download/curl-7.58.0.tar.gz"
    URL_HASH SHA256=cc245bf9a1a42a45df491501d97d5593392a03f7b4f07b952793518d97666115
    BUILD_IN_SOURCE 1
    CONFIGURE_COMMAND ./configure
        --enable-static
        --disable-shared
        "--with-ssl=${DESTDIR}/usr/local"
        --with-pic
        --enable-ipv6
        --enable-versioned-symbols
        --enable-threaded-resolver
        --with-random=/dev/urandom
        --with-ca-bundle=/etc/ssl/certs/ca-certificates.crt
        --disable-ldap
        --disable-ldaps
        --disable-manual
        --disable-rtsp
        --disable-dict
        --disable-telnet
        --disable-pop3
        --disable-imap
        --disable-smb
        --disable-smtp
        --disable-gopher
        --disable-crypto-auth
        --without-gssapi
        --without-libpsl
        --without-libidn2
        --without-gnutls
        --without-polarssl
        --without-mbedtls
        --without-cyassl
        --without-nss
        --without-axtls
        --without-brotli
        --without-libmetalink
        --without-libssh
        --without-libssh2
        --without-librtmp
        --without-nghttp2
        --without-zsh-functions-dir
    BUILD_COMMAND make "-j${NPROC}"
    INSTALL_COMMAND make install "DESTDIR=${DESTDIR}"
)

ExternalProject_Add(dep_wxwidgets
    EXCLUDE_FROM_ALL 1
    URL "https://github.com/wxWidgets/wxWidgets/releases/download/v3.1.1/wxWidgets-3.1.1.tar.bz2"
    URL_HASH SHA256=c925dfe17e8f8b09eb7ea9bfdcfcc13696a3e14e92750effd839f5e10726159e
    BUILD_IN_SOURCE 1
    CONFIGURE_COMMAND ./configure
        "--prefix=${DESTDIR}/usr/local"
        --disable-shared
        --with-gtk=2
        --with-opengl
        --enable-unicode
        --enable-graphics_ctx
        --with-regex=builtin
        --with-libpng=builtin
        --with-libxpm=builtin
        --with-libjpeg=builtin
        --with-libtiff=builtin
        --with-zlib=builtin
        --with-expat=builtin
        --disable-precomp-headers
        --enable-debug_info
        --enable-debug_gdb
    BUILD_COMMAND make "-j${NPROC}" && make -C locale allmo
    INSTALL_COMMAND make install
)
