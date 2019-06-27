#!/bin/bash
####################################################################################
# If not stated otherwise in this file or this component's Licenses.txt file the
# following copyright and licenses apply:
#
#  Copyright 2018 RDK Management
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
#######################################################################################

if [ -z "$OECORE_TARGET_SYSROOT" ]; then
  echo "OECORE_TARGET_SYSROOT is not set, this script needs to be run in an RNE environment, exiting"
  exit 1
fi

result=1
cur_dir=$PWD
release_dir=partnerapps
function buildgraphics()
{
   result=1
	echo "Entering graphics........"
	cd graphics
	autoreconf -f -i >build.log 2>&1
	./configure --host=arm-linux --prefix=$PKG_CONFIG_SYSROOT_DIR --enable-rendergl \
		--enable-xdgv5 --disable-dependency-tracking --enable-sbprotocol >>build.log 2>&1
	make  >>build.log 2>&1
	if [ $? -eq 0 ] ; then
    mkdir -p $cur_dir/$release_dir/rne-triangle/
		cp rne_triangle $cur_dir/$release_dir/rne-triangle/
		result=0
		echo "Exiting graphics........"
	else
		echo "Failed to build graphics app. Logs are present at $PWD/build.log"
	fi
	cd $cur_dir
}
function buildplayer()
{
   result=1
	echo "Entering player........"
	cd player
	autoreconf -f -i >>build.log 2>&1
	./configure --host=arm-linux --prefix=$PKG_CONFIG_SYSROOT_DIR --enable-rendergl \
		--enable-xdgv5 --disable-dependency-tracking --enable-sbprotocol \
		--enable-player --enable-breakpad >>build.log 2>&1
	make >>build.log 2>&1
	if [ $? -eq 0 ] ; then
    mkdir -p $cur_dir/$release_dir/rne-player/
		cp rne_player $cur_dir/$release_dir/rne-player/
		result=0
		echo "Exiting player........"
	else
		echo "Failed to build player app. Logs are present at $PWD/build.log"
	fi
	cd $cur_dir
}
function buildgraphicslifecycle()
{
   result=1
	echo "Entering graphics-lifecycle........"
	cd graphics-lifecycle
	autoreconf -f -i >>build.log 2>&1
	./configure --host=arm-linux --prefix=$PKG_CONFIG_SYSROOT_DIR --enable-rendergl \
		--enable-xdgv5 --disable-dependency-tracking --enable-sbprotocol \
		--enable-player >>build.log 2>&1
	make >>build.log 2>&1
	if [ $? -eq 0 ] ; then
    mkdir -p $cur_dir/$release_dir/graphics-lifecycle
		cp graphics_lifecycle $cur_dir/$release_dir/graphics-lifecycle
		result=0
		echo "Exiting graphics-lifecycle........"
	else
		echo "Failed to build graphics_lifecycle app. Logs are present at $PWD/build.log"
	fi
	cd $cur_dir
}
function buildmseplayer()
{
   result=1
	echo "Entering mse-player........"
	cd mse-player
	autoreconf -f -i >>build.log 2>&1
	./configure --host=arm-linux --prefix=$PKG_CONFIG_SYSROOT_DIR >>build.log 2>&1
	make >>build.log 2>&1
	if [ $? -eq 0 ] ; then
    mkdir -p $cur_dir/$release_dir/mse-player/
		cp mse_player $cur_dir/$release_dir/mse-player/
		cp -r mse_frames $cur_dir/$release_dir/mse-player/
		result=0
		echo "Exiting mse-player........"
	else
		echo "Failed to build mse_player. Logs are present at $PWD/build.log"
	fi
	cd $cur_dir
}
function buildaampplayer()
{
   result=1
    echo "Entering aamp-player........"
    cd aamp-player
    autoreconf -f -i >>build.log 2>&1
    ./configure --host=arm-linux --prefix=$PKG_CONFIG_SYSROOT_DIR >>build.log 2>&1
    make >>build.log 2>&1
    if [ $? -eq 0 ] ; then
    mkdir -p $cur_dir/$release_dir/aamp-player/
        cp aampplayer $cur_dir/$release_dir/aamp-player/
        result=0
        echo "Exiting aamp-player........"
    else
        echo "Failed to build aampplayer. Logs are present at $PWD/build.log"
    fi
    cd $cur_dir
}

mkdir -p $release_dir
cp appmanagerregistry.conf $release_dir/
cp run_partner_app.sh $release_dir/
cp run_westeros.sh $release_dir/

buildgraphics
if [ $result -ne 0 ] ; then
	echo "Exiting........"
   exit 1
fi
buildplayer
if [ $result -ne 0 ] ; then
	echo "Exiting........"
   exit 1
fi
buildgraphicslifecycle
if [ $result -ne 0 ] ; then
	echo "Exiting........"
   exit 1
fi
buildmseplayer
if [ $result -ne 0 ] ; then
	echo "Exiting........"
   exit 1
fi
#buildaampplayer
#if [ $result -ne 0 ] ; then
#    echo "Exiting........"
#   exit 1
#fi	
echo -e "All done..\nFour applications are compiled and installed in $release_dir directory\n\
rne-triangle will demostrate the graphics capabilities\n\
rne-player will demostrate how to play a video 
graphics-lifecycle extends rne_triangle to support suspend and resume 
mse-player shows how to build a simple MSE-like player using gstreamer and essos"
#aamp-player demonstrates how to use AAMP player to play a video" 

