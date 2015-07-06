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
    rm /data/data/com.monet.connectme/files/ConnectTo
    sleep 1
    echo "openWifiAp,TDT,password" > /data/data/com.monet.connectme/files/ConnectTo
    sleep 1
    monkey -p com.monet.connectme -c android.intent.category.LAUNCHER 1
fi
