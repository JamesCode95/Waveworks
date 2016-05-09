@ECHO OFF

REM Command file for Sphinx documentation

REM looks in cwd for source
cd %~dp0

rem Find DEVREL_ROOT by searching backwards from cwd for libdev\external\sphinx
if "%DEVREL_ROOT%" == "" (
  set DEVREL_ROOT=%~p0
)
:rootloop
if exist %DEVREL_ROOT%libdev\external\sphinx\python-2.7.5\python.exe goto :haverootpath
for /F %%i in ("%DEVREL_ROOT%") DO @set OLD_ROOT=%%~fi
set TMP_ROOT=%DEVREL_ROOT%..\
for /F %%i in ("%TMP_ROOT%") DO @set DEVREL_ROOT=%%~fi
if "%OLD_ROOT%" == "%DEVREL_ROOT%" (
  echo Cannot find Sphinx.  Please either sync p4sw://sw/devrel/libdev/external/sphinx
  echo or set the DEVREL_ROOT env. variable manually
  goto :end
)
goto :rootloop
:haverootpath

set SPHINX_ROOT=%DEVREL_ROOT%libdev\external\sphinx
echo Found Sphinx in the following location: %SPHINX_ROOT%

if NOT EXIST _static (
  mkdir _static
)

if "%SPHINXBUILD%" == "" (
	set SPHINXBUILD="%SPHINX_ROOT%\python-2.7.5\python.exe" "%SPHINX_ROOT%\python-2.7.5\Scripts\sphinx-build-script.py"
)

set OUTDIR=%~dp2
if "%2" == "" (
	set OUTDIR=%~dp0\..\..\output\docs
)
set INTDIR=%~dp3
if "%3" == "" (
	set INTDIR=%~dp0\..\..\intermediate\docs
)
set ALLSPHINXOPTS=-d %INTDIR%/doctrees %SPHINXOPTS% .
set I18NSPHINXOPTS=%SPHINXOPTS% .
if NOT "%PAPER%" == "" (
	set ALLSPHINXOPTS=-D latex_paper_size=%PAPER% %ALLSPHINXOPTS%
	set I18NSPHINXOPTS=-D latex_paper_size=%PAPER% %I18NSPHINXOPTS%
)

if "%1" == "help" (
	:help
	echo.Please use `make ^<target^> ^<outdir^> ^<intdir^>` where ^<target^> is one of
	echo.  html       to make standalone HTML files
	echo.  dirhtml    to make HTML files named index.html in directories
	echo.  singlehtml to make a single large HTML file
	echo.  pickle     to make pickle files
	echo.  json       to make JSON files
	echo.  htmlhelp   to make HTML files and a HTML help project
	echo.  qthelp     to make HTML files and a qthelp project
	echo.  devhelp    to make HTML files and a Devhelp project
	echo.  epub       to make an epub
	echo.  latex      to make LaTeX files, you can set PAPER=a4 or PAPER=letter
	echo.  text       to make text files
	echo.  man        to make manual pages
	echo.  texinfo    to make Texinfo files
	echo.  gettext    to make PO message catalogs
	echo.  changes    to make an overview over all changed/added/deprecated items
	echo.  xml        to make Docutils-native XML files
	echo.  pseudoxml  to make pseudoxml-XML files for display purposes
	echo.  linkcheck  to check all external links for integrity
	echo.  doctest    to run all doctests embedded in the documentation if enabled
	goto end
)

if "%1" == "clean" (
	for /d %%i in (%OUTDIR%\*) do rmdir /q /s %%i
	del /q /s %OUTDIR%\*
	goto end
)


%SPHINXBUILD% 2> nul
if errorlevel 9009 (
	echo.
	echo.The 'sphinx-build' command was not found. Make sure you have Sphinx
	echo.installed, then set the SPHINXBUILD environment variable to point
	echo.to the full path of the 'sphinx-build' executable. Alternatively you
	echo.may add the Sphinx directory to PATH.
	echo.
	echo.If you don't have Sphinx installed, grab it from
	echo.http://sphinx-doc.org/
	exit /b 1
)

set defaulted_1=%1
if "%1" == "" (
	set defaulted_1=html
)

if "%defaulted_1%" == "html" (
	%SPHINXBUILD% -b html %ALLSPHINXOPTS% %OUTDIR%/html
	if errorlevel 1 exit /b 1
	echo.
	echo.Build finished. The HTML pages are in %OUTDIR%/html.
	goto end
)

if "%1" == "dirhtml" (
	%SPHINXBUILD% -b dirhtml %ALLSPHINXOPTS% %OUTDIR%/dirhtml
	if errorlevel 1 exit /b 1
	echo.
	echo.Build finished. The HTML pages are in %OUTDIR%/dirhtml.
	goto end
)

if "%1" == "singlehtml" (
	%SPHINXBUILD% -b singlehtml %ALLSPHINXOPTS% %OUTDIR%/singlehtml
	if errorlevel 1 exit /b 1
	echo.
	echo.Build finished. The HTML pages are in %OUTDIR%/singlehtml.
	goto end
)

if "%1" == "pickle" (
	%SPHINXBUILD% -b pickle %ALLSPHINXOPTS% %OUTDIR%/pickle
	if errorlevel 1 exit /b 1
	echo.
	echo.Build finished; now you can process the pickle files.
	goto end
)

