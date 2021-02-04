#!/bin/bash
# ---------------------------------------------------------------------------
# install.sh -- Install AutoTag
#
# Copyright 2021 Panagiotis Dimopoulos (panosdim@gmail.com)
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License at (http://www.gnu.org/licenses/) for
# more details.
#
# Version: 1.0
# ---------------------------------------------------------------------------

if [ "$EUID" -ne 0 ]; then
  exec sudo bash "$0" "$@"
fi

INSTALL_PATH=/opt/autotag
SYSTEMD_PATH=/etc/systemd/system
BUILD_PATH=./cmake-build-release

mkdir -p $INSTALL_PATH
cp $BUILD_PATH/autotag $INSTALL_PATH

read -p "Provide the TMDB API key? " key
sed "s|apiKey = |apiKey = $key|g" config.ini >$INSTALL_PATH/config.ini

# Function to install systemd service
function install_service() {
  read -p "Provide the path to movies files that AutoTag will watch? " path
  cp autotag.service tmp.service
  sed -i "s|<PATH>|$path|g" tmp.service
  mv tmp.service $SYSTEMD_PATH/autotag.service

  systemctl start autotag.service
  systemctl enable autotag.service
}

read -p "Do you wish to install a systemd service? " yn
case $yn in
[Yy]*) install_service ;;
[Nn]*) exit ;;
*) echo "Please answer yes or no." ;;
esac
