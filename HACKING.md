# Development notes

## Building the injected library for debugging

```bash
mkdir build-arm
cd build-arm
cmake \
  -DANDROID_PLATFORM=19 \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_TOOLCHAIN_FILE=$HOME/Android/Sdk/ndk/25.2.9519653/build/cmake/android.toolchain.cmake \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DEMULATOR=OFF \
  -DDEBUG_HACK=ON \
  -DSTRIP=OFF \
  ..
```

To push it

```bash
ssh root@192.168.8.1 mount -o remount,rw /app
scp custom-menu/libbalong_custom_menu.so root@192.168.8.1:/app/hijack/lib/
scp custom-menu/ui/libui.so root@192.168.8.1:/app/hijack/lib/
```

## Debugging the injected library

Using this [`gdbserver`](https://github.com/therealsaumil/static-arm-bins/blob/master/gdbserver-armel-static-8.0.1)

On the device:

```bash
LD_LIBRARY_PATH="/app/hijack/lib/" LD_PRELOAD="/app/hijack/lib/libbalong_custom_menu.so" /online/bin/gdbserver-armel-
static-8.0.1 --multi :1234
```

On the host machine:

```bash
# Symlink the libraries for debugging
WD=$(pwd)
pushd ~/router/rootfs/app/hijack/lib
ln -s $WD/custom-menu/libbalong_custom_menu.so libbalong_custom_menu.so
ln -s $WD/custom-menu/ui/libui.so libui.so
popd

# CLion's multiarch GDB is quite convenient for this
~/JetBrains/clion/bin/gdb/linux/x64/bin/gdb \
  -ex 'set sysroot ~/router/rootfs' \
  -ex 'target extended-remote tcp:192.168.8.1:1234' \
  -ex 'set remote exec-file /app/bin/oled.orig' \
  -ex 'set solib-search-path ~/router/rootfs/app/hijack/lib:~/router/rootfs/app/lib:~/router/rootfs/system/lib' \
  -ex 'run'
```
