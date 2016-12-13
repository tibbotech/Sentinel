@echo off
echo Cleaning...
@del /Q .\compressed\*.gz
echo Compressing...
for %%F in (*.js, *.svg, *.css, favicon.ico) do (
	echo ** %%F
	zopfli --i1000 %%F
	@copy /y /b %%F.gz compressed\%%F.gz >nul
	@del /Q %%F.gz
)