# esphome-devices

This is a list of my HomeAssistant ESPhome devices.
Few of them are not used actively.

For a device to appear in this list the following needs to be done as root:
~~~
sudo -i
cd /armbian/haos/homeassistant/esphome/
ln device.yaml /home/leon/esphome-devices/
exit
cd ~/esphome-devices/
git add device.yaml
git commit -m "New device"
git push
~~~

Editing and building should be done within HomeAssistant ESPHome Builder.
