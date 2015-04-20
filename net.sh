#!/system/bin/sh

if [ $# -ne 1 ]; then
    return
fi

if [ $1 -eq 1 ]; then
    svc wifi enable
elif [ $1 -eq 2 ]; then
    svc wifi disable
elif [ $1 -eq 3 ]; then
    svc data enable
elif [ $1 -eq 4 ]; then
    svc data disable
elif [ $1 -eq 5 ]; then
    ping -c 3 www.baidu.com | grep loss
elif [ $1 -eq 6 ]; then
    dumpsys wifi | grep -m 1 Wi-Fi
elif [ $1 -eq 7 ]; then
    mount -o remount,rw /system
    mv /system/bin/wpa_supplicant /system/bin/wpa_supplicant.bak
    var1=`ps | grep wpa_supplicant`

    if [[ $var1 != "" ]]; then
        wpa_sup=${var1:9:7}
        kill $wpa_sup
    fi

    #insmod driver modules
    insmod /system/lib/modules/wlan.ko
    sleep 1
    netcfg wlan0 up
    sleep 1


    #kill hostapd
    var2=`ps | grep hostapd`
    if [[ $var2 != "" ]]; then
        hostapd=${var2:9:7}
        kill $hostapd
    fi

    #start hostapd process
    chmod 777 /data/misc/wifi/hostapd.conf
    hostapd -B /data/misc/wifi/hostapd.conf

    #assign IP to interface wlan0
    ifconfig wlan0 192.168.1.1 netmask 255.255.255.0
fi
