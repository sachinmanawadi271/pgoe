#!/bin/bash


CCG=./CubeCallgraphTool
# output is dumped here
SPEC_OUTPUT=spec-output
# cubex profiles of spec benchmarks
#SPEC_PATH=/home/rness/test/scorep/spec-cubex-best
SPEC_PATH=../spec-profiles

rm -rf $SPEC_OUTPUT
mkdir $SPEC_OUTPUT

BENCHMARKS=`find $SPEC_PATH -name *.cubex`

for bm in $BENCHMARKS ;do
	FILE=$(basename $bm)
	
	echo "running $SPEC_OUTPUT/$FILE.log"

	$CCG $SPEC_PATH/$FILE $1 &> $SPEC_OUTPUT/$FILE.log
done

