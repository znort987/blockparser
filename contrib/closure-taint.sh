#!/bin/bash

function run()
{
    COMMENT=$1
    CMD=$2

    echo "======================================================================================"
    echo "$COMMENT:"
    echo "$CMD"
    /bin/bash -c "$CMD"
    echo
}

SRCADDR=$1
if test "$SRCADDR" = ""
then
    SRCADDR="1PSf86KnLuzM7Ris5kDhTEZwooR3p2iyfV"   # Known to belong to pirate
fi
SRCSHORT=`echo $SRCADDR | dd bs=1 count=8`

DSTADDR=$2
if test "$DSTADDR" = ""
then
    DSTADDR="1DkyBEKt5S2GDtv7aQw6rQepAvnsRyHoYM"   # Richest address in chain
fi
DSTSHORT=`echo $DSTADDR | dd bs=1 count=8`

run "Compute closure of address $SRCADDR"                       "./parser closure $SRCADDR > $SRCSHORT-CLOSURE"
run "Closure cleanup"                                           "cat $SRCSHORT-CLOSURE | grep -v '^$' | cut -d' ' -f2 | sort | uniq > $SRCSHORT-CLOSURE-CLEAN"
run "See how many addresses we got"                             "wc -l $SRCSHORT-CLOSURE-CLEAN | cut -d' ' -f1"
run "Compute all transactions in and out of closure"            "./parser tx --csv file:$SRCSHORT-CLOSURE-CLEAN > $SRCSHORT-TRANSACTIONS"
run "List cleanup"                                              "cat $SRCSHORT-TRANSACTIONS | grep -v '^$' | grep -v '\"Time' >$SRCSHORT-TRANSACTIONS-CLEAN"
run "See how many tx we got"                                    "wc -l $SRCSHORT-TRANSACTIONS-CLEAN | cut -d' ' -f1"
run "Extract spends"                                            "cat $SRCSHORT-TRANSACTIONS-CLEAN | grep '-' | awk '{print \$3}' | sed -e 's/[\",]//g' | sort | uniq > $SRCSHORT-SPENDS"
run "See how many spends we got"                                "wc -l $SRCSHORT-SPENDS | cut -d' ' -f1"
run "Compute taint of these spends to every tx in the chain "   "./parser taint file:$SRCSHORT-SPENDS > $SRCSHORT-TAINT"
run "Compute list of all tx for dst address"                    "./parser tx --csv $DSTADDR > $DSTSHORT-TX"
run "Compute list of tx spending into dst address"              "cat $DSTSHORT-TX | grep -v '-' | awk '{print \$3}' | sed -e 's/[\",]//g' | sort | uniq > $DSTSHORT-INCOMING-TX"
run "Compute most recent tx spending from fat address"          "cat $DSTSHORT-TX | grep '-' | tail -1 | awk '{print \$3}' | sed -e 's/[\",]//g' | sort | uniq > $DSTSHORT-LATEST-SPEND-TX"

echo "======================================================================================"
echo "Find the taint of each of these tx"
rm -f $SRCSHORT-FAT-TAINT
for tx in `cat $DSTSHORT-INCOMING-TX`
do
    grep $tx $SRCSHORT-TAINT >>$SRCSHORT-$DSTSHORT-TAINT
done
echo

run "Show input taint to $DSTADDR by coins in the closure of $SRCADDR"      "cat $SRCSHORT-$DSTSHORT-TAINT | sort -n"
run "Show overall taint for $DSTADDR by coins in the closure of $SRCADDR"   "grep `cat $DSTSHORT-LATEST-SPEND-TX` $SRCSHORT-TAINT"

