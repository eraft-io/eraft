set -e

echo pass | sudo -S git clone --branch v1.9.2 https://github.com/gabime/spdlog.git && cd spdlog && echo pass | sudo -S  mkdir build && cd build \
       && echo pass | sudo -S  cmake .. && echo pass | sudo -S  make -j && echo pass | sudo -S make install

