#/export CGT_INSTALL_DIR=/opt/ti/TI_CGT_C6000_7.4.0
export CGT_INSTALL_DIR=/opt/ti/ccsv5/tools/compiler/c6000_7.4.4
export TARGET=6678
export ENDIAN=little
export UNUM=U0

# project name.
DSPPRJ=DSPC8681_bootLoader
DSP_OUT_FILE=DSPC8681_bootLoader4U

# input file name.
SRCOUTFILE=${DSP_OUT_FILE}.out

# output file name.
OUTPUTFILE=DSP_TBL

# array name
ARRAYNAME=_thirdBLCode_U0

echo CGT_INSTALL_DIR set as: ${CGT_INSTALL_DIR}
echo TARGET set as: ${TARGET}
echo Converting .out to HEX ...
if [ ${ENDIAN} == little ]
then
${CGT_INSTALL_DIR}/bin/hex6x -order L DPUcoreCtl.rmd ${SRCOUTFILE}
else
${CGT_INSTALL_DIR}/bin/hex6x -order M DPUcoreCtl.rmd ${SRCOUTFILE}
fi

./Bttbl2Hfile DPUcore_temp.btbl DPUcore_temp.h DPUcore_temp.bin

./hfile2array DPUcore_temp.h DPUcore.h _thirdBLCode_U0

if [ ${ENDIAN} == little ]
then
#mv DPUcore.h ${OUTPUTFILE}_${TARGET}_${UNUM}.h
mv DPUcore.h ${OUTPUTFILE}_${TARGET}.h
else
#mv DPUcore.h ${OUTPUTFILE}_${TARGET}_${UNUM}.h
mv DPUcore.h ${OUTPUTFILE}_${TARGET}.h
fi
#chmod -R 777 ${OUTPUTFILE}_${TARGET}_${UNUM}.h
chmod -R 777 ${OUTPUTFILE}_${TARGET}.h
