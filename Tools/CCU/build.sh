#!/bin/sh

cd HB-PB-1-6_CCU-addon-src
chmod +x update_script
tar -H gnu -czvf HB-PB-1-6_CCU-addon.tgz *
mv HB-PB-1-6_CCU-addon.tgz ..
cd ..