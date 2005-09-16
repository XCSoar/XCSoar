@echo off
dir /b *.exe > processor.lst
for /F %%j in (processor.lst) do (
	echo "Rename %%j to Version-4_5_beta2-%%j"
	ren %%j Version-4_5_beta2-%%j
)