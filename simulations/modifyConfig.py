import sys
import os

lat = sys.argv[1]
lon = sys.argv[2]
first_year = sys.argv[3]
last_year = sys.argv[4]
deimsID = sys.argv[5]

##################################
# configuration file #############
##################################

filename = "lat" + lat +"_lon" + lon + "__" + first_year + "-01-01_" + last_year + "-12-31__configuration__generic_v1.txt"

# search for keywords of input data to change
keywordDeims = "deimsID"
newValueDeims = deimsID

keywordLat = "latitude"
newValueLat = lat

keywordLon = "longitude"
newValueLon = lon

keywordStart = "firstYear"
newValueStart = first_year

keywordEnd = "lastYear"
newValueEnd = last_year

keywordWeather = "weatherFile"
newValueWeather = "lat" + lat + "_lon" + lon + "__" + first_year + "-01-01_" + last_year + "-12-31__weather.txt"

keywordSoil = "soilFile"
newValueSoil = "lat" + lat + "_lon" + lon + "__2020__soil.txt"

keywordManagement = "managementFile"
newValueManagement = "lat" + lat + "_lon" + lon + "__" + first_year + "-01-01_" + last_year + "-12-31__management__GER_Schwieder.txt"


# read-in the configuration file
with open(filename, "r", encoding="utf-8") as configFile:
    singleLines = configFile.readlines()
    for i, eachLine in enumerate(singleLines):
        if eachLine.startswith(keywordDeims):
            singleLines[i] = f"{keywordDeims}\t{newValueDeims}\n"
        if eachLine.startswith(keywordLat):
            singleLines[i] = f"{keywordLat}\t{newValueLat}\n"
        if eachLine.startswith(keywordLon):
            singleLines[i] = f"{keywordLon}\t{newValueLon}\n"
        if eachLine.startswith(keywordStart):
            singleLines[i] = f"{keywordStart}\t{newValueStart}\n"
        if eachLine.startswith(keywordEnd):
            singleLines[i] = f"{keywordEnd}\t{newValueEnd}\n"
        if eachLine.startswith(keywordWeather):
            singleLines[i] = f"{keywordWeather}\t{newValueWeather}\n"
        if eachLine.startswith(keywordSoil):
            singleLines[i] = f"{keywordSoil}\t{newValueSoil}\n"
        if eachLine.startswith(keywordManagement):
            singleLines[i] = f"{keywordManagement}\t{newValueManagement}\n"

# write the modified configuration file	
with open(filename, "w", encoding="utf-8") as configFile:
    configFile.writelines(singleLines)

##################################
# model run batch file ###########
##################################

keywordRun = "grassDTmodel.exe"
newValueRun = "%olddir%\lat" + lat + "_lon" + lon + "__" + first_year + "-01-01_" + last_year + "-12-31__configuration__generic_v1.txt 1> %olddir%\stout.txt 2> %olddir%\sterr.txt"

# read-in the batch-file
with open("runSimulation.cmd", "r") as batchFile:
    singleLines = batchFile.readlines()
    for i, eachLine in enumerate(singleLines):
        if eachLine.startswith(keywordRun):
            singleLines[i] = f"{keywordRun} {newValueRun}\n"
            
with open("runSimulation.cmd", "w") as batchFile:
    batchFile.writelines(singleLines)


##################################
# plant traits file ##############
##################################

keywordExternalSeed = "dayOfExternalSeedInfluxStart"
newValueExternalSeed = first_year + "-01-01"

# read-in the parameter-file
with open("../../parameters/BioDT-generic/plant_traits__generic_v1.txt", "r") as parameterFile:
    singleLines = parameterFile.readlines()
    for i, eachLine in enumerate(singleLines):
        if eachLine.startswith(keywordExternalSeed):
            singleLines[i] = f"{keywordExternalSeed}\t{newValueExternalSeed}\n"
            
with open("../../parameters/BioDT-generic/plant_traits__generic_v1.txt", "w") as parameterFile:
    parameterFile.writelines(singleLines)
