#!/bin/bash

usage()
{
    echo "Usage:"
    echo
    echo "      sh $(basename $0) [-t TREENAME=Events] ROOTFILE"
    echo
    exit
}

# Parsing command-line opts
while getopts ":th" OPTION; do
    case $OPTION in
        t) TREENAME=${OPTARG};;
        h) usage;;
        :) usage;;
    esac
done

# Set default tree name if not provided
if [ -z ${TREENAME} ]; then TREENAME=Events; fi

# to shift away the parsed options
shift $(($OPTIND - 1))

# Check root file argument
if [ -z $1 ]; then
    echo "Error: no root file name was provided."
    usage
else
    FILENAME=$1
fi

# Write the script to a temporary file in order to avoid clash when running parallel
MACRONAME=$(mktemp stupid_numbers_XXXXXXXXX)
MACRO=/tmp/${MACRONAME}.C

# Dumping the macro to the tmp file
#------------------------------------------------------------
echo "#include \"TFile.h\"
#include \"TTree.h\"
#include \"TString.h\"

int ${MACRONAME}(TString fname, TString treename)
{
    TFile* f = new TFile(fname, \"open\");
    if (!f)
    {
        printf(\"[RSR] file is screwed up\n\");
        return 1;
    }
    TTree* t = (TTree*) f->Get(treename);
    if (!t)
    {
        printf(\"[RSR] tree is screwed up\n\");
        return 1;
    }

    Long64_t nentries = t->GetEntries();
    printf(\"[RSR] ntuple has %lld events\n\", nentries);

    bool foundBad = false;
    for (Long64_t ientry = 0; ientry < t->GetEntries(); ++ientry)
    {
        Int_t bytes_read = t->GetEntry();
        if (bytes_read < 0)
        {
            foundBad = true;
            printf(\"[RSR] found bad event %lld\n\", nentries);
            return 1;
        }
    }

    if (!foundBad)
    {
        printf(\"[RSR] passed the rigorous sweeproot\n\");
        return 0;
    }
    return 0;
}" > $MACRO
#------------------------------------------------------------

# Perform a rigorous sweep
root -l -b -q ${MACRO}+\(\"$FILENAME\",\"$TREENAME\"\)

# If return code is non-zero delete the file
if [ $? -ne 0 ]; then
    echo "[RSR] failed the rigorous sweeproot"
    echo "[RSR] Deleting the file $FILENAME"
    rm $FILENAME
fi

#eof
