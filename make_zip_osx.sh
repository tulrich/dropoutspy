#!/bin/bash

datestamp=`date +%Y%m%d`
zipfile=dropoutspy-${datestamp}.zip
rm zipfile
cd Builds/MacOSX/build/Release32
zip -r ../../../../${zipfile} dropoutspy32.vst
cd ../../../..
cd Builds/MacOSX/build/Release64
zip -r ../../../../${zipfile} dropoutspy64.vst
