mingw32-make.exe CXXFLAGS="-march=native -std=c++11 -DNDEBUG -g2 -O3 -fPIC -DCRYPTOPP_DISABLE_SSSE4 -DCRYPTOPP_DISABLE_SSSE3 -DCRYPTOPP_DISABLE_SHA" static
mingw32-make.exe install PREFIX=../cryptopp-install-windows
mingw32-make.exe clean