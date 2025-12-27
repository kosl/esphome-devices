# esphome-devices

This is a list of my HomeAssistant ESPhome devices.
Few of them are not used actively.

For a HomaAssistant File Editor (Configurator) it is required that 
Git is initialized therein and only later a new `.git` is created with
~~~ sh
sudo -i
cd /armbian/haos/homeassistant/esphome/
git clone this_repo .
chgrp -R leon .git README.md *.yaml .gitignore
chmod -R g+w .git README.md *.yaml .gitignore
# then as a user symlink 
ln -s /armbian/haos/homeassistant/esphome/ ${HOME}/esphome-devices
~~~
Subsequently, as a user can commit new device with
~~~ sh
cd ~/esphome-devices/
git add device.yaml
git commit -m "New device"
git push
~~~

Editing and building should be done within HomeAssistant ESPHome Builder.
