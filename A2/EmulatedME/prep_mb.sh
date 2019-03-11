file=$1
sed -i '/^\[h264/!d' $file
sed -i '/nal_/d' $file
sed -i '/user data/d' $file
sed -i '/Reinit context/d' $file
sed -i -e 1,46d $file