if "%1" == "json" (
	%SPHINXBUILD% -b json %ALLSPHINXOPTS% %OUTDIR%/json
	if errorlevel 1 exit /b 1
	echo.
	echo.Build finished; now you can process the JSON files.
	goto end
)

if "%1" == "htmlhelp" (
	%SPHINXBUILD% -b htmlhelp %ALLSPHINXOPTS% %OUTDIR%/htmlhelp
	if errorlevel 1 exit /b 1
	echo.
	echo.Build finished; now you can run HTML Help Workshop with the ^
.hhp project file in %OUTDIR%/htmlhelp.
	goto end
)

if "%1" == "qthelp" (
	%SPHINXBUILD% -b qthelp %ALLSPHINXOPTS% %OUTDIR%/qthelp
	if errorlevel 1 exit /b 1
	echo.
	echo.Build finished; now you can run "qcollectiongenerator" with the ^
.qhcp project file in %OUTDIR%/qthelp, like this:
	echo.^> qcollectiongenerator %OUTDIR%\qthelp\MyProject.qhcp
	echo.To view the help file:
	echo.^> assistant -collectionFile %OUTDIR%\qthelp\MyProject.ghc
	goto end
)

if "%1" == "devhelp" (
	%SPHINXBUILD% -b devhelp %ALLSPHINXOPTS% %OUTDIR%/devhelp
	if errorlevel 1 exit /b 1
	echo.
	echo.Build finished.
	goto end
)

if "%1" == "epub" (
	%SPHINXBUILD% -b epub %ALLSPHINXOPTS% %OUTDIR%/epub
	if errorlevel 1 exit /b 1
	echo.
	echo.Build finished. The epub file is in %OUTDIR%/epub.
	goto end
)

if "%1" == "latex" (
	%SPHINXBUILD% -b latex %ALLSPHINXOPTS% %OUTDIR%/latex
	if errorlevel 1 exit /b 1
	echo.
	echo.Build finished; the LaTeX files are in %OUTDIR%/latex.
	goto end
)

if "%1" == "latexpdf" (
	%SPHINXBUILD% -b latex %ALLSPHINXOPTS% %OUTDIR%/latex
	cd %OUTDIR%/latex
	make all-pdf
	cd %OUTDIR%/..
	echo.
	echo.Build finished; the PDF files are in %OUTDIR%/latex.
	goto end
)

if "%1" == "latexpdfja" (
	%SPHINXBUILD% -b latex %ALLSPHINXOPTS% %OUTDIR%/latex
	cd %OUTDIR%/latex
	make all-pdf-ja
	cd %OUTDIR%/..
	echo.
	echo.Build finished; the PDF files are in %OUTDIR%/latex.
	goto end
)

if "%1" == "text" (
	%SPHINXBUILD% -b text %ALLSPHINXOPTS% %OUTDIR%/text
	if errorlevel 1 exit /b 1
	echo.
	echo.Build finished. The text files are in %OUTDIR%/text.
	goto end
)

if "%1" == "man" (
	%SPHINXBUILD% -b man %ALLSPHINXOPTS% %OUTDIR%/man
	if errorlevel 1 exit /b 1
	echo.
	echo.Build finished. The manual pages are in %OUTDIR%/man.
	goto end
)

if "%1" == "texinfo" (
	%SPHINXBUILD% -b texinfo %ALLSPHINXOPTS% %OUTDIR%/texinfo
	if errorlevel 1 exit /b 1
	echo.
	echo.Build finished. The Texinfo files are in %OUTDIR%/texinfo.
	goto end
)

if "%1" == "gettext" (
	%SPHINXBUILD% -b gettext %I18NSPHINXOPTS% %OUTDIR%/locale
	if errorlevel 1 exit /b 1
	echo.
	echo.Build finished. The message catalogs are in %OUTDIR%/locale.
	goto end
)

if "%1" == "changes" (
	%SPHINXBUILD% -b changes %ALLSPHINXOPTS% %OUTDIR%/changes
	if errorlevel 1 exit /b 1
	echo.
	echo.The overview file is in %OUTDIR%/changes.
	goto end
)

if "%1" == "linkcheck" (
	%SPHINXBUILD% -b linkcheck %ALLSPHINXOPTS% %OUTDIR%/linkcheck
	if errorlevel 1 exit /b 1
	echo.
	echo.Link check complete; look for any errors in the above output ^
or in %OUTDIR%/linkcheck/output.txt.
	goto end
)

if "%1" == "doctest" (
	%SPHINXBUILD% -b doctest %ALLSPHINXOPTS% %OUTDIR%/doctest
	if errorlevel 1 exit /b 1
	echo.
	echo.Testing of doctests in the sources finished, look at the ^
results in %OUTDIR%/doctest/output.txt.
	goto end
)

if "%1" == "xml" (
	%SPHINXBUILD% -b xml %ALLSPHINXOPTS% %OUTDIR%/xml
	if errorlevel 1 exit /b 1
	echo.
	echo.Build finished. The XML files are in %OUTDIR%/xml.
	goto end
)

if "%1" == "pseudoxml" (
	%SPHINXBUILD% -b pseudoxml %ALLSPHINXOPTS% %OUTDIR%/pseudoxml
	if errorlevel 1 exit /b 1
	echo.
	echo.Build finished. The pseudo-XML files are in %OUTDIR%/pseudoxml.
	goto end
)

:end
if "%2" == "pause" (
  pause
)

