@echo off
:: You can remove the call below if you do your own manual configuration of the dev machines
call %~dp0\configure\configure_s3.bat
:: Everything below is mandatory
if not defined PM_PYTHON goto :PYTHON_ENV_ERROR
if not defined PM_MODULE goto :MODULE_ENV_ERROR

%PM_PYTHON% %PM_MODULE% %*
goto :END

:PYTHON_ENV_ERROR
echo User environment variable PM_PYTHON is not set! Please configure machine for packman or call configure.bat.
goto :END

:MODULE_ENV_ERROR
echo User environment variable PM_MODULE is not set! Please configure machine for packman or call configure.bat.

:END
