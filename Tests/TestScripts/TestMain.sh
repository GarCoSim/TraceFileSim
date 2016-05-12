#!/usr/bin/env bash
#For each line in TestInputs.txt file, execute simulator based on line's contents
#And compare output log file to various expected outputs in line's contents.
TOLERANCE=0.10
TEMPNAME=""

#Arguments: (trialNum, logLocation, expectedVal)
emptyLog (){
	local NUMLINES=0
	while read -r OUTPUT_LINE; do
		case ${OUTPUT_LINE} in 
			[0-9]*)
			((NUMLINES++))
		esac
	done < $1
	
	if [ $NUMLINES -eq 0 ]
	then
		echo 1
	else
		echo 0
	fi
}

numGC () {
	local NUMLINES=0
	while read -r OUTPUT_LINE; do
		case ${OUTPUT_LINE} in 
			[0-9]*)
			((NUMLINES++))
		esac
	done < $2

	local returnVal=$(compare $3 $NUMLINES)
	if [ $returnVal -eq -1 ]
	then
		echo "Trial $1 failed: Number of GC's ($NUMLINES) too different from expected value ($3)"
	else
		echo "Trial $1 succeeded numGC test"
	fi
}

memUsed (){
	local finalFree
	while read -ra OUTPUT_LINE; do
		case ${OUTPUT_LINE} in 
			[0-9]*)
			finalFree=${OUTPUT_LINE[12]}
		esac
	done < $2
	local returnVal=$(compare $3 $finalFree)
	if [ $returnVal -eq -1 ]
	then
		echo "Trial $1 failed: Final Heap Used ($finalFree) too different from expected value ($3)"
	else
		echo "Trial $1 succeeded free heap test"
	fi
}

#Arguments: (Expected value, Experimental value)
compare(){
	local expectedVal=$1
	local experimentalVal=$2
	local tol=$(echo "$expectedVal * $TOLERANCE" | bc -l)
	local lowerBound=$(echo "$expectedVal-$tol" | bc -l)
	local upperBound=$(echo "$expectedVal+$tol" | bc -l)
	
	local lowerCompare=$(echo "$experimentalVal < $lowerBound" | bc -l)
	local upperCompare=$(echo "$experimentalVal > $upperBound" | bc -l)
	if [ $lowerCompare = 1 ] || [ $upperCompare = 1 ]
	then
		echo -1
	else
		echo 1
	fi
}



#Elements of LINE: (trialNum, commandLine, logLocation, [various tests])
while IFS=';' read -ra LINE; do
	case "${LINE[0]}" in 
		\#*) continue ;; 
		"") continue ;;
	esac

#Run simulator
	eval ${LINE[1]} &> /dev/null
	
#No way to intercept log files during execution. Instead, check if log file exists, and if it is relatively new
	if [ ! -e ${LINE[2]} ]
	then
		echo "Trial ${LINE[0]} failed: Log file not found."
		continue;
	fi
	
#Since consecutive trials can have the same log file, we have to cut it close on "newness" testing.
#However, since all runs of the simulator have a final "statistics" printout just before execution finish,
#this test shouldn't give any false positives. It may happen, though.
	if test `find ${LINE[2]} -mmin +0.0001` 
	then
		echo "Trial ${LINE[0]} failed: Log file too old to have been created by this script."
		continue;
	fi

#NEW TEST CALLS SHOULD GO HERE
	logTest=$(emptyLog ${LINE[2]})
	if [ $logTest -eq 1 ]
	then
		echo "Trial ${LINE[0]} failed: Log file is empty. Simulator may have crashed early in execution."
		continue
	fi
	numGC ${LINE[0]} ${LINE[2]} ${LINE[3]}
	memUsed ${LINE[0]} ${LINE[2]} ${LINE[4]}
	
done < TestInputs.txt

