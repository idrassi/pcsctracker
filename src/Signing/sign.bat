PATH=%PATH%;C:\Program Files (x86)\Windows Kits\8.1\bin\x86

set BUILDPATH=%~dp0
cd %BUILDPATH%

set SIGNING_SHA1_CERT_FINGERPRINT=772a9ea740fcaf6e446ea40d7cd0bcb95dd62c1c
set SIGNING_SHA2_CERT_FINGERPRINT=8C33BEE7C816CE61817BF34A6568439CD931BFAB
set INTERMEDIATE_SHA1_CA_CERT="%BUILDPATH%\Thawt_CodeSigning_CA.crt"
set INTERMEDIATE_SHA2_CA_CERT="%BUILDPATH%\GlobalSign_SHA256_EV_CodeSigning_CA.cer"
set TIMESTAMP_SHA1_URL=http://timestamp.verisign.com/scripts/timestamp.dll
set TIMESTAMP_SHA2_URL=http://timestamp.globalsign.com/?signature=sha2

signtool sign /v /sha1 %SIGNING_SHA1_CERT_FINGERPRINT% /ac %INTERMEDIATE_SHA1_CA_CERT% /t %TIMESTAMP_SHA1_URL% "%BUILDPATH%\..\Release\PcscTracker.exe" "%BUILDPATH%\..\Release\PcscTracker_x64.exe"

signtool sign /v /sha1 %SIGNING_SHA2_CERT_FINGERPRINT% /ac %INTERMEDIATE_SHA2_CA_CERT% /as /fd sha256 /tr %TIMESTAMP_SHA2_URL% /td SHA256 "%BUILDPATH%\..\Release\PcscTracker.exe" "%BUILDPATH%\..\Release\PcscTracker_x64.exe"

pause