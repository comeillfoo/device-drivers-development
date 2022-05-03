# Лабораторная работа 3

**Название:** "Разработка драйверов сетевых устройств"

**Цель работы:** получить знания и навыки разработки драйверов сетевых интерфейсов для операционной системы Linux

## Описание функциональности драйвера

Данный драйвер надстраивается над интерфейсом указанным в `link` и анализирует принимаемые и отправляемые пакеты, которые затем возвращает далее интерфейсу ниже.
Анализ пакетов заключается в том, что подсчитываем статистику только по IP пакетам версии 4, и которые отправляются на определенный IP адрес, указанный в `destipv4`.
Информацию о своей работе модуль пишет в кольцевой буфер ядра.
По умолчанию модуль считает родительским интерфейсом `enp0s3`, а прослушиваемый адрес -- `192.168.0.1`.

## Инструкция по сборке

1. `make` -- собрать исходные файлы в динамически-загружаемый модуль
2. `sudo make do` -- загрузить модуль с параметрами по умолчанию
    + `sudo insmod virt_net_if.ko link=<название надстраиваемого интерфейса> destipv4=<прослушиваемый адрес получателя>` -- загрузить  модуль с пользовательскими параметрами

## Инструкция пользователя

1. Собрать драйвер и загрузить в ядро ( [см. Инструкцию по сборке](#инструкция-по-сборке) )
2. Изредка просматривать кольцевой буфер ядра на наличие ошибок или на содержимое захваченных пакетов
3. Можно проверять статистику работы интерфейса с помощью `ifconfig vni0`

## Примеры использования

```
make
sudo insmod virt_net_if.ko destipv4=10.0.2.15
...
ifconfig vni0
```
---
```
[  778.823132] virt_net_if: loading out-of-tree module taints kernel.
[  778.823187] virt_net_if: module verification failed: signature and/or required key missing - tainting kernel
[  778.826031] Module virt_net_if loaded
[  778.826033] virt_net_if: create link vni0
[  778.826034] virt_net_if: registered rx handler for enp0s3
[  778.857248] vni0: device opened
[  778.873953] Checking Frame: Protocol: 800, Version: 4, daddr: 255.255.255.255, listening daddr: 10.0.2.15
[  778.874285] Checking Frame: Protocol: 800, Version: 4, daddr: 10.0.2.15, listening daddr: 10.0.2.15
[  778.874290] Captured IPv4 packet, saddr: 10.0.2.2; daddr: 10.0.2.15
[  778.874292] Data length: 556. Data:

[  778.884449] Checking Frame: Protocol: 86dd, Version: 6, daddr: 0.0.0.0, listening daddr: 10.0.2.15
[  778.905690] Checking Frame: Protocol: 806, Version: 0, daddr: 2.15.0.0, listening daddr: 10.0.2.15
...
vni0: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500
        inet 10.0.2.15  netmask 255.255.255.0  broadcast 10.0.2.255
        inet6 fe80::59bc:5577:93e0:8fa  prefixlen 64  scopeid 0x20<link>
        ether 08:00:27:dc:6e:b6  txqueuelen 1000  (Ethernet)
        RX packets 46  bytes 15048 (15.0 KB)
        RX errors 0  dropped 0  overruns 0  frame 0
        TX packets 0  bytes 0 (0.0 B)
        TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0
```
