set olddir=%CD%

cd ..\..\build\Debug\

grassDTmodel.exe %olddir%\latLAT_lonLON__startYear-01-01_endYear-12-31__configuration__generic_v1.txt 1> %olddir%\stout.txt 2> %olddir%\sterr.txt








