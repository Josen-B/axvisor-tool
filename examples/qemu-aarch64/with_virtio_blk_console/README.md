本目录下的配置文件，用于在QEMU aarch64下启动axvisor，并启动一个linux作为vm1，vm1使用axvisor-tool提供的Virtio disk和Virtio console作为自己的磁盘和终端设备。进入Root Linux后，可参考如下命令启动vm1：
```
insmod axvisor.ko
mount -t proc proc /proc
mount -t sysfs sysfs /sys
rm nohup.out
mkdir -p /dev/pts
mount -t devpts devpts /dev/pts
nohup ./axvisor virtio start virtio_cfg.json &
./axvisor vm start vm1_linux.json && \
cat nohup.out | grep "char device" && \
script /dev/null
```