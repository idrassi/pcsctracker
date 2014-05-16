PATH=%PATH%;%DDK%\bin\x86

signtool sign /v /a /ac Thawt_CodeSigning_CA.crt /t http://timestamp.verisign.com/scripts/timestamp.dll "..\Release\PcscTracker.exe"
signtool sign /v /a /ac Thawt_CodeSigning_CA.crt /t http://timestamp.verisign.com/scripts/timestamp.dll "..\Release\PcscTracker_x64.exe"

pause