ifile=$1
ofile1=$2
ofile2=$3
mvsfile=$4
mbfile=$5
echo "Starting Mb Type Generation"
ffmpeg -y -debug mb_type -thread_type none -i $ifile $ofile1 2> $mbfile
echo "Prepping Mb Type File"
bash prep_mb.sh $mbfile
python remove_scene.py $mbfile
echo "Upconverting"
python frcUP.py $ifile $ofile2 $mvsfile $mbfile 
echo "Done Upconverting"
