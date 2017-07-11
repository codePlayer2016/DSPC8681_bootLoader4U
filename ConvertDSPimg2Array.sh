#! /bin/bash

# workspace.
DSPWORKSPACE=/home/hawke/gitDSPC8681

# DSP project name.
DSPPRJ=DSPC8681_bootLoader

# Linux project name.
LINUX_PROJECT_NAME=Linux_8681Driver

# Linux project filePath.
LINUX_PROJECT_DIR=/home/hawke/gitDSPC8681

# the output .h file name.
TARGETFILE=DSP_TBL_6678_U0.h

# the output .h file filePath.
TARGETDIR=${LINUX_PROJECT_DIR}/${LINUX_PROJECT_NAME}/inc
echo the output .h file will be put to ${TARGETDIR}

# DSP compile output file name.
SRCOUTFILE=${DSPPRJ}.out

# DSP compile output file filePath.
DSPOUTFILE=${DSPWORKSPACE}/${DSPPRJ}/out/${SRCOUTFILE} 
echo DSP out file dir is ${DSPOUTFILE}
chmod 777 -R ${DSPOUTFILE}

# the tool file filePath.
TITOOLDIR=${DSPWORKSPACE}/${DSPPRJ}/out2hTools
echo "TI TOOLS dir is ${TITOOLDIR}"

TEMPFILE=*_temp.*

# cp the .out file to the tool dir
echo cd ${TITOOLDIR}
chmod -R 777 ${TITOOLDIR}
cd ${TITOOLDIR}
cp ${DSPOUTFILE} ./

# run the .sh to convert th .out to .h
echo elf2HBinU0.sh
./elf2HBinU0.sh
chmod 777 ${TARGETFILE}

cp -f ${TARGETFILE} ${TARGETDIR}

rm -rf ${TEMPFILE} ${SRCOUTFILE} ${TARGETFILE}
#rm -rf ${TEMPFILE} ${SRCOUTFILE}
echo "convert over!"

