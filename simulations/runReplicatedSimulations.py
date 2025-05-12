import sys
import os

#print ('argument list', sys.argv)
lat = sys.argv[1]
lon = sys.argv[2]
first_year = sys.argv[3]
last_year = sys.argv[4]

filename = "lat" + lat +"_lon" + lon + "__" + first_year + "-01-01_" + last_year + "-12-31__configuration__generic_v1.txt"

# search for keywords of input data to change
keywordSeed = "randomNumberGeneratorSeed"

for i in range(11):
    newValueSeed = i
    # read-in the configuration file
    with open(filename, "r", encoding="utf-8") as configFile:
        singleLines = configFile.readlines()
        for i, eachLine in enumerate(singleLines):
            if eachLine.startswith(keywordSeed):
                singleLines[i] = f"{keywordSeed}\t{newValueSeed}\n"
    # write the modified configuration file	
    with open(filename, "w", encoding="utf-8") as configFile:
        configFile.writelines(singleLines)
    os.system('runSimulation.cmd')


