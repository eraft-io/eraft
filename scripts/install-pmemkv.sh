# install pmemkv
set -e

git clone https://github.com/pmem/pmemkv.git; cd pmemkv; git checkout 0b4bb19b5cbeccfcab9e9896d458ad863bb944a7; mkdir build;
cd build; cmake .. -DENGINE_RADIX=ON; make -j2; make install; cd ../..;
