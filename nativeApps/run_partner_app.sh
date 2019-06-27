#!/bin/sh
##########################################################################
# If not stated otherwise in this file or this component's Licenses.txt
# file the following copyright and licenses apply:
#
# Copyright 2018 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
##########################################################################

is_raspberrypi=`uname -a | grep -i raspberrypi`

if [ ! -z "$is_raspberrypi" ]; then
  export XDG_RUNTIME_DIR=/run
  export WAYLAND_DISPLAY=wayland-0
else #comcast device
  export XDG_RUNTIME_DIR=/tmp/
  export WAYLAND_DISPLAY=mydisplay
  export LD_PRELOAD=libwayland-egl.so.0
fi

/usb/partnerapps/rne-triangle/rne_triangle
