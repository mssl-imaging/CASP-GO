export LANG=en_US.UTF-8

/../anaconda2/bin/activate
conda init

export ISISROOT=/../isis/isis3/isis3.5
export ISIS3DATA=/../isis/isis3/data
. $ISISROOT/scripts/isis3Startup.sh isis3

export LD_LIBRARY_PATH=/../anaconda2/lib:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH=/../CASP-GO/extraLib:$LD_LIBRARY_PATH

#export LD_LIBRARY_PATH=/usr/lib:$LD_LIBRARY_PATH
#export LD_LIBRARY_PATH=/opt/glibc-2.14/lib:$LD_LIBRARY_PATH
