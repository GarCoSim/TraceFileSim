#!/usr/bin/env bash
#For each line in TestInputs.txt file, execute simulator based on line's contents
#And compare output log file to various expected outputs in line's contents.
PRINT_FAILS_ONLY=1
RC=0 # Return Code (0 == success; 1 == failure)
if [ -n $2 ] 
then
	if [ "-all" == "$2" ]
	then
		PRINT_FAILS_ONLY=0
	fi
fi
TOLERANCE=0

if [ -z $1 ] || [ ! -f $1 ] 
then
	echo "Invalid input file."
	RC=1
	exit $RC
fi

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
		echo "$1 failed: Number of GC's ($NUMLINES) too different from expected value ($3)"
		RC=1
	else
		if [ $PRINT_FAILS_ONLY -eq 0 ]
		then
			echo "$1 success: Number of GC's"
		fi
	fi
}

memUsed (){
	local finalFree
	finalFree=$(removeNonDataLines $2 | sed -r 's/\s+//g' | awk -F'\\|' '{} END{print $6}')
	
	local returnVal=$(compare $3 $finalFree)
	if [ $returnVal -eq -1 ]
	then
		echo "$1 failed: Final heap size ($finalFree) too different from expected value ($3)"
		RC=1
	else
		if [ $PRINT_FAILS_ONLY -eq 0 ]
		then
			echo "$1 success: Final heap size"
		fi
	fi
}

lastCollectionReason(){
	local reason
	reason=$(removeNonDataLines $2 | sed -r 's/\s+//g' | awk -F'\\|' '{} END{print $2}')
	if [ $reason == $3 ]
	then
		if [ $PRINT_FAILS_ONLY -eq 0 ]
		then
			echo "$1 success: Last collection reason"
		fi
	else
		echo "$1 failed: Last collection reason ($reason) does not match expected reason ($3)"
		RC=1
	fi
}

simCrashed(){
	local lastLine
	lastLine=$(tail -1 $2 | sed -r 's/\s+//g')
	if [[ $lastLine =~ ^[0-9].* ]]
	then
		echo "$1 failed: Simulator crashed during execution."
		RC=1
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

function removeNonDataLines() {
	sed '/^\W*[1-9]/!d' $1
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
		echo "${LINE[0]} failed: Log file not found."
		RC=1
		continue;
	fi
#Since consecutive trials can have the same log file, we have to cut it close on "newness" testing.
#However, since all runs of the simulator have a final "statistics" printout just before execution finish,
#this test shouldn't give any false positives. It may happen, though.
	if test `find ${LINE[2]} -mmin +0.0005` 
	then
		echo "${LINE[0]} failed: Log file not created by this line, or simulator crashed during execution."
		RC=1
		continue;
	fi

#NEW TEST CALLS SHOULD GO HERE
	logTest=$(emptyLog ${LINE[2]})
	if [ $logTest -eq 1 ]
	then
		echo "${LINE[0]} failed: Log file is empty. Simulator may have crashed early in execution."
		RC=1
		continue
	fi
	numGC "${LINE[0]}" ${LINE[2]} ${LINE[3]}
	memUsed "${LINE[0]}" ${LINE[2]} ${LINE[4]}
	lastCollectionReason "${LINE[0]}" ${LINE[2]} ${LINE[5]}
	simCrashed "${LINE[0]}" ${LINE[2]}
done < $1

exit $RC
